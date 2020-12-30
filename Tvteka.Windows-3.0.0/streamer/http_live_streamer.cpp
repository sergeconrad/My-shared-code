/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "pch.h"

#include "debug_message.h"
#include "http_downloader.h"
#include "http_live_streamer.h"
#include "mpeg2/ts/ts_splitter.h"

#include <thread>
#include <cpprest/http_msg.h>

using namespace web::http;
using namespace hls::streamer;

http_live_streamer::http_live_streamer(std::wstring playlist_url)
	: playlist_url_(playlist_url)
	, audio_segment_streams_(std::unique_ptr<segment_stream_queue>(new segment_stream_queue(MAX_STREAMS_QUEUE_SIZE)))
	, video_segment_streams_(std::unique_ptr<segment_stream_queue>(new segment_stream_queue(MAX_STREAMS_QUEUE_SIZE)))

{
	DEBUG_MESSAGE("http_live_streamer::http_live_streamer(): created: "
		<< MAX_STREAMS_QUEUE_SIZE << " : " << playlist_url_.c_str());
}

http_live_streamer::~http_live_streamer()
{
	DEBUG_MESSAGE("http_live_streamer::http_live_streamer(): deleted: ");
}

void
http_live_streamer::reset()
{
	DEBUG_MESSAGE("http_live_streamer::reset()");
	audio_segment_streams_.reset(new segment_stream_queue(MAX_STREAMS_QUEUE_SIZE));
	video_segment_streams_.reset(new segment_stream_queue(MAX_STREAMS_QUEUE_SIZE));

	return;
}

void
http_live_streamer::cleanup()
{
	DEBUG_MESSAGE("http_live_streamer::cleanup(): Streams");
	audio_segment_streams_.reset();
	video_segment_streams_.reset();

	DEBUG_MESSAGE("http_live_streamer::cleanup(): Playlist");
	playlist_.reset();

	return;
}

