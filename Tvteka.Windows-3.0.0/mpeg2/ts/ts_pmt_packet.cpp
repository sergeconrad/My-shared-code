/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "pch.h"
#include "ts_pmt_packet.h"

using namespace mpeg2::ts;

const std::vector<std::pair<pid_t, type_t>> 
ts_pmt_packet::get_pes_pids() const
{
	std::vector<std::pair<pid_t, type_t>> pid_table;
	pid_table.reserve(2); // at least audio and video streams

	//const uint8_t* pmt = Pmt();

	// skip pmt header data
	const uint8_t* pos = pmt() + PMT_HEADER_LENGTH;

	// process program info descriptors
	// not implemented
	
	const uint8_t* end = pos + program_info_length();
	while (pos < end)
	{
		// TODO add descriptor processing here
		pos++;
	}

	// process program map table
	const uint8_t HEAD_LEN = 3; // table_id (1 byte) + section_length(2 bytes)
	const uint8_t CRC_32_LEN = 4; // CRC_32 (4 bytes)

	// point to end of program map
	end = pmt() + HEAD_LEN + section_length() - CRC_32_LEN;
	while (pos < end)
	{
		// stream type
		type_t stream_type = pos[0];
		pos += 1;

		// elementary_PID (13-bit) '0001 1111 1111 1111'
		pid_t stream_pid = ((pos[0] << 8) | pos[1]) & 0x1FFF;
		pos += 2;

		// save 
		pid_table.push_back(std::make_pair(stream_pid, stream_type));

		// process es info descriptors
		// not implemented     

		// ES_info_length (12-bit) '0000 1111 1111 1111'
		uint16_t es_info_len = ((pos[0] << 8) | pos[1]) & 0x0FFF;
		pos += 2;

		const uint8_t* end_es = pos + es_info_len;
		while (pos < end_es)
		{
			// TODO add descriptor processing here
			pos++;
		}
	}
	return std::move(pid_table);
}