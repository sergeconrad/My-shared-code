/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef HLS_STREAMER_HTTP_LIVE_STREAMER_H
#define HLS_STREAMER_HTTP_LIVE_STREAMER_H

#include "playlist.h"
#include "segment_stream.h"
#include "start_segment_info.h"
#include "blocking_collection.h"

#include <memory>
#include <string>

namespace hls {
	namespace streamer {

		class http_live_streamer
		{
			//static const int TIMEOUT = 30; // http_downloader timeout in seconds (30 is default)

			static const int HTTP_TIMEOUT = 30; // http_downloader timeout in seconds (30 is default)
			static const int DOWNLOAD_ATTEMPTS = 5;
			static const int RECONNECT_SLEEP_TIME = 500;

		public:
			static const std::size_t MAX_STREAMS_QUEUE_SIZE = 4; // max segments in streams queue

		public:
			http_live_streamer(std::wstring playlist_url);
			~http_live_streamer();

			pplx::task<audio_stream_params> initialize(pplx::cancellation_token);
			pplx::task<void> streaming(std::size_t start_segment, pplx::cancellation_token);

			void reset();
			void cleanup();

			long get_playlist_duration() const { return playlist_->duration(); }
			bool is_live() const { return playlist_->get_type() == playlist::type::LIVE; }
		
			bool is_audio_completed() { return (audio_segment_streams_->is_completed()) && (audio_segment_streams_->count() == 0); }
			bool is_video_completed() { return (video_segment_streams_->is_completed()) && (video_segment_streams_->count() == 0); }
			
			std::size_t audio_segments_count() const { return audio_segment_streams_->count(); }
			std::size_t video_segments_count() const { return video_segment_streams_->count(); }

			segment_stream* get_next_audio_segment(pplx::cancellation_token);
			segment_stream* get_next_video_segment(pplx::cancellation_token);

			start_segment_info search_segment(long start_second);
			void set_streaming_position(int segment_index, pplx::cancellation_token);

		private:

		private:
			// Playlist
			const std::wstring playlist_url_;
			std::unique_ptr<playlist> playlist_;
			
			// Stream Parameters
			audio_stream_params audio_params_;

			// Audio/Video Streams
			typedef blocking_collection<std::unique_ptr<segment_stream>> segment_stream_queue;

			std::unique_ptr<segment_stream_queue> audio_segment_streams_;
			std::unique_ptr<segment_stream_queue> video_segment_streams_;

		};
	}
}


#endif
