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
#include <algorithm>

using namespace web::http;
using namespace hls::streaming;

playlist::playlist(std::wstring playlist_url, playlist::type playlist_type)
	: playlist_url_(playlist_url)
	, type_(playlist_type)
	, target_duration_(-1)
	, media_sequence_(-1)
{
	// extract root url
	auto p = playlist_url_.find_last_of('/');
	root_url_ = playlist_url_.substr(0, p + 1);

	DEBUG_MESSAGE("playlist::playlist(): created " << playlist_url_.c_str() << " " << (int)type_);
}

void 
playlist::add(playlist::segment segment)
{
	segments_.push_back(segment);
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

				int sleeped = 0;
				int sleep_period = 250;
				while (sleeped < to_sleep)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(sleep_period));
					if (ct.is_canceled())
					{
						DEBUG_MESSAGE("\nplaylist::update(): sleep canceled after: " << sleeped);
						pplx::cancel_current_task();
					}
					sleeped += sleep_period;
				}
			}

			try
			{
				std::unique_ptr<playlist> new_playlist =
					std::unique_ptr<playlist>(playlist::create(playlist_url_, ct).get());

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
		}
		return;

	});	
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
		.then([m3u8_url, ct](std::wstring m3u8_content) 
	{
		if (ct.is_canceled())
		{
			DEBUG_MESSAGE("playlist::create(): canceled");
			pplx::cancel_current_task();
		}

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

		// playlist SEGMENT INFO description
		std::wregex reg_segment(L"#EXTINF:([0-9]+),\n(#EXT-X-DISCONTINUITY\n)?(.*)\n?");
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
			int duration = -1;
			bool discont = false;
			std::wstring file_name;

			// get duration value
			std::wstring val = match.str(1);
			if (!val.empty()) 
			{
				wchar_t* end;
				duration = std::wcstol(val.c_str(), &end, 10);
			}
			// get discountinuity flag
			val = match.str(2);
			if (!val.empty()) 
			{
				discont = true;
			}
			// get segment file name
			val = match.str(3);
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

pplx::task<std::wstring>
playlist::download(std::wstring m3u8_url, pplx::cancellation_token ct)
{
	const int TIMEOUT = 45; // default value is 30

	DEBUG_MESSAGE("playlist::download(): " << m3u8_url.c_str());
	
	if (ct.is_canceled()) 
	{
		DEBUG_MESSAGE("playlist::download(): canceled");
		pplx::cancel_current_task();
	}
	// download playlist m3u8 content
	return 
		http_downloader::download_string(m3u8_url, ct, TIMEOUT)
		.then([](pplx::task<std::wstring> download_task)
	{
		// get result and handle exceptions
		std::wstring m3u8_content;
		try
		{
			m3u8_content = download_task.get();
		}
		// canceled
		catch (const pplx::task_canceled&)
		{
			DEBUG_MESSAGE("playlist::download(): canceled");
			pplx::cancel_current_task();
		}
		// http exception
		catch (const http_exception& he)
		{
			if (he.error_code().value() == 105)
			{
				DEBUG_MESSAGE("playlist::download(): http canceled");
				pplx::cancel_current_task();
			}
			else
			{
				DEBUG_MESSAGE("playlist::download(): http exception: " << he.what());
				throw;
			}
		}
		// unknown exception
		catch (const std::exception& ex)
		{
			DEBUG_MESSAGE("playlist::download(): system exception: " << ex.what());
			throw;
		}

		// done
		DEBUG_MESSAGE("playlist::download(): " << m3u8_content.size() << " bytes");
		return m3u8_content;
	});
}

