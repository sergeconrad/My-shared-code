/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "pch.h"
#include "HttpLiveStreamer.h"

#include "core/debug_message.h"
#include "core/http_downloader.h"
#include "mpeg2/ts/ts_splitter.h"

#include <cpprest/http_msg.h>
#include <thread>

using namespace web::http;
using namespace mpeg2::ts;
using namespace hls::streaming;

using namespace Platform;
using namespace Windows::UI::Core;

int PesSegmentStream::DESTROYS = 0;

HttpLiveStreamer::HttpLiveStreamer(String^ playlistUrl)
	: playlist_url_(playlistUrl->Data())
	, audio_segment_streams_(std::unique_ptr<segment_stream_queue>(new segment_stream_queue(MAX_STREAM_QUEUE_SIZE)))
	, video_segment_streams_(std::unique_ptr<segment_stream_queue>(new segment_stream_queue(MAX_STREAM_QUEUE_SIZE)))
{
	DEBUG_MESSAGE("HttpLiveStreamer::HttpLiveStreamer(): created: " << playlist_url_.c_str());
}

HttpLiveStreamer::~HttpLiveStreamer()
{
	DEBUG_MESSAGE("HttpLiveStreamer::~HttpLiveStreamer(): destroyed");
}

IAsyncOperation<StartSegmentInfo^>^ 
HttpLiveStreamer::SearchStreamSegment(double startSecond)
{
	return pplx::create_async(
		[this, startSecond]()
	{
		const double PRECISION = 5.0; // Seconds (if start_seconds 5 sec. after start of ts - take this ts, else next ts)
		DEBUG_MESSAGE("HttpLiveStreamer::SearchStreamSegment(): Looking for " << startSecond << " seconds");

		int segment = -1;
		//size_t index = 0;

		double curr_time = 0;
		double curr_duration = 0;

		for (size_t index = 0; index < (playlist_->segments().size() - 2); ++index)
		{			
			curr_duration = (playlist_->segments()[index]).duration();
			if (((startSecond - curr_time) <= PRECISION) || (curr_time >= startSecond)) //if (time >= start_second)
			{
				DEBUG_MESSAGE("HttpLiveStreamer::SearchStreamSegment(): Found: TS Index: " << index);
				segment = index;
				break;
			}
			curr_time += curr_duration;
		}

		if (segment == -1)
		{
			segment = playlist_->segments().size() - 2; // take the last-1 segment
		}

		DEBUG_MESSAGE("HttpLiveStreamer::SearchStreamSegment(): Found TS Index: " << segment << ": curr_time: " << curr_time << ": next_dur: " << curr_duration);

		return ref new StartSegmentInfo(segment, curr_time);
	});
}

