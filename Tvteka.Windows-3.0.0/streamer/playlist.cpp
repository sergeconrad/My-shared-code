/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "pch.h"
#include "playlist.h"

#include "debug_message.h"
#include "http_downloader.h"
#include <cpprest/http_msg.h>

#include <memory>
#include <thread>
#include <regex>
#include <cctype>
#include <algorithm>


using namespace web::http;
using namespace hls::streamer;

playlist::playlist(std::wstring playlist_url, playlist::type playlist_type)
	: playlist_url_(playlist_url)
	, type_(playlist_type)
	, playlist_duration_(0)
	, target_duration_(-1)
	, media_sequence_(-1)
{
	// extract root url
	auto p = playlist_url_.find_last_of('/');
	root_url_ = playlist_url_.substr(0, p + 1);

	DEBUG_MESSAGE("playlist::playlist(): empty created: " << playlist_url_.c_str() << " " << (int)type_);
}

void
playlist::add(playlist::segment segment)
{
	segments_.push_back(segment);
}

std::wstring&
playlist::trim(std::wstring& str)
{
	str.erase(str.begin(), find_if(str.begin(), str.end(),
		[](wchar_t& ch)->bool { return !isspace(ch); }));

	str.erase(find_if(str.rbegin(), str.rend(),
		[](wchar_t& ch)->bool { return !isspace(ch); }).base(), str.end());

	return str;
}

pplx::task<void>
playlist::update(bool is_urgent, pplx::cancellation_token ct)
{
	DEBUG_MESSAGE("\nplaylist::update(): is urgent: " << is_urgent << ": " << playlist_url_.c_str());

	return pplx::create_task([this, is_urgent, ct]()
	{
		size_t old_size = segments_.size();
		size_t new_size = old_size;

		while (new_size == old_size)
		{
			if (ct.is_canceled())
			{
				DEBUG_MESSAGE("playlist::update(): canceled");
				pplx::cancel_current_task();
			}

			if (!is_urgent)
			{
				DEBUG_MESSAGE("playlist::update(): target duration: " << this->target_duration());

				// Sleep if not urgent update
				int to_sleep = target_duration() == -1 ? 10 * 1000 : (int)(target_duration() * 1000);// *0.75); // fine tune

				DEBUG_MESSAGE("playlist::update(): sleeping for: " << to_sleep);
				sleep(to_sleep, ct);
			}

			try
			{
				std::unique_ptr<playlist> new_playlist = std::unique_ptr<playlist>(playlist::create(playlist_url_, ct).get());

				DEBUG_MESSAGE("\nplaylist::update(): updated sequence: " << new_playlist->media_sequence() << " prev: " << this->media_sequence());

				if (new_playlist->media_sequence() != this->media_sequence())
				{
					DEBUG_MESSAGE("\nplaylist::update(): new: updated sequence: " << new_playlist->media_sequence() << " duration: " << new_playlist->target_duration());

					// nave got new segments
					// save new sequence, duration
					this->media_sequence_ = new_playlist->media_sequence();
					if (new_playlist->target_duration() != -1)
					{
						this->target_duration_ = new_playlist->target_duration();
					}

					// add new segments
					for (auto s = new_playlist->segments().begin(); s != new_playlist->segments().end(); ++s)
					{
						auto f = std::find(this->segments().begin(), this->segments().end(), *s);
						if (f == this->segments().end())
						{
							// add new segment
							this->add(*s);
						}
					}
					old_size = this->segments().size();
				}
				else // the same sequence
				{
					// sleep for n second
					int to_sleep = target_duration() == -1 ? 10 * 1000 : (int)(target_duration() * 1000);// *0.75); // fine tune

					DEBUG_MESSAGE("playlist::update(): THE SAME SEQUENCE sleeping for: " << to_sleep);
					sleep(to_sleep, ct);
				}

				if (new_playlist->get_type() == playlist::type::VOD)
				{
					type_ = playlist::type::VOD;
				}
			}
			catch (const pplx::task_canceled&)
			{
				DEBUG_MESSAGE("playlist::update(): canceled");
				pplx::cancel_current_task();
			}
			catch (const std::exception& ex)
			{
				DEBUG_MESSAGE("playlist::update(): Exception: " << ex.what());
				throw;
			}
		}
		return;

	});
}

