/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "pch.h"
#include "pes_packet.h"

#include "debug_message.h"
#include "ts_pes_packet.h"

#include <memory>
//#include "AdtsHeaderPacket.h"

using namespace mpeg2::ts;

pes_packet::pes_packet(const ts_pes_packet* ts_pes_pack)
	: es_buffer_(ts_pes_pack->es_packet_length())
{
	// MUST be 'PayloadUnitStart() = false' & 'HasPayload() = true'
	// & 'tsPesPacket->StartCodePrefix() = 0x01'

	// Save TS PES Header in memory here
	// PES Header Size (188 - X)
	uint8_t pes_header_sz = (ts_pes_pack->es_data_start() - (uint8_t*)ts_pes_pack);
	header_sz_ = pes_header_sz;

	// allocate memory for ts pes packet header
	header_ = (ts_pes_packet*)(new uint8_t[header_sz_]);

	// map ts_pes_packet on the memory
	std::memcpy(header_, ts_pes_pack, pes_header_sz);

	// DEBUG 
	int start_pfx = header_->packet_start_code_prefix();
	bool pts_f = header_->has_pts();
	bool dts_f = header_->has_dts();
	uint64_t pts = header_->pts();
	// END DEBUG

	// write ES data of the start ts pes packet
	// 'payload unit start' = true
	written_bytes_ += es_buffer_.write(ts_pes_pack->es_data_start(), ts_pes_pack->es_data_length());

	// DEBUG (THIS WILL BE IN AudioPesStream class)
	/*
	AdtsHeaderPacket* adtsHeaderPacket = new(const_cast<uint8_t*>(es_buffer_.data())) AdtsHeaderPacket();

	uint16_t sw = adtsHeaderPacket->Syncword();
	uint16_t id = adtsHeaderPacket->Id();
	uint16_t lr = adtsHeaderPacket->Layer();
	uint16_t pr = adtsHeaderPacket->Profile();
	uint8_t sfi = adtsHeaderPacket->SamplFreqIndex();
	uint8_t cfg = adtsHeaderPacket->ChannelConfiguration();
	*/
	return;
}

pes_packet::~pes_packet()
{
	//DEBUG_MESSAGE("~pes_packet(): pes header deleted");
	delete[](uint8_t*)header_;
}

void
pes_packet::add(const ts_pes_packet* ts_pes_pack)
{
	// MUST be 'payload unit start == false' & 'has_payload() == true'

	// Add ES data of segment ts pes packet
	// 'Payload unit start' = false
	written_bytes_ += es_buffer_.write(ts_pes_pack->es_data_start(), ts_pes_pack->es_data_length());

	return;
}

size_t
pes_packet::es_length() const
{
	return es_buffer_.size();
}

size_t
pes_packet::packet_length() const
{
	return header_sz_ + es_buffer_.size();
}

// PTS from TS PES Header
uint64_t 
pes_packet::pts() const 
{ 
	return header_->pts(); 
}

// type of media data in the PES packet
bool 
pes_packet::is_audio() const 
{ 
	return header_->is_audio(); 
}

bool 
pes_packet::is_video() const 
{ 
	return header_->is_video(); 
}

const uint8_t*
pes_packet::es_data() const
{
	return es_buffer_.data();
}