/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "pch.h"
#include "ts_pes_packet.h"

using namespace mpeg2::ts;

uint64_t
ts_pes_packet::pts() const
{
	if (!has_pts()) return 0;

	const uint8_t* pes_p = pes();
	
	// just for debug 'start' must be 2 '0010'
	uint8_t start = (pes_p[9] & 0xF0) >> 4;

	// '0000' '1110'
	uint32_t pts_32_30 = (pes_p[9] & 0x0E) >> 1; // 2 bits

	uint32_t i16 = 0;
	i16 = (pes_p[10] << 8) | pes_p[11];
	// '1111' '1111' '1111' '1110'
	uint32_t pts_29_15 = (i16 & 0xFFFE) >> 1; // 15 bits

	i16 = (pes_p[12] << 8) | pes_p[13];
	// '1111' '1111' '1111' '1110'
	uint32_t pts_14_00 = (i16 & 0xFFFE) >> 1; // 15 bits

	// assemble PTS value
	uint64_t pts = (pts_32_30 << 30);
	pts += (pts_29_15 << 15);
	pts += pts_14_00;

	// Convert to milliseconds
	//pts = pts / 90;
	return pts;
}

bool
ts_pes_packet::is_audio() const
{
	// '110x' 'xxxx' (3-bit) value
	return (stream_id() >> 5) == 0X6; // 0x6='0110'
}

bool
ts_pes_packet::is_video() const
{
	// '1110' 'xxxx' (4-bit) value
	return (stream_id() >> 4) == 0xE; // 0xE='1110'
}

uint8_t
ts_pes_packet::stream_number() const
{
	uint8_t number = 0;
	uint8_t mask = is_audio() ? 0x1F : 0x0F;
	number = stream_id() & mask;
	return number;
}

uint16_t
ts_pes_packet::es_packet_length() const
{
	uint16_t pack_len = pes_packet_lenght();
	pack_len = (pack_len != 0) ? (pack_len - pes_header_data_length() - 3) : pack_len; // 3 = flags up to PES_header_data(3) 
	return pack_len;
}

const uint8_t*
ts_pes_packet::es_data_start() const
{
	// 9 = packet_start_code_prefix (3) + stream_id(1)
	//   + pes_packet_length(2) + flags up to PES_header_data(3) 
	return payload_unit_start_indicator() ? pes() + pes_header_data_length() + 9 : pes();
}

int
ts_pes_packet::es_data_length() const
{
	return packet_ + TS_PACKET_SIZE - es_data_start();
}

/*
// private
int
TsPesPacket::GetNalUnitType(const unsigned char* p) const
{
// skip prefix code
while (*p == 0x00)
p++;
return p[1] & 0x1F; // '0000 0001' '0001 1111' 5-bit value
}

const unsigned char*
TsPesPacket::GetNextNalUnit(const unsigned char* nal_unit_ptr) const
{
return nullptr;
}

std::pair<const unsigned char*, size_t>
TsPesPacket::GetCodecPrivateData() const
{
if (GetNalUnitType(EsDataStart()) == 9) // Access Unit Delimiter
{
// next nal unit type must be 7 - Sequence parameter set unit
// this is start
}

return std::make_pair(nullptr, 0);
}
*/