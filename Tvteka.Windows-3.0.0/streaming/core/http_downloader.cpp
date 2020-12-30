/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "pch.h"
#include "http_downloader.h"
#include "debug_message.h"

#include <codecvt>
#include <cpprest/http_client.h>

using namespace web::http;
using namespace web::http::client;
using namespace hls::streaming;

pplx::task<std::wstring>
http_downloader::download_string(std::wstring url, pplx::cancellation_token ct, int timeout)
{
	if (ct.is_canceled()) {
		DEBUG_MESSAGE("http_downloader::download_string(): canceled");
		pplx::cancel_current_task();
	}
	// added 23/12/14 timeout config
	http_client_config config;
	config.set_timeout(utility::seconds(timeout));

	http_client client(url, config);

	return client.request(methods::GET, ct).then(
		[ct](http_response response)
	{
		if (ct.is_canceled()) {
			DEBUG_MESSAGE("http_downloader::download_string(): request(): canceled");
			pplx::cancel_current_task();
		}

		return response.content_ready().then(
			[ct](http_response response)
		{
			if (ct.is_canceled()) {
				DEBUG_MESSAGE("http_downloader::download_string(): content_ready(): canceled");
				pplx::cancel_current_task();
			}
			
			DEBUG_MESSAGE("http_downloader::download_string(): content_ready(): status code: " << response.status_code());

			if (response.status_code() == 404) // 03.01.2015
			{
				throw std::exception("http_downloader: 404 - not found"); //web::http::http_exception(404);
			}

			concurrency::streams::istream stream = response.body();
			concurrency::streams::container_buffer<std::string> buffer;

			return stream.read_to_end(buffer).then(
				[buffer, ct](size_t sz)
			{
				if (ct.is_canceled()) {
					DEBUG_MESSAGE("http_downloader::download_string(): read_to_end(): canceled");
					pplx::cancel_current_task();
				}
				//DEBUG_MESSAGE("http_downloader::download_string(): " << sz << " bytes downloaded");
				const std::string& data = buffer.collection();

				std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf16conv;
				std::wostringstream ss;
				ss << utf16conv.from_bytes(data.c_str());

				std::wstring content = ss.str();
				//DEBUG_MESSAGE("http_downloader::download_string(): " << content.size() << " utf16 symbols");

				return content;
			});
		});
	});
}

pplx::task<std::vector<unsigned char>>
http_downloader::download_bytes(std::wstring url, pplx::cancellation_token ct, int timeout)
{
	if (ct.is_canceled()) {
		DEBUG_MESSAGE("http_downloader::download_bytes(): canceled");
		pplx::cancel_current_task();
	}
	//DEBUG_MESSAGE("http_downloader::download_bytes(): " << url.c_str());
	
	// added 23/12/14 timeout config
	http_client_config config;
	config.set_timeout(utility::seconds(timeout));

	http_client client(url, config);
	http_request request(methods::GET);

	concurrency::streams::container_buffer<std::vector<uint8_t>> buffer;
	request.set_response_stream(buffer.create_ostream());

	return client.request(request, ct).then(
		[buffer, ct](http_response response)
	{
		if (ct.is_canceled()) {
			DEBUG_MESSAGE("http_downloader::download_bytes(): request(): canceled");
			pplx::cancel_current_task();
		}
		//DEBUG_MESSAGE("http_downloader::download_bytes(): " << response.headers().content_length() << " bytes to download");

		return response.content_ready().then(
			[buffer, ct](http_response response)
		{
			if (ct.is_canceled()) {
				DEBUG_MESSAGE("http_downloader::download_bytes(): content_ready(): canceled");
				pplx::cancel_current_task();
			}

			DEBUG_MESSAGE("http_downloader::download_bytes(): content_ready(): status code: " << response.status_code());			
			
			// TODO status code 404 - another player connected to tvteka
			if (response.status_code() == 404) // 01.01.2015
			{
				throw std::exception("http_downloader: 404 - not found"); //web::http::http_exception(404);
			}

			auto bytes = std::move(buffer.collection());
			//DEBUG_MESSAGE("http_downloader::download_bytes(): " << bytes.size() << " bytes downloaded");

			bytes.shrink_to_fit();
			return std::move(bytes);
		});
	});
}