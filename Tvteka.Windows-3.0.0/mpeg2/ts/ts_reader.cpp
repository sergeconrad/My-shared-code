/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "pch.h"
#include "ts_reader.h"

#include "debug_message.h"
#include "ts_packet.h"

using namespace mpeg2::ts;

ts_reader::ts_reader(const uint8_t* begin, std::size_t length)
	: begin_(begin)
	, end_(begin + length)
	, pos_(begin)
{
	//DEBUG_MESSAGE("ts_reader::ts_reader() created for " << begin << " : " << this->length() << " : " << packets() << " packets");
}

const ts_packet*
ts_reader::read_next_packet()
{
	ts_packet* packet = nullptr;

	if ((end_ - pos_) >= TS_PACKET_SIZE) 
	{
		packet = new (const_cast<uint8_t*>(pos_))ts_packet;
		pos_ += TS_PACKET_SIZE;
	}
	return packet;
}