IAsyncAction^
HttpLiveStreamer::SetStreamingPosition(int segment_index)
{
	return pplx::create_async(
		[this, segment_index](pplx::cancellation_token ct)
	{
		if (ct.is_canceled())
		{
			DEBUG_MESSAGE("HttpLiveStreamer::SetStreamingPosition(): beginning canceled ");
			pplx::cancel_current_task();

		}

		DEBUG_MESSAGE("HttpLiveStreamer::SetStreamingPosition(): START: " << segment_index);

		// === download the segment [segment_index]

		std::vector<uint8_t> segment_data;
		auto segment = playlist_->segments()[segment_index];

		std::wstring segment_url = playlist_->root_path() + segment.file_name();
		try
		{
			DEBUG_MESSAGE("HttpLiveStreamer::SetStreamingPosition(): Download: " << segment.file_name());
			segment_data = http_downloader::download_bytes(segment_url, ct, TIMEOUT).get();
		}
		catch (const pplx::task_canceled&)
		{
			DEBUG_MESSAGE("HttpLiveStreamer::SetStreamingPosition(): Canceled");
			pplx::cancel_current_task();
		}
		catch (const http_exception& he)
		{
			if (he.error_code().value() == 105)
			{
				DEBUG_MESSAGE("HttpLiveStreamer::SetStreamingPosition(): Http Canceled");
				pplx::cancel_current_task();
			}
			else
			{
				DEBUG_MESSAGE("HttpLiveStreamer::SetStreamingPosition(): Http Exception: " << he.what());
				throw;
			}
		}
		catch (const std::exception& ex)
		{
			DEBUG_MESSAGE("HttpLiveStreamer::SetStreamingPosition(): Exception: " << ex.what());
			throw;
		}

		if (ct.is_canceled())
		{
			DEBUG_MESSAGE("HttpLiveStreamer::SetStreamingPosition(): after download canceled ");
			pplx::cancel_current_task();

		}

		DEBUG_MESSAGE("HttpLiveStreamer::SetStreamingPosition(): Reset Streams Queues: " << audio_segment_streams_->count() << ":" << video_segment_streams_->count() << " segments");

		audio_segment_streams_.reset(new segment_stream_queue(MAX_STREAM_QUEUE_SIZE));
		video_segment_streams_.reset(new segment_stream_queue(MAX_STREAM_QUEUE_SIZE));

		DEBUG_MESSAGE("HttpLiveStreamer::SetStreamingPosition(): New Streams Queues: " << audio_segment_streams_->count() << ":" << video_segment_streams_->count() << " segments");

		// === split segment 
		ts_splitter stream_splitter;
		stream_splitter.split_buffer(segment_data.data(), segment_data.size());

		std::unique_ptr<segment_stream> seg_stream = nullptr;
		AudioStreamParams^ audio_stream_parameters = nullptr;

		// === add audio segment stream to queue
		try
		{
			pes_packet_stream* pps = stream_splitter.get_audio_stream();
			seg_stream = std::unique_ptr<segment_stream>(new segment_stream(segment, pps));
			audio_segment_streams_->add(std::move(seg_stream), ct);
		}
		catch (const pplx::task_canceled&)
		{
			DEBUG_MESSAGE("HttpLiveStreamer::SetStreamingPosition(): add audio segment stream canceled");
			pplx::cancel_current_task();
		}
		catch (const std::exception& ex)
		{
			DEBUG_MESSAGE("HttpLiveStreamer::SetStreamingPosition(): add audio segment : EXCEPTION: " << ex.what());
		}

		// === add video segment stream to queue
		try
		{
			pes_packet_stream* pps = stream_splitter.get_video_stream();
			seg_stream = std::unique_ptr<segment_stream>(new segment_stream(segment, pps));
			video_segment_streams_->add(std::move(seg_stream), ct);
		}
		catch (const pplx::task_canceled&)
		{
			DEBUG_MESSAGE("HttpLiveStreamer::SetStreamingPosition(): add video segment stream canceled");
			pplx::cancel_current_task();
		}
		catch (const std::exception& ex)
		{
			DEBUG_MESSAGE("HttpLiveStreamer::SetStreamingPosition(): add video segment : EXCEPTION: " << ex.what());
		}

		// streams added			
		DEBUG_MESSAGE("HttpLiveStreamer::SetStreamingPosition(): Audio Queue: " << audio_segment_streams_->count() << " segment(s)");
		DEBUG_MESSAGE("HttpLiveStreamer::SetStreamingPosition(): Video Queue: " << video_segment_streams_->count() << " segment(s)");

		DEBUG_MESSAGE("HttpLiveStreamer::SetStreamingPosition(): initialized");
		return;
	});
}