void
playlist::sleep(int sleep_time_ms, pplx::cancellation_token ct)
{
	DEBUG_MESSAGE("playlist::sleep(): sleeping for: " << sleep_time_ms);

	int slept = 0;
	int sleep_period = 250;

	while (slept < sleep_time_ms)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_period));
		if (ct.is_canceled())
		{
			DEBUG_MESSAGE("\nplaylist::sleep(): sleep canceled after: " << slept);
			pplx::cancel_current_task();
		}
		slept += sleep_period;
	}
	return;
}

pplx::task<playlist*>
playlist::create(std::wstring m3u8_url, pplx::cancellation_token ct)
{
	DEBUG_MESSAGE("playlist::create(): " << m3u8_url.c_str());
	if (ct.is_canceled())
	{
		DEBUG_MESSAGE("playlist::create(): canceled");
		pplx::cancel_current_task();
	}
	return
		// download m3u8 
		download(m3u8_url, ct)
		.then([m3u8_url, ct](pplx::task<std::wstring> download_task)
	{
		std::wstring m3u8_content;
		try
		{
			m3u8_content = download_task.get();
		}
		catch (const pplx::task_canceled&)
		{
			DEBUG_MESSAGE("playlist::create(): download: Canceled");
			pplx::cancel_current_task();
		}
		catch (const http_exception& hex)
		{
			DEBUG_MESSAGE("playlist::create(): download: HTTP: Exception: " << hex.what());
			throw;
		}
		catch (const std::exception& ex)
		{
			DEBUG_MESSAGE("playlist::create(): download: Exception: " << ex.what());
			throw;
		}

		if (ct.is_canceled())
		{
			DEBUG_MESSAGE("playlist::create(): canceled");
			pplx::cancel_current_task();
		}

		DEBUG_MESSAGE("playlist::create(): creating: " << m3u8_content);

		// get playlist type
		playlist::type ptype = playlist::type::EMPTY;

		std::wstring::size_type is_found = m3u8_content.find(L"#EXT-X-ENDLIST");
		if (is_found == std::wstring::npos)
		{
			ptype = playlist::type::LIVE;
		}
		else
		{
			ptype = playlist::type::VOD;
		}

		// create empty playlist
		std::unique_ptr<playlist> playlist_ptr = std::unique_ptr<playlist>(new playlist(m3u8_url, ptype));

		if (ct.is_canceled())
		{
			DEBUG_MESSAGE("playlist::create(): canceled");
			pplx::cancel_current_task();
		}

		// get TARGET DURATION
		auto pos = m3u8_content.cbegin();
		auto end = m3u8_content.cend();

		std::wregex reg_dur(L"#EXT-X-TARGETDURATION:([0-9]+)");
		std::wsmatch dur_match;
		if (std::regex_search(pos, end, dur_match, reg_dur))
		{
			std::wstring val = dur_match.str(1);
			wchar_t* end;
			playlist_ptr->target_duration_ = std::wcstol(val.c_str(), &end, 10);
		}

		// get MEDIA SEQUENCE number
		pos = m3u8_content.cbegin();
		end = m3u8_content.cend();

		std::wregex reg_media(L"#EXT-X-MEDIA-SEQUENCE:([0-9]+)");
		std::wsmatch media_match;
		if (std::regex_search(pos, end, media_match, reg_media))
		{
			std::wstring val = media_match.str(1);
			wchar_t* end;
			playlist_ptr->media_sequence_ = std::wcstol(val.c_str(), &end, 10);
		}

		// playlist SEGMENT 
		std::wregex reg_segment(
			L"(#EXT-X-DISCONTINUITY\r?\n)?#EXTINF:([0-9.]+),.*\r?\n(#EXT-X-DISCONTINUITY\r?\n)?(#EXT-X-KEY:.*\r?\n)?(#EXT-X-PROGRAM-DATE-TIME:.*\r?\n)?(.*)\r?\n?"); // 30/01/2015

		std::wsmatch match;

		pos = m3u8_content.cbegin();
		end = m3u8_content.cend();

		// parse each segment data
		while (std::regex_search(pos, end, match, reg_segment))
		{
			// cancelation check
			if (ct.is_canceled())
			{
				DEBUG_MESSAGE("playlist::create(): while: canceled");
				pplx::cancel_current_task();
			}
			// segment data
			double duration = -1;
			bool discont = false;
			std::wstring file_name;

			std::wstring val;

			// get discountinuity flag
			val = match.str(1);
			if (!val.empty())
			{
				discont = true;
			}

			// get duration value
			val = match.str(2);
			if (!val.empty())
			{
				wchar_t* end;
				duration = std::wcstod(val.c_str(), &end);
			}

			// get discountinuity flag
			val = match.str(3);
			if (!val.empty())
			{
				discont = true;
			}

			// get segment file name
			val = match.str(6);
			if (!val.empty())
			{
				file_name = val;
			}
			else
			{
				// TODO: error 'there's no file name string'
			}
			// all segment data extracted
			if (discont)
			{
				DEBUG_MESSAGE("playlist::create(): discontinuity " << file_name.c_str());
			}
			// 25/12/14 temporary 
			if (true)//!discont) // skip discontinuity segments
			{
				playlist_ptr->add(playlist::segment(file_name, duration, discont));
			}
			// go to the next segment
			pos = match.suffix().first;
		}

		DEBUG_MESSAGE("playlist::create(): " << playlist_ptr->segments_.size() << " segments");
		return playlist_ptr.release();
	});

}

