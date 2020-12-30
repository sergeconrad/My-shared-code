/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

HttpLiveStreamer HLS Streamer

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "pch.h"

#include "HttpLivePlayer.h"
#include "hls/streamer/debug_message.h"
#include "hls/streamer/http_downloader.h"

#include <thread>
#include <regex>
#include <cmath>
#include <algorithm>

#include <cpprest/http_msg.h>
using namespace web::http;
using namespace hls::player;

//using namespace Platform::Collections;

//using namespace Windows::Storage::Streams;

// String helper helpers

#include <algorithm>
#include <cctype>

std::wstring& trim(std::wstring& str)
{
	str.erase(str.begin(), find_if(str.begin(), str.end(),
		[](wchar_t& ch)->bool { return !isspace(ch); }));

	str.erase(find_if(str.rbegin(), str.rend(),
		[](wchar_t& ch)->bool { return !isspace(ch); }).base(), str.end());

	return str;
}

std::wstring convert_to_wstring(std::string str)
{
	// Convert to wchar_t
	std::wstring message = std::wstring(str.begin(), str.end());
	return message;
}

// String Helpers


HttpLivePlayer::HttpLivePlayer(MediaElement^ mediaElement)
	: media_element_(mediaElement)
	, streamer_(nullptr)
	, is_canceled_(false)
	, can_seek(true)
	, is_first_starting(true)
	, audio_segment_stream_(nullptr), video_segment_stream_(nullptr)
	, current_audio_packet_(nullptr), current_video_packet_(nullptr)
	, current_audio_time_(0), current_video_time_(0)
	, segment_duration_(0)
	, audio_frame_duration_(0), video_frame_duration_(VIDEO_FRAME_DURATION)
{
	DEBUG_MESSAGE("HttpLivePlayer::HttpLivePlayer(): Created");
}

HttpLivePlayer::~HttpLivePlayer()
{
	DEBUG_MESSAGE("HttpLivePlayer::~HttpLivePlayer(): Dispose");

	reset();
	cleanup_stream_source();

	if (streamer_ != nullptr)
	{
		streamer_->cleanup();
	}

	DEBUG_MESSAGE("HttpLivePlayer::~HttpLivePlayer(): Deleted");
}

void
HttpLivePlayer::reset()
{
	DEBUG_MESSAGE("\nHttpLivePlayer::RESET()");
	DEBUG_MESSAGE("HttpLivePlayer::RESET(): IS_CANCELED_ IS : " << is_canceled_);

	is_canceled_ = false;
	DEBUG_MESSAGE("HttpLivePlayer::RESET(): IS_CANCELED_ NOW : " << is_canceled_ << "\n");

	audio_segment_stream_.reset();
	video_segment_stream_.reset();

	current_audio_packet_.reset();
	current_video_packet_.reset();

	current_audio_time_ = 0;
	current_video_time_ = 0;

	return;
}

IAsyncOperation<IVectorView<PlaylistInfo^>^>^
HttpLivePlayer::GetPlaylistInfoList(String^ playlist_url)
{
	DEBUG_MESSAGE("HttpLivePlayer::GetPlaylistInfoList(): " << playlist_url->Data());

	std::wstring url = playlist_url->Data();
	pplx::cancellation_token ct = cts_.get_token();

	return pplx::create_async([this, url, ct]()
	{
		return playlist::get_media_playlist_infos(url, ct)
			.then([this](pplx::task<std::vector<playlist_info>> playlists_task)
		{
			Vector<PlaylistInfo^>^ infos_list = ref new Vector<PlaylistInfo^>();
			std::vector<playlist_info> pi_vec;
			try
			{
				pi_vec = playlists_task.get();
			}
			catch (const pplx::task_canceled&)
			{
				DEBUG_MESSAGE("HttpLivePlayer::GetPlaylistInfoList(): Canceled.");
				pplx::cancel_current_task();
			}
			catch (const std::exception& ex)
			{
				DEBUG_MESSAGE("HttpLivePlayer::GetPlaylistInfoList(): Exception: " << ex.what());
				throw;
			}

			// sort playlist by bandwidth
			std::sort(pi_vec.begin(), pi_vec.end(),
				[](const playlist_info& x, const playlist_info& y)
			{
				return x.bandwidth() < y.bandwidth();
			});

			for (auto pi : pi_vec)
			{
				infos_list->Append(ref new PlaylistInfo(pi));
			}
			return infos_list->GetView();
		});
	});
}

void
HttpLivePlayer::Cancel()
{
	DEBUG_MESSAGE("\nHttpLivePlayer::Cancel()");

	// Stop MediaElement Playback (not pause)
	if (media_stream_source_ != nullptr)
	{
		media_stream_source_->NotifyError(MediaStreamSourceErrorStatus::Other);
	}

	// Cancel HttpLivePlayer
	is_canceled_ = true;
	cts_.cancel();

	return;
}

