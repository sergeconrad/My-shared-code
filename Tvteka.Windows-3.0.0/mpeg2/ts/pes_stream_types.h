/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef MPEG2_PES_STREAM_TYPES_H
#define MPEG2_PES_STREAM_TYPES_H

#include "mpeg2_ts.h"

#include <map>
#include <string>

namespace mpeg2 {
	namespace ts {

		struct media
		{
			enum class type : uint8_t { UNKNOWN = 0, AUDIO, VIDEO };
			enum class supported : bool { YES = true, NO = false };
		};

		// stream type in TS Program Map Table
		// codes from http://www.sno.phy.queensu.ca/~phil/exiftool/TagNames/M2TS.html
		// and http://msdn.microsoft.com/en-us/library/windows/hardware/ff567738(v=vs.85).aspx

		class pes_stream_type
		{
		public:
			static const type_t UNKNOWN_TYPE_CODE = 0xFF;

			pes_stream_type()
				: code_(0)
				, type_(media::type::UNKNOWN)
				, supported_(media::supported::NO)
				, note_(L"Reserved")
			{}

			pes_stream_type(type_t code, media::type type, media::supported supported, std::wstring note)
				: code_(code)
				, type_(type)
				, supported_(supported)
				, note_(note)
			{}
			
			/*
			pes_stream_type(const pes_stream_type& type)
				: code_(type.code_)
				, type_(type.type_)
				, supported_(type.supported_)
				, note_(type.note_)
			{}
			pes_stream_type& operator=(const pes_stream_type& left)
			{
				if (this != &left) {
					code_ = left.code_;
					type_ = left.type_;
					supported_ = left.supported_;
					note_ = left.note_;
				}
				return *this;
			}
			*/
			
			type_t code() const { return code_; }
			media::type type() const { return type_; }
			media::supported is_supported() const { return supported_; }
			std::wstring notes() const { return note_; }

		private:
			type_t code_;
			media::type type_;
			media::supported supported_;
			std::wstring note_;
		};

		// Hold all known pes stream types
		class pes_stream_types
		{
		public:
			static std::map<type_t, pes_stream_type> initialize_types();
			static const pes_stream_type& lookup(type_t);
		private:
			static std::map<type_t, pes_stream_type> types_;
		};
	}
}

#endif