IAsyncOperation<AudioStreamParams^>^
HttpLiveStreamer::Initialize()
{
	DEBUG_MESSAGE("HttpLiveStreamer::Initialize()");

	return pplx::create_async([this](pplx::cancellation_token ct)
	{
		return
			playlist::create(playlist_url_, ct)
			.then([this, ct](pplx::task<playlist*> create_task)
		{
			// get playlist
			try
			{
				playlist_ = std::unique_ptr<playlist>(create_task.get());
			}
			catch (const pplx::task_canceled&)
			{
				DEBUG_MESSAGE("HttpLiveStreamer::Initialize(): canceled ");
				pplx::cancel_current_task();
			}
			catch (const std::exception& ex)
			{
				DEBUG_MESSAGE("HttpLiveStreamer::Initialize(): exception " << ex.what());
				throw;
			}

			// read the first palylist segment to get audio/video stream parameters			
			if (playlist_->segments().size() == 0)
			{
				throw std::exception("HttpLiveStreamer::Initialize(): zero playlist!");
			}

			if (ct.is_canceled())
			{
				DEBUG_MESSAGE("HttpLiveStreamer::Initialize(): canceled ");
				pplx::cancel_current_task();

			}

			// === download the first segment (1.ts)
			std::vector<uint8_t> segment_data;
			auto segment = playlist_->segments()[0];

			std::wstring segment_url = playlist_->root_path() + segment.file_name();
			try
			{
				segment_data = http_downloader::download_bytes(segment_url, ct, TIMEOUT).get();
			}
			catch (const pplx::task_canceled&)
			{
				DEBUG_MESSAGE("HttpLiveStreamer::Initialize(): Canceled");
				pplx::cancel_current_task();
			}
			catch (const http_exception& he)
			{
				if (he.error_code().value() == 105)
				{
					DEBUG_MESSAGE("HttpLiveStreamer::Initialize(): Http Canceled");
					pplx::cancel_current_task();
				}
				else
				{
					DEBUG_MESSAGE("HttpLiveStreamer::Initialize(): Http Exception: " << he.what());
					throw;
				}
			}
			catch (const std::exception& ex)
			{
				DEBUG_MESSAGE("HttpLiveStreamer::Initialize(): Exception: " << ex.what());
				throw;
			}

			// === split segment 
			ts_splitter stream_splitter;
			stream_splitter.split_buffer(segment_data.data(), segment_data.size());

			std::unique_ptr<segment_stream> seg_stream = nullptr;
			AudioStreamParams^ audio_stream_parameters = nullptr;

			// === add audio segment stream to queue
			try
			{
				pes_packet_stream* pps = stream_splitter.get_audio_stream();

				// === extract aac adts audio stream information (frequency and channels) 

				audio_stream_params as_params = pps->get_audio_stream_params();
				audio_stream_parameters = ref new AudioStreamParams(as_params);

				DEBUG_MESSAGE("HttpLiveStreamer::Initialize(): audio stream info: " << as_params.channels() << ":" << as_params.frequency());

				// add audio segment stream
				seg_stream = std::unique_ptr<segment_stream>(new segment_stream(segment, pps));
				audio_segment_streams_->add(std::move(seg_stream), ct);
			}
			catch (const pplx::task_canceled&)
			{
				DEBUG_MESSAGE("HttpLiveStreamer::Initialize(): add audio segment stream canceled");
				pplx::cancel_current_task();
			}

			// === add video segment stream to queue
			try
			{
				pes_packet_stream* pps = stream_splitter.get_video_stream();
				seg_stream = std::unique_ptr<segment_stream>(new segment_stream(segment, pps));
				video_segment_streams_->add(std::move(seg_stream), ct);
			}
			catch (const pplx::task_canceled&)
			{
				DEBUG_MESSAGE("HttpLiveStreamer::Initialize(): add video segment stream canceled");
				pplx::cancel_current_task();
			}

			// streams added			
			DEBUG_MESSAGE("HttpLiveStreamer::Initialize(): Audio Queue: " << audio_segment_streams_->count() << " segment(s)");
			DEBUG_MESSAGE("HttpLiveStreamer::Initialize(): Video Queue: " << video_segment_streams_->count() << " segment(s)");

			DEBUG_MESSAGE("HttpLiveStreamer::Initialize(): initialized");

			return audio_stream_parameters;
		});
	});
}