IAsyncAction^
HttpLivePlayer::Play(String^ playlistUrl, int width, int height)
{
	DEBUG_MESSAGE("HttpLivePlayer::Play(): " << width << "x" << height << " : " << playlistUrl->Data());

	width_ = width;
	height_ = height;
	var_playlist_url_ = playlistUrl->Data();

	// Reset streams, packets, current time
	reset();

	// Create new cancelation token
	cts_ = pplx::cancellation_token_source();
	pplx::cancellation_token ct = cts_.get_token();

	return pplx::create_async([this, ct]()
	{
		// Get Regular Playlist URL

		return pplx::create_task([this, ct]()
		{
			auto main_task = get_playlist_info(var_playlist_url_, ct)
				.then([this, ct](pplx::task<playlist_info> playlist_info_task)
			{
				bool can_continue = true;
				try
				{
					// PLAYLIST
					playlist_info pl_info = playlist_info_task.get();
					playlist_url_ = pl_info.url();

					DEBUG_MESSAGE("HttpLivePlayer::Play(): Playlist: " << playlist_url_.c_str()
						<< ", id = " << pl_info.program_id() << ", bandwidth = " << pl_info.bandwidth());
				}
				catch (const pplx::task_canceled&)
				{
					DEBUG_MESSAGE("HttpLivePlayer::Play(): Playlist: Canceled");
					return pplx::task_from_result();
				}
				catch (const std::exception& ex)
				{
					DEBUG_MESSAGE("HttpLivePlayer::Play(): Playlist: Exception: " << ex.what());

					// Fire Failed Event
					std::wstring message = convert_to_wstring(ex.what());
					this->Failed(ref new String(message.c_str()));

					return pplx::task_from_result();
				}

				// Create New Streamer
				if (streamer_ == nullptr)
				{
					streamer_ = std::unique_ptr<http_live_streamer>(new http_live_streamer(playlist_url_));
					DEBUG_MESSAGE("\nHttpLivePlayer::Play(): Streamer: NEW CREATED");
				}
				else
				{
					streamer_.reset(new http_live_streamer(playlist_url_));
					DEBUG_MESSAGE("\nHttpLivePlayer::Play(): Streamer: RESET, NEW CREATED");
				}

				// Initialize Streamer, Get Audio Stream Parameters
				return streamer_->initialize(ct)
					.then([this, ct](pplx::task<audio_stream_params> initialize_task)
				{
					try
					{
						// INITIALIZE
						audio_params_ = initialize_task.get();
						DEBUG_MESSAGE("\nHttpLivePlayer::Play(): Streamer: Initialized: " << audio_params_.channels() << " : " << audio_params_.frequency());
					}
					catch (const pplx::task_canceled&)
					{
						DEBUG_MESSAGE("HttpLivePlayer::Play(): Initialize: Canceled");
						return pplx::task_from_result();
					}
					catch (const std::exception ex)
					{
						DEBUG_MESSAGE("HttpLivePlayer::Play(): Initialize: Exception: " << ex.what());

						// Fire Failed Event
						std::wstring message = convert_to_wstring(ex.what());
						this->Failed(ref new String(message.c_str()));

						return pplx::task_from_result();
					}
					// MediaElement Event Handlers

					// Create Media Stream Source
					initialize_stream_source(audio_params_);

					// Get Initial Media Streams
					try
					{

						audio_segment_stream_ = std::unique_ptr<segment_stream>(streamer_->get_next_audio_segment(ct));
						video_segment_stream_ = std::unique_ptr<segment_stream>(streamer_->get_next_video_segment(ct));

						DEBUG_MESSAGE("\nHttpLivePlayer::Play(): Initialize: Initial Segment: "
							<< " VIDEO PACKETS = " << video_segment_stream_->packets_count()
							<< ", AUDIO PACKETS = " << audio_segment_stream_->packets_count());

						if (audio_segment_stream_->packets_count() == 0 || video_segment_stream_->packets_count() == 0)
						{
							throw std::exception("\nHttpLivePlayer::Play(): Initialize: ZERO PACKETS COUNT");
						}

						// Calculate Segment Duration of video packets count (each packet is 40 ms.)
						segment_duration_ = (video_segment_stream_->packets_count() * VIDEO_FRAME_DURATION);

						DEBUG_MESSAGE("\nHttpLivePlayer::Play(): Initialize: SEGMENT DURATION = " << segment_duration_
							<< " (" << audio_segment_stream_->duration() << " from .ts)"
							<< ", VIDEO PACKETS = " << video_segment_stream_->packets_count()
							<< ", AUDIO PACKETS = " << audio_segment_stream_->packets_count());

						// Calculate median Audio Frame Duration based on CALCULATED segment duration
						// The audio frame duration is used in discontinuous segments, video frame duration is 40 always 
						audio_frame_duration_ = (int)(segment_duration_ / audio_segment_stream_->packets_count());

						DEBUG_MESSAGE("HttpLivePlayer::Play(): Initialize: MEDIAN AUDIO FRAME DURATION = " << audio_frame_duration_);

						current_audio_packet_ = std::unique_ptr<pes_packet>(audio_segment_stream_->get_next_pes_packet());
						current_video_packet_ = std::unique_ptr<pes_packet>(video_segment_stream_->get_next_pes_packet());

						current_audio_time_ = 0;
						current_video_time_ = 0;
					}
					catch (const pplx::task_canceled&)
					{
						DEBUG_MESSAGE("HttpLivePlayer::Play(): Get Initial Streams: Canceled");
						return pplx::task_from_result();
					}
					catch (const std::exception& ex)
					{
						DEBUG_MESSAGE("HttpLivePlayer::Play(): Get Initial Streams: Exception: " << ex.what());

						// Fire Failed Event
						std::wstring message = convert_to_wstring(ex.what());
						this->Failed(ref new String(message.c_str()));

						return pplx::task_from_result();
					}
					// Fire Initialized Event
					this->Initialized(ref new String());

					// Start Streaming From Second Segment
					DEBUG_MESSAGE("\nHttpLivePlayer::Play(): Streamer: START STREAMING: 1");
					return streamer_->streaming(1, ct)
						.then([this](pplx::task<void> streaming_task)
					{

						DEBUG_MESSAGE("\nHttpLivePlayer::Play(): Streamer: STREAMING ENDED. is_canceled = " << is_canceled_ << "\n");

						// SAVE STREAMING TASK
						streaming_task_ = streaming_task;

						while (!is_canceled_)
						{
							try
							{
								streaming_task_.get(); // NEW STREAMING TASK MAY BE HERE (FROM SEEKING OPERATION)

								// Playlist ended
								DEBUG_MESSAGE("HttpLivePlayer::Play(): Streaming: Playlist ended");

								// Playlist Ended Event
								this->Ended(ref new String());
								break;
							}
							catch (const pplx::task_canceled&)
							{
								DEBUG_MESSAGE("HttpLivePlayer::Play(): Streaming: Cancelation Request: " << is_canceled_);
								if (!is_canceled_) // is_canceled = FALSE -> CANCELATION FROM OnStarting() SEEK OPERATION
								{
									std::this_thread::sleep_for(std::chrono::milliseconds(500));
									continue;
								}
								else
								{
									// CANCELATION FROM Cancel() CALL
									DEBUG_MESSAGE("\nHttpLivePlayer::Play(): Streaming: Canceled");
									return pplx::task_from_result();
								}
							}
							catch (const http_exception& hex)
							{
								DEBUG_MESSAGE("\nHttpLivePlayer::Play(): Streaming: Http Exception: " << hex.what());

								// Fire Failed Event
								std::wstring message = convert_to_wstring(hex.what());
								this->Failed(ref new String(message.c_str()));

								return pplx::task_from_result();
							}
							catch (const std::exception& ex)
							{
								DEBUG_MESSAGE("\nHttpLivePlayer::Play(): Streaming: Exception: " << ex.what());

								// Fire Failed Event
								std::wstring message = convert_to_wstring(ex.what());
								this->Failed(ref new String(message.c_str()));

								return pplx::task_from_result();
							}
							catch (...)
							{
								DEBUG_MESSAGE("\nHttpLivePlayer::Play(): UNKNOWN EXCEPTION: FAILED CALLED");

								// Fire Failed Event
								this->Failed(ref new String());

								return pplx::task_from_result();
							}
						}

						DEBUG_MESSAGE("HttpLivePlayer::Play(): Streaming: IS_CANCELED_ = " << is_canceled_);
						return pplx::task_from_result();
					});
				});
			});

			try
			{
				DEBUG_MESSAGE("\nHttpLivePlayer::Play(): MAIN TASK: STREAMING STARTED. WAIT COMPLETED STATE. IS_CANCELED_ = " << is_canceled_ << "\n");

				main_task.get();

				DEBUG_MESSAGE("\nHttpLivePlayer::Play(): MAIN TASK: FINISHED. IS_CANCELED_ = " << is_canceled_);
			}
			catch (const std::exception& ex)
			{
				// Never get here, it's just for catching exceptions inside 
				DEBUG_MESSAGE("\nHttpLivePlayer::Play(): MAIN TASK: EXCEPTION: " << ex.what());
			}

			return pplx::task_from_result();

		}, pplx::task_continuation_context::use_arbitrary());

	});
}