pplx::task<std::vector<playlist_info>>
playlist::get_media_playlist_infos(std::wstring master_playlist_url, pplx::cancellation_token ct)
{
	DEBUG_MESSAGE("\nplaylist::get_media_playlist_infos(): " << master_playlist_url);

	return
		http_downloader::download_string_data(master_playlist_url, ct, HTTP_TIMEOUT, DOWNLOAD_ATTEMPTS, RECONNECT_SLEEP_TIME)
		.then([master_playlist_url](pplx::task<std::wstring> download_task)
	{
		std::vector<playlist_info> playlist_infos;
		std::wstring content;
		try
		{
			content = download_task.get();
			DEBUG_MESSAGE("playlist::get_media_playlist_infos(): " << content);
		}
		catch (const pplx::task_canceled&)
		{
			DEBUG_MESSAGE("playlist::get_media_playlist_infos(): canceled.");
			pplx::cancel_current_task();
		}
		catch (const http_exception& hex)
		{
			DEBUG_MESSAGE("playlist::get_media_playlist_infos(): http: exception: " << hex.what());
			throw;
		}
		catch (const std::exception& ex)
		{
			DEBUG_MESSAGE("playlist::get_media_playlist_infos(): exception: " << ex.what());
			throw;
		}

		std::size_t found = content.find(L"#EXTINF:");
		if (found != std::wstring::npos)
		{
			DEBUG_MESSAGE("\nplaylist::get_media_playlist_infos(): Regular Media Playlist Found");
			// It's regular media playlist
			playlist_infos.push_back(playlist_info(master_playlist_url, 0, 0));
		}
		else
		{
			found = content.find(L"#EXT-X-STREAM-INF:");
			if (found == std::wstring::npos)
			{
				throw std::exception("playlist::get_media_playlist_infos(): exception: unsupported m3u8 format!");
			}

			// master playlist parsing
			DEBUG_MESSAGE("\nplaylist::get_media_playlist_infos(): Master Playlist Found");

			// parse each 'ext-x-stream-inf' tag
			//std::wregex x_stream_inf(L"#EXT-X-STREAM-INF:PROGRAM-ID=([0-9]+),BANDWIDTH=([0-9]+).*\r?\n(.*)\r?\n?");
			std::wregex x_stream_inf(L"#EXT-X-STREAM-INF:PROGRAM-ID=([0-9]+),BANDWIDTH=([0-9]+).*\r?\n^(?:[\t ]*(?:\r?\n|\r))*(.*)\r?\n?");

			std::wsmatch match;

			auto pos = content.cbegin();
			auto end = content.cend();

			while (std::regex_search(pos, end, match, x_stream_inf))
			{
				std::wstring url = L"";
				int program_id = 0;
				int bandwidth = 0;

				std::wstring val_str; // work variable

				// get program_id
				val_str = match.str(1);
				if (!val_str.empty())
				{
					wchar_t* end;
					program_id = std::wcstol(val_str.c_str(), &end, 10);
				}

				// get bandwidth
				val_str = match.str(2);
				if (!val_str.empty())
				{
					wchar_t* end;
					bandwidth = std::wcstol(val_str.c_str(), &end, 10);
				}

				// get url
				val_str = match.str(3);
				if (!val_str.empty())
				{
					url = val_str;
					url = trim(url);

					std::size_t is_http = url.find(L"http://");
					if (is_http == std::wstring::npos)
					{
						// extract root url
						auto p = master_playlist_url.find_last_of('/');
						std::wstring root_url = master_playlist_url.substr(0, p + 1);

						if (url[0] == '/') {
							url = url.substr(1);
						}
						
						url = root_url + url;
						
					}
					url = trim(url);
				}
				// add media playlist info
				if (!url.empty())
				{
					playlist_infos.push_back(playlist_info(url, program_id, bandwidth));
				}
				// go to the next #ext-x-stream-inf
				pos = match.suffix().first;
			}
		}

		DEBUG_MESSAGE("\nplaylist::get_media_playlist_infos(): " << playlist_infos.size() << " playlists found");
		return playlist_infos;
	});
}

