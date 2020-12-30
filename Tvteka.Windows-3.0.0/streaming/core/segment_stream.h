/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef HLS_STREAMING_SEGMENT_STREAM_H
#define HLS_STREAMING_SEGMENT_STREAM_H

#include <memory>
#include <string>

#include "playlist.h"
#include "mpeg2/ts/pes_packet_stream.h"

using namespace mpeg2::ts;

namespace hls {
	namespace streaming {

		class segment_stream
		{
		public:
			segment_stream(playlist::segment segment, pes_packet_stream* stream)
				: segment_(segment)
				, stream_(stream)
			{}

			~segment_stream()
			{
				DEBUG_MESSAGE("segment_stream::~segment_stream(): deleted: " << segment_.file_name().c_str());
			}

		public:
			int duration() const { return segment_.duration(); }
			bool discontinuity() const { return segment_.discontinuity(); }
			std::wstring file_name() { return segment_.file_name(); }

			std::size_t packets_count() const { return stream_->count(); }
			pes_packet* get_next_pes_packet() { return stream_->get_next_pes_packet(); }

		private:
			playlist::segment segment_;
			std::unique_ptr<pes_packet_stream> stream_;
		};
	}
}

#endif