void
HttpLivePlayer::initialize_stream_source(audio_stream_params audio_params)
{
	DEBUG_MESSAGE("HttpLivePlayer::initialize_stream_source(): Audio: " << audio_params.channels() << " : " << audio_params.frequency());

	// Audio Descriptor
	AudioEncodingProperties^ audio_props =
		AudioEncodingProperties::CreateAacAdts(audio_params.frequency(), audio_params.channels(), 0);

	audio_descriptor_ = ref new AudioStreamDescriptor(audio_props);

	// Video Descriptor
	VideoEncodingProperties^ video_props = VideoEncodingProperties::CreateH264();
	//video_props->Width = width_;
	//video_props->Height = height_;

	video_props->Width = (width_ == 0) ? 400 : width_;		// 25/01/2015
	video_props->Height = (height_ == 0) ? 300 : height_;

	video_descriptor_ = ref new VideoStreamDescriptor(video_props);
	DEBUG_MESSAGE("HttpLivePlayer::initialize_stream_source(): Video: " << video_props->Width << " : " << video_props->Height);

	// Media Stream Source
	media_stream_source_ = ref new MediaStreamSource(audio_descriptor_, video_descriptor_);

	// MSS Event Handlers
	staring_request_token_ = media_stream_source_->Starting += ref new
		TypedEventHandler<MediaStreamSource^, MediaStreamSourceStartingEventArgs^>
		(this, &HttpLivePlayer::OnStarting);

	closed_request_token_ = media_stream_source_->Closed += ref new
		TypedEventHandler<MediaStreamSource^, MediaStreamSourceClosedEventArgs^>
		(this, &HttpLivePlayer::OnClosed);

	paused_request_token_ = media_stream_source_->Paused += ref new
		TypedEventHandler<MediaStreamSource^, Object^>(this, &HttpLivePlayer::OnPaused);

	sample_request_token_ = media_stream_source_->SampleRequested += ref new
		TypedEventHandler<MediaStreamSource^, MediaStreamSourceSampleRequestedEventArgs^>
		(this, &HttpLivePlayer::OnSampleRequested);

	// MSS Parameters

	uint64_t duration = streamer_->get_playlist_duration();

	TimeSpan playlist_duration;
	playlist_duration.Duration = duration * 10000000; // in 100 nano-seconds - here convert seconds to 100-nano (7 zeros) (nano is 9 zeros)

	media_stream_source_->Duration = playlist_duration;
	media_stream_source_->CanSeek = streamer_->is_live() ? false : true;

	TimeSpan buffer_time;
	buffer_time.Duration = 0;
	media_stream_source_->BufferTime = buffer_time;// TimeSpan(); // Zero

	// Media Element
	media_element_->Dispatcher->RunAsync(CoreDispatcherPriority::Normal,
		ref new DispatchedHandler([this]()
	{
		media_element_->AutoPlay = true;
		media_element_->RealTimePlayback = false;
		media_element_->AreTransportControlsEnabled = true;

		DEBUG_MESSAGE("HttpLivePlayer::SetMediaStreamSource()");
		media_element_->SetMediaStreamSource(media_stream_source_);
	}));

	return;
}