pplx::task<audio_stream_params>
http_live_streamer::initialize(pplx::cancellation_token ct)
{
	DEBUG_MESSAGE("http_live_streamer::initialize()");

	return
		// Create playlist
		playlist::create(playlist_url_, ct)
		.then([this, ct](pplx::task<playlist*> playlist_create_task)
	{
		try
		{
			// PLAYLIST CREATED
			playlist_ = std::unique_ptr<playlist>(playlist_create_task.get());
		}
		catch (const pplx::task_canceled&)
		{
			DEBUG_MESSAGE("http_live_streamer::initialize(): Playlist: Canceled");
			pplx::cancel_current_task();
		}
		catch (const std::exception& ex)
		{
			DEBUG_MESSAGE("http_live_streamer::initialize(): Playlist: Exception " << ex.what());
			throw;
		}

		// Check for invalid playlist and cancelation
		if (playlist_->segments().size() == 0)
		{
			throw std::exception("http_live_streamer::initialize(): Zero playlist!");
		}
		if (ct.is_canceled())
		{
			DEBUG_MESSAGE("http_live_streamer::initialize(): Canceled ");
			pplx::cancel_current_task();
		}

		// Read the first playlist segment to get audio/video stream parameters			
		// Download the first segment (1.ts)

		std::vector<uint8_t> segment_data;
		auto segment = playlist_->segments()[0];
		//auto root_path = playlist_->root_path();
		
		// initally assume the target segment stream url is the segment file name
		std::wstring segment_url = segment.file_name(); 

		// check if the target url starts with 'http'
		const std::wstring http_prefix = L"http";
		if (segment_url.compare(0, http_prefix.size(), http_prefix) != 0) {
			// segment file name doesn't start with 'http' 
			// add 'http palylist root path'
			segment_url = playlist_->root_path() + segment.file_name();
		}

		try
		{
			DEBUG_MESSAGE("http_live_streamer::initialize(): Getting Segment #0 : " << segment.file_name());
			
			segment_data = http_downloader::download_binary_data(segment_url, ct, 
				HTTP_TIMEOUT, DOWNLOAD_ATTEMPTS, RECONNECT_SLEEP_TIME).get();
		}
		catch (const pplx::task_canceled&)
		{
			DEBUG_MESSAGE("http_live_streamer::initialize(): Download: Canceled.");
			pplx::cancel_current_task();
		}
		catch (const http_exception& he)
		{
			DEBUG_MESSAGE("http_live_streamer::initialize(): Download: Http: Exception: " << he.what());
			throw;
		}
		catch (const std::exception& ex)
		{
			DEBUG_MESSAGE("http_live_streamer::initialize(): Download: Exception: " << ex.what());
			throw;
		}

		// Split segment 
		ts_splitter stream_splitter;
		try
		{
			stream_splitter.split_buffer(segment_data.data(), segment_data.size());
		}
		catch (const std::exception ex)
		{
			DEBUG_MESSAGE("http_live_streamer::initialize(): Split: Exception: " << ex.what());
		}

		std::unique_ptr<segment_stream> seg_stream = nullptr;

		// Add audio segment stream to queue
		try
		{
			pes_packet_stream* pps = stream_splitter.get_audio_stream();			
			if (pps == nullptr) 
			{
				DEBUG_MESSAGE("http_live_streamer::initialize(): Audio: Error: There's No Audio Stream");
				throw std::exception("http_live_streamer::initialize(): Audio: There's No Audio Stream");
			}
			if (pps->count() == 0)
			{
				delete pps;
				DEBUG_MESSAGE("http_live_streamer::initialize(): Audio: Error: Audio Packets Count is ZERO");
				throw std::exception("http_live_streamer::initialize(): Audio: Audio Packets Count is ZERO");
			}

			// Extract aac adts audio stream information (frequency and channels) 
			audio_params_ = pps->get_audio_stream_params();

			DEBUG_MESSAGE("http_live_streamer::initialize(): Audio Stream Params: "
				<< audio_params_.channels() << " : " << audio_params_.frequency());

			// Add Audio segment stream
			seg_stream = std::unique_ptr<segment_stream>(new segment_stream(segment, pps));
			audio_segment_streams_->add(std::move(seg_stream), ct);
		}
		catch (const pplx::task_canceled&)
		{
			DEBUG_MESSAGE("http_live_streamer::initialize(): Add Audio Segment Stream: Canceled");
			pplx::cancel_current_task();
		}

		// Add Video segment stream to queue
		try
		{
			pes_packet_stream* pps = stream_splitter.get_video_stream();
			if (pps == nullptr)
			{
				DEBUG_MESSAGE("http_live_streamer::initialize(): Video: Error: There's No Video Stream");
				throw std::exception("http_live_streamer::initialize(): Video: There's No Video Stream");
			}
			if (pps->count() == 0)
			{
				delete pps;
				DEBUG_MESSAGE("http_live_streamer::initialize(): Video: Error: Video Packets Count is ZERO");
				throw std::exception("http_live_streamer::initialize(): Video: Video Packets Count is ZERO");
			}

			seg_stream = std::unique_ptr<segment_stream>(new segment_stream(segment, pps));
			video_segment_streams_->add(std::move(seg_stream), ct);
		}
		catch (const pplx::task_canceled&)
		{
			DEBUG_MESSAGE("http_live_streamer::initialize(): Add Video Segment Stream: Canceled");
			pplx::cancel_current_task();
		}

		// streams added			
		DEBUG_MESSAGE("http_live_streamer::initialize(): Added Audio Segment - Audio Queue: " << audio_segment_streams_->count() << " segment(s)");
		DEBUG_MESSAGE("http_live_streamer::initialize(): Added Video Segment - Video Queue: " << video_segment_streams_->count() << " segment(s)");

		DEBUG_MESSAGE("http_live_streamer::initialize(): Audio Stream Parameters Initialized\n");

		return audio_params_;
	});
}

