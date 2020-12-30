/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef HLS_STREAMER_PLAYLIST_INFO_H
#define HLS_STREAMER_PLAYLIST_INFO_H

#include <string>

namespace hls {
	namespace streamer {

		class playlist_info
		{
		public:
			playlist_info()
				: url_(L""), program_id_(0), bandwidth_(0)
			{}

			playlist_info(const playlist_info& pi)
				: url_(pi.url_), program_id_(pi.program_id_), bandwidth_(pi.bandwidth_)
			{}

			playlist_info(std::wstring url, int program_id, int bandwidth)
				: url_(url), program_id_(program_id), bandwidth_(bandwidth)
			{}

		public:
			std::wstring url() const { return url_; }
			int program_id() const { return program_id_; }
			int bandwidth() const { return bandwidth_; }

		private:
			std::wstring url_;
			int program_id_;
			int bandwidth_;
		};

	}
}

#endif