void
HttpLivePlayer::cleanup_stream_source()
{
	DEBUG_MESSAGE("HttpLivePlayer::cleanup_stream_source()");

	if (media_stream_source_ != nullptr)
	{
		media_stream_source_->Starting -= staring_request_token_;
		media_stream_source_->Closed -= closed_request_token_;
		media_stream_source_->Paused -= paused_request_token_;
		media_stream_source_->SampleRequested -= sample_request_token_;

		media_stream_source_ = nullptr;
	}
	return;
}

pplx::task<playlist_info>
HttpLivePlayer::get_playlist_info(std::wstring master_playlist_url, pplx::cancellation_token ct)
{
	DEBUG_MESSAGE("HttpLivePlayer::get_playlist_info(): " << master_playlist_url.c_str());

	return pplx::create_task(
		[master_playlist_url, ct]()
	{
		return playlist::get_media_playlist_infos(master_playlist_url, ct)
			.then([](pplx::task<std::vector<playlist_info>> playlist_infos_task)
		{
			playlist_info media_playlist_info;
			std::vector<playlist_info> infos;

			try
			{
				infos = playlist_infos_task.get();
			}
			catch (const pplx::task_canceled&)
			{
				DEBUG_MESSAGE("HttpLivePlayer::get_playlist_info(): canceled.");
				pplx::cancel_current_task();
			}

			if (infos.size() == 0)
			{
				DEBUG_MESSAGE("HttpLivePlayer::get_playlist_info(): NO PLAYLIST FOUND");
				throw std::exception("HttpLivePlayer::get_playlist_info(): no playlist found");
			}
			if (infos.size() == 1)
			{
				media_playlist_info = infos[0];
				DEBUG_MESSAGE("HttpLivePlayer::get_playlist_info(): only one playlist exists. taken: ");
			}
			else
			{
				// Select playlist with the best bandwidth
				DEBUG_MESSAGE("HttpLivePlayer::get_playlist_info(): select playlist of: " << infos.size());

				// get all programs id's
				std::vector<int> programs;
				for (const auto& p : infos)
				{
					int id = p.program_id();
					auto found = std::find(std::begin(programs), std::end(programs), id);

					if (found == std::end(programs)) {
						programs.push_back(id);
					}
				}
				DEBUG_MESSAGE("HttpLivePlayer::get_playlist_info(): " << programs.size() << " programs found.");

				// get the first program now
				int prog_id = programs[0];

				// GET MAX BANDWIDTH , ANY PROGRAM_ID
				int max_bandwidth = 0;

				auto max = std::max_element(infos.begin(), infos.end(),
					[](const playlist_info& i, const playlist_info& j) { return j.bandwidth() > i.bandwidth(); });

				media_playlist_info = *max;
			}

			DEBUG_MESSAGE("HttpLivePlayer::get_playlist_info(): Taken: " << media_playlist_info.url()
				<< ", id = " << media_playlist_info.program_id() << ", bandwidth =" << media_playlist_info.bandwidth() << "\n");

			return media_playlist_info;
		});

	}, pplx::task_continuation_context::use_arbitrary());
}

