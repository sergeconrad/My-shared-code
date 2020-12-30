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
using namespace hls::streamer;

// PUBLIC INTERFACE

pplx::task<std::wstring> 
http_downloader::download_string_data(
	std::wstring url, pplx::cancellation_token ct, int timeout, int attempts, int sleep_time_ms)
{
	DEBUG_MESSAGE("http_downloader::download_string_data():\n" << url << " T:" << timeout << ", A:" << attempts << ", S:" << sleep_time_ms << "\n");

	return pplx::create_task([=]()
	{
		std::wstring data;

		bool reconnect = true;
		int attempt_count = attempts;

		while (reconnect)
		{
			try
			{
				data = download_string(url, ct, timeout).get();
				reconnect = false;
			}
			catch (const pplx::task_canceled&)
			{
				DEBUG_MESSAGE("http_downloader::download_string_data(): canceled.");
				pplx::cancel_current_task();
			}
			catch (const http_exception& hex)
			{
				DEBUG_MESSAGE("http_downloader::download_string_data(): http: exception: " << hex.what());
				if (attempt_count != 0)
				{
					attempt_count--;
					DEBUG_MESSAGE("http_downloader::download_string_data(): reconnect: " << attempt_count << " attempts remain");
					DEBUG_MESSAGE("http_downloader::download_string_data(): reconnect: sleeping " << sleep_time_ms << " ms.");
					std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_ms));
				}
				else
				{
					DEBUG_MESSAGE("http_downloader::download_string_data(): http: exception: throw." << hex.what());
					throw;
				}
			}
			catch (const std::exception& ex)
			{
				DEBUG_MESSAGE("http_downloader::download_string_data(): exception: " << ex.what());
				throw;
			}
		}

		DEBUG_MESSAGE("http_downloader::download_string_data(): ok. " << data.size() << " b.");
		return data;
	});

}

pplx::task<std::vector<unsigned char>>
http_downloader::download_binary_data(
	std::wstring url, pplx::cancellation_token ct, int timeout, int attempts, int sleep_time_ms)
{
	DEBUG_MESSAGE("http_downloader::download_binary_data():\n" << url << " T:" << timeout << ", A:" << attempts << ", S:" << sleep_time_ms << "\n");
	
	return pplx::create_task([=]()
	{
		std::vector<unsigned char> data;

		bool reconnect = true;
		int attempt_count = attempts;
		
		while (reconnect)
		{
			try
			{
				data = download_bytes(url, ct, timeout).get();
				reconnect = false;
			}
			catch (const pplx::task_canceled&)
			{
				DEBUG_MESSAGE("http_downloader::download_binary_data(): canceled.");
				pplx::cancel_current_task();
			}
			catch (const http_exception& hex)
			{
				DEBUG_MESSAGE("http_downloader::download_binary_data(): http: exception: " << hex.what());
				if (attempt_count != 0)
				{
					attempt_count--;
					DEBUG_MESSAGE("http_downloader::download_binary_data(): reconnect: " << attempt_count << " attempts remain");
					DEBUG_MESSAGE("http_downloader::download_binary_data(): reconnect: sleeping " << sleep_time_ms << " ms.");
					std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_ms));
				}
				else
				{
					DEBUG_MESSAGE("http_downloader::download_binary_data(): http: exception: throw." << hex.what());
					throw;
				}
			}
			catch (const std::exception& ex)
			{
				DEBUG_MESSAGE("http_downloader::download_binary_data(): exception: " << ex.what());
				throw;
			}
		}

		DEBUG_MESSAGE("http_downloader::download_binary_data(): ok. " << data.size() << " b.");
		return data;
	});
}

// PRIVATE FUNCTIONS

