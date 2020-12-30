/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

HttpLiveStreamer HLS Streamer

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef HLS_PLAYER_HTTP_LIVE_PLAYER_H
#define HLS_PLAYER_HTTP_LIVE_PLAYER_H

#include "mpeg2/ts/audio_stream_params.h"
#include "hls/streamer/http_live_streamer.h"
#include "PlaylistInfo.h"

#include <pplx/pplxtasks.h>
#include <collection.h>

using namespace mpeg2::ts;
using namespace hls::streamer;

using namespace Platform;
using namespace Platform::Collections;

using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

using namespace Windows::Storage::Streams;

using namespace Windows::Media::Core;
using namespace Windows::Media::MediaProperties;

using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Controls;

namespace hls {
	namespace player
	{
		public delegate void EventHandler(String^);

		[Windows::Foundation::Metadata::WebHostHidden]
		public ref class HttpLivePlayer sealed
		{
			static const int VIDEO_FRAME_DURATION = 40; // in milliseconds = 1/25 * 1000

		private:
			pplx::cancellation_token_source cts_;

		public:
			HttpLivePlayer(MediaElement^);
			virtual ~HttpLivePlayer();

			IAsyncAction^ Play(String^ playlist_url, int width, int height);
			void Cancel();

			IAsyncOperation<IVectorView<PlaylistInfo^>^>^ GetPlaylistInfoList(String^ playlist_url);

		public:
			event EventHandler^ Failed;

			event EventHandler^ Initialized;
			event EventHandler^ Waiting;
			event EventHandler^ Playing;
			event EventHandler^ Paused;
			event EventHandler^ Ended;

		private:
			void reset(); // Cleanup media streams, packets, create new cancelation source
			void cleanup_stream_source(); // Remove event handlers, and mss

			void initialize_stream_source(audio_stream_params);

			pes_packet* get_next_audio_packet(pplx::cancellation_token);
			pes_packet* get_next_video_packet(pplx::cancellation_token);

			MediaStreamSample^ get_next_audio_sample(pplx::cancellation_token);
			MediaStreamSample^ get_next_video_sample(pplx::cancellation_token);

			//pplx::task<std::wstring> get_playlist_url(std::wstring var_playlist_url, pplx::cancellation_token);
			pplx::task<playlist_info> get_playlist_info(std::wstring master_playlist_url, pplx::cancellation_token);

			IBuffer^ make_ibuffer_from_packet(pes_packet*);

		private:
			MediaElement^ media_element_;
			MediaStreamSource^ media_stream_source_;
			
			volatile bool is_canceled_;

			bool is_first_starting;
			bool can_seek;

			AudioStreamDescriptor^ audio_descriptor_;
			VideoStreamDescriptor^ video_descriptor_;

			EventRegistrationToken staring_request_token_;
			EventRegistrationToken closed_request_token_;
			EventRegistrationToken paused_request_token_;
			EventRegistrationToken sample_request_token_;

			void OnStarting(MediaStreamSource^, MediaStreamSourceStartingEventArgs^);
			void OnClosed(MediaStreamSource^, MediaStreamSourceClosedEventArgs^);
			void OnPaused(MediaStreamSource^, Object^);
			void OnSampleRequested(MediaStreamSource^, MediaStreamSourceSampleRequestedEventArgs^);

		private:
			std::wstring playlist_url_;
			std::wstring var_playlist_url_;

			pplx::task<void> streaming_task_;
			audio_stream_params audio_params_;

			std::unique_ptr<segment_stream> audio_segment_stream_;
			std::unique_ptr<segment_stream> video_segment_stream_;

			std::unique_ptr<pes_packet> current_audio_packet_;
			std::unique_ptr<pes_packet> current_video_packet_;

			__int64 current_audio_time_; // in milliseconds
			__int64 current_video_time_; // in milliseconds

			double segment_duration_; // in milliseconds
			int audio_frame_duration_; // in milliseconds
			int video_frame_duration_; // in milliseconds

			int width_;
			int height_;

			std::unique_ptr<http_live_streamer> streamer_;
		};
	}
}

#endif