pes_packet*
HttpLivePlayer::get_next_audio_packet(pplx::cancellation_token ct)
{
	bool is_waiting = false;
	pes_packet* next_audio_packet = nullptr;

	if (ct.is_canceled())
	{
		DEBUG_MESSAGE("HttpLivePlayer::get_next_audio_packet(): Canceled");
		return nullptr;
	}

	if (audio_segment_stream_ != nullptr)
	{
		next_audio_packet = audio_segment_stream_->get_next_pes_packet();

		if (next_audio_packet == nullptr)
		{
			if (streamer_->is_audio_completed() == false)
			{
				try
				{
					std::size_t audio_segments = streamer_->audio_segments_count();
					std::size_t video_segments = streamer_->video_segments_count();

					DEBUG_MESSAGE("\nHttpLivePlayer::get_next_audio_packet(): Waiting New Audio Segment: Audio = "
						<< audio_segments << ", Video = " << video_segments);

					if (audio_segments == 0 && video_segments == 0)
					{
						DEBUG_MESSAGE("HttpLivePlayer::get_next_audio_packet(): BUFFERING CASE");

						is_waiting = true;
						this->Waiting(nullptr);
					}


					while (true)
					{
						if ((streamer_->audio_segments_count() == 0) && (streamer_->video_segments_count() == streamer_->MAX_STREAMS_QUEUE_SIZE))
						{
							DEBUG_MESSAGE("\nHttpLivePlayer::get_next_audio_packet(): RESET VIDEO TIME TO AUDIO TIME VALUE: " << current_audio_time_ << "/" << current_video_time_ << "\n");
							current_video_time_ = current_audio_time_;
						}

						// Get Next Audio Segment
						audio_segment_stream_.reset(streamer_->get_next_audio_segment(ct));

						if (audio_segment_stream_ == nullptr)
						{
							DEBUG_MESSAGE("HttpLivePlayer::get_next_audio_packet(): NO MORE AUDIO SEGMENTS");
							return nullptr;
						}
						if (audio_segment_stream_->packets_count() != 0)
						{
#ifdef _DEBUG
							if ((audio_segment_stream_->duration() < 6) || (audio_segment_stream_->duration() > 12))
							{
								DEBUG_MESSAGE("HttpLivePlayer::get_next_audio_packet(): UNUSUAL AUDIO SEGMENT DURATION (FROM .TS) " << audio_segment_stream_->duration());
							}
#endif
							// Calculate Median Audio Frame Duration
							//audio_frame_duration_ = (int)((video_segment_stream_->packets_count() * VIDEO_FRAME_DURATION) / audio_segment_stream_->packets_count());

							audio_frame_duration_ = (int)(segment_duration_ / audio_segment_stream_->packets_count()); // 02/02/15
							DEBUG_MESSAGE("\nHttpLivePlayer::get_next_audio_packet(): MEDIAN AUDIO FRAME DURATION = " << audio_frame_duration_);

							break;
						}
						else // SKIP SEGMENT WITH NO PACKETS - GET NEXT ONE
						{
							DEBUG_MESSAGE("HttpLivePlayer::get_next_audio_packet(): ZERO AUDIO SEGMENT STREAM - NO PACKETS, GET NEXT ONE");
							continue;
						}
					}
				}
				catch (const pplx::task_canceled&)
				{
					DEBUG_MESSAGE("HttpLivePlayer::get_next_audio_packet(): Canceled");
					return nullptr;
				}
				catch (const std::exception& ex)
				{
					DEBUG_MESSAGE("HttpLivePlayer::get_next_audio_packet(): Exception: " << ex.what());
					return nullptr;
				}

				if (is_waiting)
				{
					is_waiting = false;
					this->Playing(ref new String());
				}

				DEBUG_MESSAGE("\nHttpLivePlayer::get_next_audio_packet(): Playing AUDIO Segment: '"
					<< audio_segment_stream_->file_name().c_str()
					//<< " DIFF A/V: " << diff
					<< "', Packets = " << audio_segment_stream_->packets_count()
					<< ", T:" << current_audio_time_
					<< ", D:" << audio_segment_stream_->discontinuity());

				// Get Next Audio Packet/Frame
				next_audio_packet = audio_segment_stream_->get_next_pes_packet();

				__int64 diff = (current_audio_time_ - current_video_time_);
				DEBUG_MESSAGE("HttpLivePlayer::get_next_audio_packet(): Difference between Audio/Video = " << diff);
			}
		}
	}
	return next_audio_packet;
}

pes_packet*
HttpLivePlayer::get_next_video_packet(pplx::cancellation_token ct)
{
	bool is_waiting = false;
	pes_packet* next_video_packet = nullptr;

	if (ct.is_canceled())
	{
		DEBUG_MESSAGE("HttpLivePlayer::get_next_video_packet(): Canceled");
		return nullptr;
	}

	if (video_segment_stream_ != nullptr)
	{
		next_video_packet = video_segment_stream_->get_next_pes_packet();

		if (next_video_packet == nullptr)
		{
			if (streamer_->is_video_completed() == false)
			{
				try
				{
					std::size_t audio_segments = streamer_->audio_segments_count();
					std::size_t video_segments = streamer_->video_segments_count();

					DEBUG_MESSAGE("\nHttpLivePlayer::get_next_video_packet(): Waiting New Video Segment: Audio = "
						<< audio_segments << ", Video = " << video_segments);

					if (audio_segments == 0 && video_segments == 0)
					{
						DEBUG_MESSAGE("HttpLivePlayer::get_next_video_packet(): BUFFERING CASE");

						is_waiting = true;
						this->Waiting(ref new String());
					}

					// Get Next Video Segment
					while (true)
					{
						video_segment_stream_.reset(streamer_->get_next_video_segment(ct));

						if (video_segment_stream_ == nullptr)
						{
							DEBUG_MESSAGE("HttpLivePlayer::get_next_video_packet(): NO MORE VIDEO SEGMENTS");
							return nullptr;
						}
						if (video_segment_stream_->packets_count() != 0)
						{
							// Calculate Segment Duration based on video packets count
							segment_duration_ = (video_segment_stream_->packets_count() * VIDEO_FRAME_DURATION);

							DEBUG_MESSAGE("\nHttpLivePlayer::get_next_video_packet(): SEGMENT DURATION = "
								<< segment_duration_ << " (" << video_segment_stream_->duration() << " from .ts)");

							break;
						}
						else // SKIP SEGMENT WITH NO PACKETS - GET NEXT ONE
						{
							DEBUG_MESSAGE("HttpLivePlayer::get_next_video_packet(): ZERO VIDEO STREAM - NO PACKETS, GET NEXT ONE");
							continue;
						}
					}
				}
				catch (const pplx::task_canceled&)
				{
					DEBUG_MESSAGE("HttpLivePlayer::get_next_video_packet(): Canceled");
					return nullptr;
				}
				catch (const std::exception& ex)
				{
					DEBUG_MESSAGE("HttpLivePlayer::get_next_video_packet(): Exception: " << ex.what());
					return nullptr;
				}

				if (is_waiting)
				{
					is_waiting = false;
					this->Playing(ref new String());
				}

				DEBUG_MESSAGE("\nHttpLivePlayer::get_next_video_packet(): Playing VIDEO Segment: '"
					<< video_segment_stream_->file_name().c_str()
					//<< " DIFF A/V: " << diff
					<< "', Packets = " << video_segment_stream_->packets_count()
					<< ", T:" << current_video_time_
					<< ", D:" << video_segment_stream_->discontinuity());

				// Get Next Video Packet/Frame
				next_video_packet = video_segment_stream_->get_next_pes_packet();

				__int64 diff = (current_audio_time_ - current_video_time_);
				DEBUG_MESSAGE("HttpLivePlayer::get_next_video_packet(): Difference between Audio/Video = " << diff);
			}
		}
	}
	return next_video_packet;
}