IAsyncAction^
HttpLiveStreamer::Streaming(size_t startSegment)
{
	DEBUG_MESSAGE("\nHttpLiveStreamer::Streaming(): Start from " << startSegment);

	return pplx::create_async(
		[this, startSegment](pplx::cancellation_token ct)
	{
		if (playlist_ == nullptr)
		{
			throw std::exception("HttpLiveStreamer::Streaming(): Streamer is not initialized");
		}

		bool is_starting = true;
		size_t current_segment = startSegment;

		while (true)
		{
			if (ct.is_canceled())
			{
				DEBUG_MESSAGE("HttpLiveStreamer::Streaming(): Canceled");
				pplx::cancel_current_task();
			}

			if (current_segment >= playlist_->segments().size())
			{
				// Need to get updated playlist
				if (playlist_->get_type() == playlist::type::LIVE)
				{
					bool is_urgent = false;
					if (is_starting)
					{
						is_urgent = false;
						is_starting = false;
					}
					else
					{
						is_urgent = (AudioSegmentsCount() <= 1 || VideoSegmentsCount() <= 1) ? true : false;
					}
					// update playlist
					try
					{
						playlist_->update(is_urgent, ct).get();
					}
					catch (const pplx::task_canceled&)
					{
						DEBUG_MESSAGE("HttpLiveStreamer::Streaming(): Update: Canceled");
						pplx::cancel_current_task();
					}
					catch (const http_exception& he)
					{
						if (he.error_code().value() == 105)
						{
							DEBUG_MESSAGE("HttpLiveStreamer::Streaming(): Update: Http canceled");
							pplx::cancel_current_task();
						}
						else
						{
							DEBUG_MESSAGE("HttpLiveStreamer::Streaming(): Update: Http exception");
							throw;
						}
					}
					catch (const std::exception& ex)
					{
						DEBUG_MESSAGE("HttpLiveStreamer::Streaming(): Update: Exception: " << ex.what());
						throw;
					}

					// to start of splitting loop
					continue;
				}
				else
				{
					// End of playlist
					DEBUG_MESSAGE("\nHttpLiveStreamer::Streaming(): PLAYLIST ENDED");

					audio_segment_streams_->complete_adding();
					video_segment_streams_->complete_adding();
				}
				break;
			}

			auto segment = playlist_->segments()[current_segment];
			DEBUG_MESSAGE("\nHttpLiveStreamer::Streaming(): Getting segment #" << current_segment << " : " << segment.file_name());

			std::wstring segment_url = playlist_->root_path() + segment.file_name();
			std::vector<uint8_t> segment_data;

			int attempts = 5;
			bool reconnect = true;
			while (reconnect)
			{
				try
				{
					segment_data = http_downloader::download_bytes(segment_url, ct, TIMEOUT).get();
					reconnect = false;
				}
				catch (const http_exception& he)
				{
					if (he.error_code().value() == 105)
					{
						DEBUG_MESSAGE("HttpLiveStreamer::Streaming(): Http canceled");
						pplx::cancel_current_task();
					}
					else
					{

						DEBUG_MESSAGE("HttpLiveStreamer::Streaming(): Http exception: " << he.what());
						if (attempts == 0)
						{
							throw;
						}
						else
						{
							--attempts;
							int to_sleep = 1000; // 1 sec.

							DEBUG_MESSAGE("HttpLiveStreamer::Streaming(): trying reconnect (" << attempts << ") remains");
							DEBUG_MESSAGE("HttpLiveStreamer::Streaming(): sleeping for " << to_sleep);

							std::this_thread::sleep_for(std::chrono::milliseconds(to_sleep));
							reconnect = true;
						}
					}
				}
				catch (const std::exception& ex)
				{
					DEBUG_MESSAGE("HttpLiveStreamer::Streaming(): exception: " << ex.what());
					throw;
				}
			}

			// SPLIT segment 
			ts_splitter stream_splitter;
			stream_splitter.split_buffer(segment_data.data(), segment_data.size());

			// === add audio stream to queue
			std::unique_ptr<segment_stream> seg_stream = nullptr;
			try
			{
				seg_stream = std::unique_ptr<segment_stream>(new segment_stream(segment, stream_splitter.get_audio_stream()));
				audio_segment_streams_->add(std::move(seg_stream), ct);
			}
			catch (const pplx::task_canceled&)
			{
				DEBUG_MESSAGE("HttpLiveStreamer::Streaming(): add audio stream canceled");
				pplx::cancel_current_task();
			}

			// === add video stream to queue
			try
			{
				seg_stream = std::unique_ptr<segment_stream>(new segment_stream(segment, stream_splitter.get_video_stream()));
				video_segment_streams_->add(std::move(seg_stream), ct);

			}
			catch (const pplx::task_canceled&)
			{
				DEBUG_MESSAGE("HttpLiveStreamer::Streaming(): add video stream canceled");
				pplx::cancel_current_task();
			}

			// streams added			
			DEBUG_MESSAGE("HttpLiveStreamer::Streaming(): Audio Queue: " << audio_segment_streams_->count() << " segment(s)");
			DEBUG_MESSAGE("HttpLiveStreamer::Streaming(): Video Queue: " << video_segment_streams_->count() << " segment(s)");

			current_segment += 1;
		}
	});
}

