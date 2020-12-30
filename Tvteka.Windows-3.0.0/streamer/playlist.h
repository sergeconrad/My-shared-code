/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef HLS_STREAMER_PLAYLIST_H
#define HLS_STREAMER_PLAYLIST_H

#include <string>
#include <vector>
#include <pplx/pplxtasks.h>
#include "playlist_info.h"

namespace hls {
	namespace streamer {

		class playlist
		{
			static const int HTTP_TIMEOUT = 15; // default value is 30
			static const int DOWNLOAD_ATTEMPTS = 3;
			static const int RECONNECT_SLEEP_TIME = 1000;

		public:
			// playlist type
			enum class type
			{
				VOD, LIVE, EMPTY
			};

			// playlist segment file
			class segment
			{
			public:
				segment(std::wstring name, double duration, bool discontinuity)
					: file_name_(name), duration_(duration)
					, discont_(discontinuity) {}
				
				double duration() const { return duration_; }
				bool discontinuity() const { return discont_; }
				std::wstring file_name() const { return file_name_; }

				bool operator==(const segment& rhs) const
				{ 
					if (this == &rhs) return true;
					return file_name() == rhs.file_name();
				}
			private:
				double duration_;
				bool discont_;
				std::wstring file_name_;
			};

		public:
			// create media playlist
			static pplx::task<playlist*> 
				create(std::wstring media_playlist_url, pplx::cancellation_token);
			
			// get media playlists urls from master or regular playlist url
			static pplx::task<std::vector<playlist_info>> 
				get_media_playlist_infos(std::wstring master_playlist_url, pplx::cancellation_token);

		public:
			const std::vector<segment>& segments() const { return segments_; }
			std::wstring root_path() const { return root_url_; }
	
			// get playlist type
			playlist::type get_type() const { return type_; }

			// get target duration
			int target_duration() const { return target_duration_; }
			
			// get media sequence number
			int media_sequence() const { return media_sequence_; }

			// update playlist
			pplx::task<void> update(bool is_urgent, pplx::cancellation_token);

			// playlist duration
			long duration() const;


		private:
			static std::wstring& trim(std::wstring& str);

			static pplx::task<playlist*> parse(std::wstring m3u8, pplx::cancellation_token);
			static pplx::task<std::wstring> download(std::wstring m3u8_url, pplx::cancellation_token);
			
		private:
			// single constructor
			playlist(std::wstring playlist_url, playlist::type playlist_type);
			// deleted
			playlist(const playlist&);
			playlist& operator=(const playlist&);

			// sleep for n milliseconds
			void sleep(int sleep_time_ms, pplx::cancellation_token);

			// add playlist segment
			void add(playlist::segment);

		private:
			playlist::type type_;
			std::wstring root_url_; // url path to download segments
			std::wstring playlist_url_; // url to update playlist
			std::vector<segment> segments_; // playlist segments (1.ts, 2.ts, ...)
			
			int target_duration_;
			int media_sequence_;

			mutable long playlist_duration_;

		};
	}
}

#endif