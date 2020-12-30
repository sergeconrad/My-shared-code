/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "pch.h"
#include "memory_buffer.h"

#include "debug_message.h"
#include <memory>

using namespace mpeg2::ts;

memory_buffer::memory_buffer(size_t size)
{
	dynamic_ = (size == 0) ? true : false;
	if (dynamic_)
	{
		//DEBUG_MESSAGE("memory_buffer::memory_buffer(): DYNAMIC BUFFER");
		buffer_ = (uint8_t*)std::malloc(DEFAULT_BUFFERSIZE);
		buffer_size_ = DEFAULT_BUFFERSIZE;
	}
	else
	{
		buffer_ = (uint8_t*)std::malloc(size);
		buffer_size_ = size;
	}
	curr_pos_ = buffer_;
	data_size_ = 0;
}

memory_buffer::~memory_buffer()
{
	std::free(buffer_);
}

std::size_t
memory_buffer::write(const uint8_t* data, std::size_t length)
{
	std::size_t bytes_to_copy;
	std::size_t free_space = (buffer_ + buffer_size_) - curr_pos_;

	// there's no free place to write
	if (free_space < length)
	{
		if (dynamic_)
		{
			// reallocate buffer with new size
			std::size_t alloc_size = calc_alloc_size(length);
			buffer_size_ += alloc_size;

			buffer_ = (uint8_t*)std::realloc(buffer_, buffer_size_);
			// reset curr_pos because buffer address 
			// after realloc may be changed
			curr_pos_ = buffer_ + data_size_;
			bytes_to_copy = length;
		}
		else
		{
			// trim source data
			bytes_to_copy = free_space;
		}
	}
	// enough space to write
	else
	{
		bytes_to_copy = length;
	}
	// copy data to buffer
	std::memcpy(curr_pos_, data, bytes_to_copy);
	
	curr_pos_ += bytes_to_copy;
	data_size_ += bytes_to_copy;

	return bytes_to_copy;
}

std::size_t
memory_buffer::calc_alloc_size(std::size_t data_length) const
{
	std::size_t alloc_size = DEFAULT_BUFFERSIZE;
	while (alloc_size < data_length)
	{
		alloc_size += DEFAULT_BUFFERSIZE;
	}
	return alloc_size;
}