pplx::task<std::wstring>
http_downloader::download_string(std::wstring url, pplx::cancellation_token ct, int timeout)
{
	if (ct.is_canceled()) {
		DEBUG_MESSAGE("http_downloader::download_string(): canceled");
		pplx::cancel_current_task();
	}

	http_client_config config;
	config.set_timeout(utility::seconds(timeout));

	http_client client(url, config);
	http_request request(methods::GET);

	request.headers().set_cache_control(L"no-cache");

	// !!! IMPORTANT !!! to get/send updated response 
	// (without it request isn't sent and taken from the winhttp cache) 
	request.headers().add(L"If-None-Match", L"1");

	// Send Request
	return client.request(request, ct).then(
		[ct](pplx::task<http_response> request_task)
	{
		http_response response;
		try
		{
			response = request_task.get();
		}
		catch (const pplx::task_canceled&)
		{
			DEBUG_MESSAGE("http_downloader::download_string(): request(): canceled.");
			pplx::cancel_current_task();
		}
		catch (const http_exception& hex)
		{
			if (hex.error_code().value() == 105)
			{
				DEBUG_MESSAGE("http_downloader::download_string(): request(): canceled.");
				pplx::cancel_current_task();
			}
			else
			{
				DEBUG_MESSAGE("http_downloader::download_string(): request(): http: exception: rethrow: " << hex.what());
				throw;
			}
		}
		catch (const std::exception& ex)
		{
			DEBUG_MESSAGE("http_downloader::download_string(): request(): exception: rethrow: " << ex.what());
			throw;
		}

		// Read Content
		return response.content_ready().then(
			[ct](http_response response)
		{
			if (ct.is_canceled()) {
				DEBUG_MESSAGE("http_downloader::download_string(): content_ready(): canceled");
				pplx::cancel_current_task();
			}

			DEBUG_MESSAGE("http_downloader::download_string(): content_ready(): status code: " << response.status_code());
			
			if (response.status_code() != 200) // 29/01/2015
			{
				DEBUG_MESSAGE("http_downloader::download_string(): content_ready(): Failure: throw http_exception");
				//	throw web::http::http_exception(response.status_code());
				
				std::wstring message = std::to_wstring(response.status_code()) + L" - " + response.reason_phrase();
				throw web::http::http_exception(message);
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
				//DEBUG_MESSAGE("http_downloader::download_string(): ok. " << content.size() << " utf16 symbols");

				return content;
			});
		});
	});
}

pplx::task<std::vector<unsigned char>>
http_downloader::download_bytes(std::wstring url, pplx::cancellation_token ct, int timeout)
{
	concurrency::streams::container_buffer<std::vector<uint8_t>> buffer;

	if (ct.is_canceled()) {
		DEBUG_MESSAGE("http_downloader::download_bytes(): canceled");
		pplx::cancel_current_task();
	}
	//DEBUG_MESSAGE("http_downloader::download_bytes(): " << url.c_str());

	http_client_config config;
	config.set_timeout(utility::seconds(timeout));

	http_client client(url, config);
	http_request request(methods::GET);

	request.set_response_stream(buffer.create_ostream());

	// Send Request
	return client.request(request, ct).then(
		[buffer, ct](pplx::task<http_response> request_task)
	{
		http_response response;
		try
		{
			response = request_task.get();
		}
		catch (const pplx::task_canceled&)
		{
			DEBUG_MESSAGE("http_downloader::download_bytes(): request(): canceled.");
			pplx::cancel_current_task();
		}
		catch (const http_exception& hex)
		{
			if (hex.error_code().value() == 105)
			{
				DEBUG_MESSAGE("http_downloader::download_bytes(): request(): canceled.");
				pplx::cancel_current_task();
			}
			else
			{
				DEBUG_MESSAGE("http_downloader::download_bytes(): request(): http: exception: rethrow: " << hex.what());
				throw;
			}
		}
		catch (const std::exception& ex)
		{
			DEBUG_MESSAGE("http_downloader::download_bytes(): request(): exception: rethrow: " << ex.what());
			throw;
		}

		// Read Content
		return response.content_ready().then(
			[buffer, ct](pplx::task<http_response> response_task)
		{
			if (ct.is_canceled()) {
				DEBUG_MESSAGE("http_downloader::download_bytes(): content_ready(): canceled");
				pplx::cancel_current_task();
			}

			http_response response;
			try
			{
				response = response_task.get();
			}
			catch (const pplx::task_canceled&)
			{
				DEBUG_MESSAGE("http_downloader::download_bytes(): content_ready(): canceled");
				pplx::cancel_current_task();
			}
			catch (const http_exception& hex)
			{
				if (hex.error_code().value() == 105)
				{
					DEBUG_MESSAGE("http_downloader::download_bytes(): content_ready(): canceled.");
					pplx::cancel_current_task();
				}
				else
				{
					DEBUG_MESSAGE("http_downloader::download_bytes(): content_ready(): http: exception: rethrow: " << hex.what());
					throw;
				}
			}
			catch (const std::exception& ex)
			{
				DEBUG_MESSAGE("http_downloader::download_bytes(): content_ready(): exception: " << ex.what());
				throw;
			}

			DEBUG_MESSAGE("http_downloader::download_bytes(): content_ready(): status code: " << response.status_code());

			if (response.status_code() != 200) // 29/01/2015
			{
				DEBUG_MESSAGE("http_downloader::download_bytes(): content_ready(): failure(!200): throw http_exception");
				//throw web::http::http_exception(response.status_code());

				std::wstring message = std::to_wstring(response.status_code()) + L" - " + response.reason_phrase();
				throw web::http::http_exception(message);
			}

			auto bytes = std::move(buffer.collection());
			//DEBUG_MESSAGE("http_downloader::download_bytes(): ok. " << bytes.size() << " bytes");

			bytes.shrink_to_fit();
			return std::move(bytes);
		});
	});
}