MediaStreamSample^
HttpLivePlayer::get_next_audio_sample(pplx::cancellation_token ct)
{
	MediaStreamSample^ media_sample = nullptr;

	pes_packet* raw_packet = nullptr;
	pes_packet* next_audio_packet = nullptr;
	try
	{
		if (current_audio_packet_ != nullptr)
		{
			__int64 sample_duration = 0;

			next_audio_packet = get_next_audio_packet(ct);

			if (next_audio_packet != nullptr)
			{
				__int64 curr_time = current_audio_packet_->pts() / 90; // milliseconds
				__int64 next_time = next_audio_packet->pts() / 90;

				sample_duration = (next_time - curr_time);

				// DISCONTINUITY CASE
				if (audio_segment_stream_->discontinuity())
				{
					DEBUG_MESSAGE("HttpLivePlayer::get_next_audio_sample(): AUDIO: DISCONTINUITY: SOURCE SAMPLE DURATION: "
						<< sample_duration << " ( "
						<< curr_time << ":" << next_time
						<< " ), current_audio_time = " << current_audio_time_);

					sample_duration = audio_frame_duration_;
					DEBUG_MESSAGE("HttpLivePlayer::get_next_audio_sample(): AUDIO: DISCONTINUITY: SET DISCONT SAMPLE: " << sample_duration);
				}
				else
				{ // REGULAR SEGMENT
#ifdef _DEBUG
					if ((sample_duration <= 0) || (sample_duration > (audio_frame_duration_ + 5)))
					{
						//DEBUG_MESSAGE("\n - AUDIO: INVALID SAMPLE DURATION: ( <0 | >" << (audio_frame_duration_ + 5) << " ) : " << sample_duration << " ( " << curr_time << "/" << next_time << " ) : " << current_audio_time_);
					}
#endif
					if ((sample_duration < 0))//|| (sample_duration >(audio_frame_duration_ + 100)))
					{
						DEBUG_MESSAGE("\n - AUDIO: INVALID SAMPLE DURATION (d < 0,d > " << (audio_frame_duration_ + 100) << ") : " << sample_duration << ", Current Time A / V = " << current_audio_time_ << " / " << current_video_time_);

						sample_duration = audio_frame_duration_;
						current_video_time_ = current_audio_time_;

						DEBUG_MESSAGE("\n - AUDIO: SET SAMPLE DURATION: " << sample_duration << ", CURRENT TIME: " << current_video_time_);
					}
				}
			}

			// Create IBuffer
			raw_packet = current_audio_packet_.release();

			IBuffer^ buffer = make_ibuffer_from_packet(raw_packet);
			delete raw_packet;

			// Get Playing Time Position
			TimeSpan play_time;
			play_time.Duration = current_audio_time_ * 10000;

			// Create Sample
			media_sample = MediaStreamSample::CreateFromBuffer(buffer, play_time);

			// Set The Next Audio Packet As Current Audio Packet
			current_audio_packet_.reset(next_audio_packet);
			current_audio_time_ += sample_duration;
		}
		else
		{
			DEBUG_MESSAGE("HttpLivePlayer::get_next_audio_sample(): ENDED");
		}
	}
	catch (const pplx::task_canceled&)
	{
		DEBUG_MESSAGE("HttpLivePlayer::get_next_audio_sample(): Canceled");
	}
	catch (const std::exception& ex)
	{
		DEBUG_MESSAGE("HttpLivePlayer::get_next_audio_sample(): Exception: " << ex.what());
		if (raw_packet != nullptr)
		{
			delete raw_packet;
		}
	}

	return media_sample;
}