pplx::task<void>
http_live_streamer::streaming(std::size_t start_segment, pplx::cancellation_token ct)
{
	DEBUG_MESSAGE("http_live_streamer::streaming(): start from: " << start_segment);

	return pplx::create_task(
		[this, start_segment, ct]()
	{
		bool is_starting = true;
		size_t current_segment = start_segment;

		while (true)
		{
			if (ct.is_canceled())
			{
				DEBUG_MESSAGE("http_live_streamer::streaming(): Canceled");
				pplx::cancel_current_task();
			}

			// Check of END OF PLAYLIST
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
						is_urgent =
							((audio_segment_streams_->count() <= 1) || (video_segment_streams_->count() <= 1))
							? true : false;
					}
					// update playlist
					try
					{
						playlist_->update(is_urgent, ct).get();
					}
					catch (const pplx::task_canceled&)
					{
						DEBUG_MESSAGE("http_live_streamer::streaming(): Update: Canceled");
						pplx::cancel_current_task();
					}
					catch (const http_exception& he)
					{
						if (he.error_code().value() == 105)
						{
							DEBUG_MESSAGE("http_live_streamer::streaming(): Update: Http: Canceled");
							pplx::cancel_current_task();
						}
						else
						{
							DEBUG_MESSAGE("http_live_streamer::streaming(): Update: Http: Exception: " << he.what());
							throw;
						}
					}
					catch (const std::exception& ex)
					{
						DEBUG_MESSAGE("http_live_streamer::streaming(): Update: Exception: " << ex.what());
						throw;
					}

					// to start of splitting loop
					continue;
				}
				else
				{
					// End of playlist
					DEBUG_MESSAGE("\nhttp_live_streamer::streaming(): PLAYLIST ENDED");

					audio_segment_streams_->complete_adding();
					video_segment_streams_->complete_adding();
				}
				break;
			}

			auto segment = playlist_->segments()[current_segment];
			DEBUG_MESSAGE("\nhttp_live_streamer::streaming(): Getting segment #" << current_segment << " : " << segment.file_name());
						
			std::wstring segment_url = segment.file_name(); // initially target url is the segment file mane
			
			// check if url starts with 'http'
			const std::wstring http_prefix = L"http";
			if (segment_url.compare(0, http_prefix.size(), http_prefix) != 0) {
				// segment url doesn't start with 'http' 
				// so add http playlist root path to the target segment url
				segment_url = playlist_->root_path() + segment_url;
			}
			//std::wstring segment_url = playlist_->root_path() + segment.file_name();
			
			std::vector<uint8_t> segment_data;
			try
			{
				segment_data = http_downloader::download_binary_data(segment_url, ct, 
					HTTP_TIMEOUT, DOWNLOAD_ATTEMPTS, RECONNECT_SLEEP_TIME).get();
			}
			catch (const pplx::task_canceled&)
			{
				DEBUG_MESSAGE("\nhttp_live_streamer::streaming(): Download TS: Canceled.");
				pplx::cancel_current_task();
			}
			catch (const http_exception& he)
			{
				DEBUG_MESSAGE("\nhttp_live_streamer::streaming(): Download TS: Http: Exception: " << he.what());
				throw;

			}
			catch (const std::exception& ex)
			{
				DEBUG_MESSAGE("\nhttp_live_streamer::streaming(): Download TS: Exception: " << ex.what());
				throw;
			}

			/*
			int attempts = 5;
			bool reconnect = true;
			while (reconnect)
			{
			try
			{
			segment_data = http_downloader::download_bytes(segment_url, ct, TIMEOUT).get();
			reconnect = false;
			}
			catch (const pplx::task_canceled&)
			{
			DEBUG_MESSAGE("\nhttp_live_streamer::streaming(): Download TS: Canceled T");
			pplx::cancel_current_task();
			}
			catch (const http_exception& he)
			{
			if (he.error_code().value() == 105)
			{
			DEBUG_MESSAGE("\nhttp_live_streamer::streaming(): Download TS: Http: Canceled H");
			pplx::cancel_current_task();
			}
			else
			{

			DEBUG_MESSAGE("\nhttp_live_streamer::streaming(): Download TS: Http: Exception: " << he.what());
			if (attempts == 0)
			{
			throw;
			}
			else
			{
			--attempts;
			int to_sleep = 1000; // 1 sec.

			DEBUG_MESSAGE("http_live_streamer::streaming(): Trying Reconnect (" << attempts << ") remains");
			DEBUG_MESSAGE("http_live_streamer::streaming(): Sleeping: " << to_sleep);

			std::this_thread::sleep_for(std::chrono::milliseconds(to_sleep));
			reconnect = true;
			}
			}
			}
			catch (const std::exception& ex)
			{
			DEBUG_MESSAGE("\nhttp_live_streamer::streaming(): Download TS: Exception: " << ex.what() << " THROW");
			throw;
			}
			catch (...)
			{
			DEBUG_MESSAGE("\nhttp_live_streamer::streaming():UNKNOWN EXCEPTION: THROW ");
			throw;
			}
			}
			*/

			// SPLIT segment 
			ts_splitter stream_splitter;
			try
			{
				stream_splitter.split_buffer(segment_data.data(), segment_data.size());
			}
			catch (const std::exception& ex)
			{
				DEBUG_MESSAGE("http_live_streamer::streaming(): Split: Exception: " << ex.what());
				throw;
			}

			// Add Audio Segment Stream To Queue
			std::unique_ptr<segment_stream> seg_stream = nullptr;
			try
			{
				seg_stream = std::unique_ptr<segment_stream>(new segment_stream(segment, stream_splitter.get_audio_stream()));
				audio_segment_streams_->add(std::move(seg_stream), ct);
			}
			catch (const pplx::task_canceled&)
			{
				DEBUG_MESSAGE("http_live_streamer::streaming() : Add Audio Segment Stream: Canceled");
				pplx::cancel_current_task();
			}
			catch (const std::exception& ex)
			{
				DEBUG_MESSAGE("http_live_streamer::streaming() : Add Audio Segment Stream: Exception: " << ex.what());
				throw;
			}

			// Add Video Segment Stream To Queue
			try
			{
				seg_stream = std::unique_ptr<segment_stream>(new segment_stream(segment, stream_splitter.get_video_stream()));
				video_segment_streams_->add(std::move(seg_stream), ct);
			}
			catch (const pplx::task_canceled&)
			{
				DEBUG_MESSAGE("http_live_streamer::streaming(): Add Video Segment Stream: Canceled");
				pplx::cancel_current_task();
			}
			catch (const std::exception& ex)
			{
				DEBUG_MESSAGE("http_live_streamer::streaming() : Add Video Segment Stream: Exception: " << ex.what());
				throw;
			}

			// streams added			
			DEBUG_MESSAGE("http_live_streamer::streaming(): Added Audio Segment - Audio Queue: " << audio_segment_streams_->count() << " segment(s)");
			DEBUG_MESSAGE("http_live_streamer::streaming(): Added Video Segment - Video Queue: " << video_segment_streams_->count() << " segment(s)");

			current_segment += 1;
		}

		return;
	});
}

