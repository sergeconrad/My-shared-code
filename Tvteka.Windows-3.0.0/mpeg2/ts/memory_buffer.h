/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef MPEG2_TS_MEMORY_BUFFER_H
#define MPEG2_TS_MEMORY_BUFFER_H

#include <cstddef>
#include <cstdint>

namespace mpeg2 {
	namespace ts {

		class memory_buffer
		{
		public:
			memory_buffer(std::size_t size = 0);
			~memory_buffer();

			std::size_t size() const { return data_size_; }
			const uint8_t* data() const { return buffer_; }

			std::size_t write(const uint8_t* data, std::size_t length);

		private:
			std::size_t calc_alloc_size(std::size_t data_length) const;

		private:
			bool dynamic_; // expandable flag
			const static int DEFAULT_BUFFERSIZE = 16 * 1024; // 16K

			uint8_t* buffer_;   // allocated memory
			uint8_t* curr_pos_; // write pointer

			std::size_t buffer_size_; // allocated memory size
			std::size_t data_size_;   // written data size
		};
	}
}


#endif