MediaStreamSample^
HttpLivePlayer::get_next_video_sample(pplx::cancellation_token ct)
{
	MediaStreamSample^ media_sample = nullptr;

	pes_packet* raw_packet = nullptr;
	pes_packet* next_video_packet = nullptr;

	try
	{
		if (current_video_packet_ != nullptr)
		{
			__int64 sample_duration = 0;

			next_video_packet = get_next_video_packet(ct);

			if (next_video_packet != nullptr)
			{
				__int64 curr_time = current_video_packet_->pts() / 90; // milliseconds
				__int64 next_time = next_video_packet->pts() / 90;

				sample_duration = (next_time - curr_time);

				// DISCONTINUITY CASE
				if (video_segment_stream_->discontinuity())
				{
					DEBUG_MESSAGE("HttpLivePlayer::get_next_video_sample(): VIDEO: DISCONTINUITY: SOURCE SAMPLE DURATION: "
						<< sample_duration << " ( "
						<< curr_time << ":" << next_time
						<< " ), current_video_time = " << current_video_time_);

					sample_duration = video_frame_duration_; // (always 40 ms.)
					DEBUG_MESSAGE("HttpLivePlayer::get_next_video_sample(): VIDEO: DISCONTINUITY: SET DISCONT SAMPLE: " << sample_duration);
				}
#ifdef _DEBUG
				//if ((sample_duration <= 0) || (sample_duration > 40))
				if ((sample_duration <= -240) || (sample_duration > 240))
				{
					DEBUG_MESSAGE("\n ===== VIDEO: SAMPLE DURATION: ( < -240 || > 240 ) : " << sample_duration << " (" << curr_time << " : " << next_time << " ) : " << current_video_time_);
				}
#endif
			}

			// Create IBuffer
			raw_packet = current_video_packet_.release();

			IBuffer^ buffer = make_ibuffer_from_packet(raw_packet);
			delete raw_packet;

			// Get Playing Time Position
			TimeSpan play_time;
			play_time.Duration = current_video_time_ * 10000;

			// Create Sample
			media_sample = MediaStreamSample::CreateFromBuffer(buffer, play_time);

			// Set The Next Video Packet As Current One
			current_video_packet_.reset(next_video_packet);
			current_video_time_ += sample_duration;
		}
		else
		{
			DEBUG_MESSAGE("HttpLivePlayer::get_next_video_sample(): ENDED");
		}
	}
	catch (const pplx::task_canceled&)
	{
		DEBUG_MESSAGE("HttpLivePlayer::get_next_video_sample(): Canceled");
	}
	catch (const std::exception& ex)
	{
		DEBUG_MESSAGE("HttpLivePlayer::get_next_video_sample(): Exception: " << ex.what());
		if (raw_packet != nullptr)
		{
			delete raw_packet;
		}
	}

	return media_sample;
}

IBuffer^
HttpLivePlayer::make_ibuffer_from_packet(pes_packet* packet)
{
	IBuffer^ buffer = nullptr;
	const Array<uint8_t>^ data = nullptr;

	DataWriter^ writer = ref new DataWriter();
	try
	{
		data = ref new Array<uint8_t>(const_cast<uint8_t*>(packet->es_data()), packet->es_length());
		writer->WriteBytes(data);
	}
	catch (const std::exception& e)
	{
		DEBUG_MESSAGE("HttpLivePlayer::make_ibuffer_from_packet: Exception: " << e.what());
	}

	buffer = writer->DetachBuffer();

	if (data != nullptr)
	{
		delete data;
	}
	delete writer;

	return buffer;
}