segment_stream*
http_live_streamer::get_next_audio_segment(pplx::cancellation_token ct)
{
	DEBUG_MESSAGE("http_live_streamer::get_next_audio_segment()");

	if (ct.is_canceled())
	{
		DEBUG_MESSAGE("http_live_streamer::get_next_audio_segment(): Begin: Canceled");
		pplx::cancel_current_task();
	}

	segment_stream* segment = nullptr;
	try
	{
		if (!is_audio_completed())
		{
			if (audio_segment_streams_->count() == 0 && video_segment_streams_->count() == MAX_STREAMS_QUEUE_SIZE)
			{
				DEBUG_MESSAGE("\nhttp_live_streamer::get_next_audio_segment(): DROP ALL QUEUED VIDEO SEGMENTS");
				for (int i = 0; i < MAX_STREAMS_QUEUE_SIZE; ++i)
				{
					// Drop one video segment to prevent lock (can't add new segments when queue 0/MAX)
					segment_stream* dropped = video_segment_streams_->take(ct).release();
					delete dropped;
				}
			}
			// Wait audio segment here
			segment = audio_segment_streams_->take(ct).release();

			DEBUG_MESSAGE("http_live_streamer::get_next_audio_segment(): Audio/Video Queue: " << audio_segment_streams_->count() << "/" << video_segment_streams_->count() << " remains");
		}
	}
	catch (const pplx::task_canceled&)
	{
		DEBUG_MESSAGE("http_live_streamer::get_next_audio_segment(): Take: Canceled");
		pplx::cancel_current_task();
	}
	catch (const std::exception& ex)
	{
		DEBUG_MESSAGE("http_live_streamer::get_next_audio_segment(): Take: Exception: " << ex.what());
		throw;
	}

	return segment;
}

segment_stream*
http_live_streamer::get_next_video_segment(pplx::cancellation_token ct)
{
	DEBUG_MESSAGE("http_live_streamer::get_next_video_segment()");

	if (ct.is_canceled())
	{
		DEBUG_MESSAGE("http_live_streamer::get_next_video_segment(): Begin: Canceled");
		pplx::cancel_current_task();
	}

	segment_stream* segment = nullptr;
	try
	{
		if (!is_video_completed())
		{
			// Wait video segment here
			segment = video_segment_streams_->take(ct).release();

			DEBUG_MESSAGE("http_live_streamer::get_next_video_segment(): Audio/Video Queue: " << audio_segment_streams_->count() << "/" << video_segment_streams_->count() << " remains");
		}
	}
	catch (const pplx::task_canceled&)
	{
		DEBUG_MESSAGE("http_live_streamer::get_next_video_segment(): Take: Canceled");
		pplx::cancel_current_task();
	}
	catch (const std::exception& ex)
	{
		DEBUG_MESSAGE("http_live_streamer::get_next_video_segment(): Take: Exception: " << ex.what());
		throw;
	}

	return segment;
}