pplx::task<std::wstring>
playlist::download(std::wstring playlist_url, pplx::cancellation_token ct)
{
	DEBUG_MESSAGE("playlist::download(): " << playlist_url.c_str());

	if (ct.is_canceled())
	{
		DEBUG_MESSAGE("playlist::download(): canceled");
		pplx::cancel_current_task();
	}
	// download playlist m3u8 content
	return pplx::create_task([=]()
	{
		std::wstring m3u8_content;

		try
		{
			m3u8_content =
				http_downloader::download_string_data(
				playlist_url, ct, HTTP_TIMEOUT, DOWNLOAD_ATTEMPTS, RECONNECT_SLEEP_TIME).get();
		}
		catch (const pplx::task_canceled&)
		{
			DEBUG_MESSAGE("\nplaylist::download(): Canceled.");
			pplx::cancel_current_task();
		}
		catch (const http_exception& he)
		{
			DEBUG_MESSAGE("\nplaylist::download(): HTTP: Exception: " << he.what());
		}
		catch (const std::exception& ex)
		{
			DEBUG_MESSAGE("\nplaylist::download(): Exception: " << ex.what() << " THROW");
			throw;
		}
		catch (...)
		{
			DEBUG_MESSAGE("\nplaylist::download(): UNKNOWN EXCEPTION: THROW ");
			throw;
		}

		return m3u8_content;
	});
}

long
playlist::duration() const
{
	if (get_type() == playlist::type::VOD)
	{
		if (playlist_duration_ == 0)
		{
			//long dur = 0;
			double dur = 0.0;
			for (auto i : segments_)
			{
				dur += i.duration();
			}
			playlist_duration_ = (long)dur;
		}
	}
	return playlist_duration_;
}