IAsyncOperation<PesSegmentStream^>^
HttpLiveStreamer::GetNextAudioSegmentStreamAsync()
{
	DEBUG_MESSAGE("HttpLiveStreamer::GetNextAudioSegmentStreamAsync()");

	return pplx::create_async(
		[this](pplx::cancellation_token ct)
	{
		if (ct.is_canceled())
		{
			DEBUG_MESSAGE("HttpLiveStreamer::GetNextAudioSegmentStreamAsync(): Canceled");
			pplx::cancel_current_task();
		}

		PesSegmentStream^ segment = nullptr;
		segment_stream* audio_segment_stream = nullptr;
		try
		{
			if (IsAudioCompleted() == false)
			{
				// TODO if audio count == 0 and video == MAX - it may stuck
				// may wait here
				audio_segment_stream = audio_segment_streams_->take(ct).release();
				DEBUG_MESSAGE("HttpLiveStreamer::GetNextAudioSegmentStreamAsync(): Audio/Video Queue: " << audio_segment_streams_->count() << "/" << video_segment_streams_->count() << " remains");
			}
		}
		catch (const pplx::task_canceled&)
		{
			DEBUG_MESSAGE("HttpLiveStreamer::GetNextAudioSegmentStreamAsync(): Canceled");
			pplx::cancel_current_task();
		}

		if (audio_segment_stream != nullptr)
		{
			segment = ref new PesSegmentStream(audio_segment_stream);
		}
		return segment;
	});
}

IAsyncOperation<PesSegmentStream^>^
HttpLiveStreamer::GetNextVideoSegmentStreamAsync()
{
	DEBUG_MESSAGE("HttpLiveStreamer::GetNextVideoSegmentStreamAsync()");

	return pplx::create_async(
		[this](pplx::cancellation_token ct)
	{
		if (ct.is_canceled())
		{
			DEBUG_MESSAGE("HttpLiveStreamer::GetNextVideoSegmentStreamAsync(): Canceled");
			pplx::cancel_current_task();
		}
		PesSegmentStream^ segment = nullptr;
		segment_stream* video_stream = nullptr;
		try
		{
			if (IsVideoCompleted() == false)
			{
				// may wait here
				video_stream = video_segment_streams_->take(ct).release();
				DEBUG_MESSAGE("HttpLiveStreamer::GetNextVideoSegmentStreamAsync(): Audio/Video Queue: " << audio_segment_streams_->count() << "/" << video_segment_streams_->count() << " remains");
			}
		}
		catch (const pplx::task_canceled&)
		{
			DEBUG_MESSAGE("HttpLiveStreamer::GetNextVideoSegmentStreamAsync(): Canceled");
			pplx::cancel_current_task();
		}

		if (video_stream != nullptr)
		{
			segment = ref new PesSegmentStream(video_stream);
		}
		return segment;
	});
}