start_segment_info
http_live_streamer::search_segment(long start_second)
{
	const int PRECISION = 5; // Seconds (if start_seconds 5 sec. after start of ts - take this ts, else next ts)
	DEBUG_MESSAGE("http_live_streamer::search_segment(): Looking for " << start_second << " seconds");

	int segment = -1;
	//size_t index = 0;

	//long curr_time = 0;
	//long curr_duration = 0;

	double curr_time = 0;
	double curr_duration = 0;

	for (size_t index = 0; index < (playlist_->segments().size() - 2); ++index)
	{
		curr_duration = (playlist_->segments()[index]).duration();
		if (((start_second - curr_time) <= PRECISION) || (curr_time >= start_second)) //if (time >= start_second)
		{
			DEBUG_MESSAGE("http_live_streamer::search_segment(): Found: TS Index: " << index);
			segment = index;
			break;
		}
		curr_time += curr_duration;
	}

	if (segment == -1)
	{
		segment = playlist_->segments().size() - 2; // take the last-1 segment
	}

	DEBUG_MESSAGE("http_live_streamer::search_segment(): Found TS Index: " << segment << ": curr_time: " << curr_time << ": next_dur: " << curr_duration);
	return start_segment_info(segment, (long)curr_time);
}

void
http_live_streamer::set_streaming_position(int segment_index, pplx::cancellation_token ct)
{
	DEBUG_MESSAGE("http_live_streamer::set_streaming_position(): " << segment_index);

	std::vector<uint8_t> segment_data;
	auto segment = playlist_->segments()[segment_index];

	std::wstring segment_url = playlist_->root_path() + segment.file_name();
	try
	{
		segment_data = http_downloader::download_binary_data(segment_url, ct,
			HTTP_TIMEOUT, DOWNLOAD_ATTEMPTS, RECONNECT_SLEEP_TIME).get();
	}
	catch (const pplx::task_canceled&)
	{
		DEBUG_MESSAGE("http_live_streamer::set_streaming_position(): Download: Canceled");
		pplx::cancel_current_task();
	}
	catch (const http_exception& he)
	{
		DEBUG_MESSAGE("http_live_streamer::set_streaming_position(): Download: Http: Exception: " << he.what());
		throw;
	}
	catch (const std::exception& ex)
	{
		DEBUG_MESSAGE("http_live_streamer::set_streaming_position(): Download: Exception: " << ex.what());
		throw;
	}

	// Split segment 
	ts_splitter stream_splitter;
	try
	{
		stream_splitter.split_buffer(segment_data.data(), segment_data.size());
	}
	catch (const std::exception ex)
	{
		DEBUG_MESSAGE("http_live_streamer::set_streaming_position(): Split: Exception: " << ex.what());
		throw;
	}

	std::unique_ptr<segment_stream> seg_stream = nullptr;
	// Add audio segment stream to queue
	try
	{
		pes_packet_stream* pps = stream_splitter.get_audio_stream();
		seg_stream = std::unique_ptr<segment_stream>(new segment_stream(segment, pps));
		audio_segment_streams_->add(std::move(seg_stream), ct);
	}
	catch (const pplx::task_canceled&)
	{
		DEBUG_MESSAGE("http_live_streamer::set_streaming_position(): Add Audio Segment Stream: Canceled");
		pplx::cancel_current_task();
	}
	catch (const std::exception ex)
	{
		DEBUG_MESSAGE("http_live_streamer::set_streaming_position(): Add Audio Segment Stream: Exception: " << ex.what());
		throw;
	}
	// Add Video segment stream to queue
	try
	{
		pes_packet_stream* pps = stream_splitter.get_video_stream();
		seg_stream = std::unique_ptr<segment_stream>(new segment_stream(segment, pps));
		video_segment_streams_->add(std::move(seg_stream), ct);
	}
	catch (const pplx::task_canceled&)
	{
		DEBUG_MESSAGE("http_live_streamer::set_streaming_position(): Add Video Segment Stream: Canceled");
		pplx::cancel_current_task();
	}
	catch (const std::exception ex)
	{
		DEBUG_MESSAGE("http_live_streamer::set_streaming_position(): Add Video Segment Stream: Exception: " << ex.what());
		throw;
	}

	// streams added			
	DEBUG_MESSAGE("http_live_streamer::set_streaming_position(): Audio Queue: " << audio_segment_streams_->count() << " segment(s)");
	DEBUG_MESSAGE("http_live_streamer::set_streaming_position(): Video Queue: " << video_segment_streams_->count() << " segment(s)");

	DEBUG_MESSAGE("http_live_streamer::set_streaming_position(): Done");
	return;
}