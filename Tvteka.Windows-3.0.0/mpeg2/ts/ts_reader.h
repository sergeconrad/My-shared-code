/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef MPEG2_TS_READER_H
#define MPEG2_TS_READER_H

#include "mpeg2_ts.h"

namespace mpeg2 {
	namespace ts {

		class ts_packet;

		class ts_reader
		{
		public:
			ts_reader(const uint8_t* begin, std::size_t length);

			std::size_t length() const { return end_ - begin_; }
			std::size_t packets() const { return length() / TS_PACKET_SIZE; }

			const ts_packet* read_next_packet();

		private:
			const uint8_t* begin_; // the first byte of byte's buffer
			const uint8_t* end_;   // one past the last byte
			const uint8_t* pos_;   // current read position
		};
	}
}

#endif