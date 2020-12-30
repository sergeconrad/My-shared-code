/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef HLS_STREAMING_HTTP_DOWNLOADER_H
#define HLS_STREAMING_HTTP_DOWNLOADER_H

#include <string>
#include <vector>
#include <thread>

#include <pplx/pplxtasks.h>

namespace hls {
	namespace streamer
	{
		class http_downloader
		{
		public:
			static pplx::task<std::wstring> download_string_data
				(std::wstring url, pplx::cancellation_token, int timeout, int attempts, int sleep_time_ms);

			static pplx::task<std::vector<unsigned char>> download_binary_data
				(std::wstring url, pplx::cancellation_token, int timeout, int attempts, int sleep_time_ms);
		
		private:
			static pplx::task<std::wstring> download_string(std::wstring url, pplx::cancellation_token, int timeout);
			static pplx::task<std::vector<unsigned char>> download_bytes(std::wstring url, pplx::cancellation_token, int timeout);
		};
	}
}

#endif
