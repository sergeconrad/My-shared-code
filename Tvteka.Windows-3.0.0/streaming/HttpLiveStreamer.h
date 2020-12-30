/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef HLS_STREAMING_HTTP_LIVE_STREAMER_H
#define HLS_STREAMING_HTTP_LIVE_STREAMER_H

#include <memory>
#include <string>

#include "core/playlist.h"
#include "core/blocking_collection.h"
#include "core/segment_stream.h"

#include "AudioStreamParams.h"
#include "PesSegmentStream.h"
#include "StartSegmentInfo.h"

using namespace Windows::Foundation;

namespace hls {
	namespace streaming {
		
		public ref class HttpLiveStreamer sealed
		{
			static const std::size_t MAX_STREAM_QUEUE_SIZE = 4;
			static const int TIMEOUT = 30; // http_downloader timeout in seconds (30 is default)

		public:
			HttpLiveStreamer(Platform::String^ playlistUrl);
			virtual ~HttpLiveStreamer();
			
			IAsyncOperation<AudioStreamParams^>^ Initialize();
			IAsyncAction^ Streaming(size_t startSegment);


			IAsyncOperation<StartSegmentInfo^>^ SearchStreamSegment(double startSecond);
			IAsyncAction^ SetStreamingPosition(int segment_index);

			IAsyncOperation<PesSegmentStream^>^ GetNextAudioSegmentStreamAsync();
			IAsyncOperation<PesSegmentStream^>^ GetNextVideoSegmentStreamAsync();

			bool IsAudioCompleted() { return audio_segment_streams_->is_completed() && audio_segment_streams_->count() == 0; }
			bool IsVideoCompleted() { return video_segment_streams_->is_completed() && video_segment_streams_->count() == 0; }


			size_t AudioSegmentsCount() { return audio_segment_streams_->count(); }
			size_t VideoSegmentsCount() { return video_segment_streams_->count(); }

			int BufferSize() { return MAX_STREAM_QUEUE_SIZE; }
			bool IsLive() { return playlist_->get_type() == playlist::type::LIVE; }

		private:
			// playlist url
			const std::wstring playlist_url_;

			// playlist
			std::unique_ptr<playlist> playlist_;

			// audio/video streams
			typedef blocking_collection<std::unique_ptr<segment_stream>> segment_stream_queue;

			std::unique_ptr<segment_stream_queue> audio_segment_streams_;
			std::unique_ptr<segment_stream_queue> video_segment_streams_;
		};
	}
}

#endif