void
HttpLivePlayer::OnStarting(MediaStreamSource ^sender, MediaStreamSourceStartingEventArgs ^args)
{
	int spp = args->Request->StartPosition ? (int)(args->Request->StartPosition->Value.Duration / 10000) : -1;
	DEBUG_MESSAGE("\nHttpLivePlayer::MSS::OnStarting(): POS: " << spp);

	MediaStreamSourceStartingRequest^ request = args->Request;
	MediaStreamSourceStartingRequestDeferral^ deferral = request->GetDeferral();

	if (request->StartPosition)
	{
		__int64 position = request->StartPosition->Value.Duration;
		if (position == 0)
		{
			if (is_first_starting)
			{
				DEBUG_MESSAGE("HttpLivePlayer::OnStarting(): Zero: First Time Call");
				is_first_starting = false;

				TimeSpan ts; ts.Duration = 0;
				request->SetActualStartPosition(ts);
				deferral->Complete();
				return;
			}
		}

		// New Streaming Position
		DEBUG_MESSAGE("\nHttpLivePlayer::OnStarting(): Start: " << position / 10000000 << " Second");
		__int64 start_time_ms = position / 10000;

		if (!can_seek)
		{
			DEBUG_MESSAGE("\nHttpLivePlayer::OnStarting(): Start: " << position / 10000000 << " Second. CAN'T SEEK");
			can_seek = true;

			TimeSpan pos;
			pos.Duration = current_audio_time_ * 10000;
			request->SetActualStartPosition(pos);

			deferral->Complete();
			return;
		}

		// Debug only
		__int64 audio_shift = start_time_ms - current_audio_time_;
		__int64 video_shift = start_time_ms - current_video_time_;
		//DEBUG_MESSAGE("HttpLivePlayer::OnStarting(): Start: Shift: " << audio_shift << ":" << video_shift);

		DEBUG_MESSAGE("\nHttpLivePlayer::OnStarting(): CANCEL CURRENT STREAMING");
		// Cancel Current Streaming
		cts_.cancel();

		// Wait all other tasks get canceled state
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));


		// Look for start ts segment
		start_segment_info seg_info = streamer_->search_segment((long)(start_time_ms / 1000));
		DEBUG_MESSAGE("\nHttpLivePlayer::OnStarting(): Search: Found: TS: " << seg_info.index() << ", Time: " << seg_info.time());

		try
		{
			// Reset current streamer buffers
			streamer_->reset();

			// Set New Streaming Position
			DEBUG_MESSAGE("HttpLivePlayer::OnStarting(): Set Position: " << seg_info.index());

			// Create new cancelation source
			cts_ = pplx::cancellation_token_source();
			DEBUG_MESSAGE("HttpLivePlayer::OnStarting(): NEW CANCELATION TOKEN CREATED");

			// NEW STREAMING TASK (EXCEPTIONS CATCH IN PLAY() FUNCTION)
			streamer_->set_streaming_position(seg_info.index(), cts_.get_token());
		}
		catch (const pplx::task_canceled&)
		{
			DEBUG_MESSAGE("HttpLivePlayer::OnStarting(): Set Position: Canceled");

			is_canceled_ = true;

			deferral->Complete();
			return;
		}
		catch (const std::exception& ex)
		{
			DEBUG_MESSAGE("HttpLivePlayer::OnStarting(): Set Position: Exception: " << ex.what());

			this->Failed(ref new String());

			TimeSpan ts; ts.Duration = 0;
			request->SetActualStartPosition(ts);

			deferral->Complete();
			return;
		}

		// Reset current player streams, packets, times
		this->reset();

		pplx::cancellation_token ct = cts_.get_token();

		audio_segment_stream_.reset(streamer_->get_next_audio_segment(ct));
		video_segment_stream_.reset(streamer_->get_next_video_segment(ct));

		current_audio_packet_.reset(audio_segment_stream_->get_next_pes_packet());
		current_video_packet_.reset(video_segment_stream_->get_next_pes_packet());

		// Set new current streaming time position
		current_audio_time_ = seg_info.time() * 1000; // seconds to milliseconds
		current_video_time_ = seg_info.time() * 1000; // seconds to milliseconds

		// Start new streaming task
		DEBUG_MESSAGE("\nHttpLivePlayer::OnStarting(): Start Streaming From: " << seg_info.index() + 1);
		streaming_task_ = streamer_->streaming(seg_info.index() + 1, ct);

		TimeSpan stream_pos;
		stream_pos.Duration = seg_info.time() * 10000000; // to 100-nano sec. items

		// Done 
		DEBUG_MESSAGE("\nHttpLivePlayer::OnStarting(): SetActualStartPosition(): " << seg_info.time());
		//on_seeking = true;

		request->SetActualStartPosition(stream_pos);
		deferral->Complete();
	}
	else
	{
		can_seek = false;

		TimeSpan pos;
		pos.Duration = current_audio_time_ * 10000;
		request->SetActualStartPosition(pos);

		// Fire Playing Event
		this->Playing(ref new String());

		deferral->Complete();
	}
	return;
}

void
HttpLivePlayer::OnClosed(MediaStreamSource ^sender, MediaStreamSourceClosedEventArgs ^args)
{
	MediaStreamSourceClosedReason reason = args->Request->Reason;
	DEBUG_MESSAGE("\nHttpLivePlayer::OnClosed():");
}

void
HttpLivePlayer::OnPaused(MediaStreamSource^ sender, Object^ obj)
{
	DEBUG_MESSAGE("\nHttpLivePlayer::OnPaused():");

	// Fire Paused Event
	this->Paused(ref new String());
}

void
HttpLivePlayer::OnSampleRequested(MediaStreamSource ^sender, MediaStreamSourceSampleRequestedEventArgs ^args)
{
	//DEBUG_MESSAGE("HttpLivePlayer::MSS::OnSampleRequested()");

	MediaStreamSourceSampleRequest^ request = args->Request;
	MediaStreamSourceSampleRequestDeferral^ defferal = args->Request->GetDeferral();

	pplx::cancellation_token ct = cts_.get_token();
	try
	{
		// Audio
		if (request->StreamDescriptor == audio_descriptor_)
		{
			MediaStreamSample^ media_sample = get_next_audio_sample(ct);
			if (media_sample != nullptr)
			{
				request->Sample = media_sample;
			}
			else
			{
				DEBUG_MESSAGE("HttpLivePlayer::MSS::OnSampleRequested(): AUDIO ENDED");
				request->Sample = nullptr;
			}
		}

		// Video
		if (request->StreamDescriptor == video_descriptor_)
		{
			MediaStreamSample^ media_sample = get_next_video_sample(ct);
			if (media_sample != nullptr)
			{
				media_sample->KeyFrame = true;
				request->Sample = media_sample;

			}
			else
			{
				DEBUG_MESSAGE("HttpLivePlayer::MSS::OnSampleRequested(): VIDEO ENDED");
				request->Sample = nullptr;
			}
		}
	}
	catch (const pplx::task_canceled&)
	{
		DEBUG_MESSAGE("HttpLivePlayer::MSS::OnSampleRequested(): Canceled");
		request->Sample = nullptr;
	}
	catch (const std::exception& ex)
	{
		DEBUG_MESSAGE("HttpLivePlayer::MSS::OnSampleRequested(): Exception: " << ex.what());
		request->Sample = nullptr;
	}

	if (ct.is_canceled())
	{
		DEBUG_MESSAGE("HttpLivePlayer::MSS::OnSampleRequested(): Canceled in the end of call");
		request->Sample = nullptr;
	}

	defferal->Complete();
	return;
}
