/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "pch.h"
#include "ts_pat_packet.h"

using namespace mpeg2::ts;

// ISO 13818-1
// p. 43 Table 2-25 – Program association section

const std::vector<std::pair<program_t, pid_t>> 
ts_pat_packet::get_programs_table() const
{
	std::vector<std::pair<program_t, pid_t>> table;

	// Number of programs in the program secton
	int count = (section_length() - 9) / 4;
	if (count != 0)
	{
		table.reserve(count); // space for n programs

		//const uint8_t* pat = pat();

		// position of the first byte of program section
		uint8_t pos = PAT_HEADER_LENGTH;

		// For each program get 'program_number' and 'program_map_pid'
		for (int i = 0; i < count; ++i)
		{
			// Program number
			program_t program_number = ((pat()[pos] << 8) | pat()[pos + 1]);
			pos += 2;

			// Program Map PID
			// Last 13 bits of 16 (0x01FFF '0001' '1111' '1111' '1111')
			pid_t pid = ((pat()[pos] << 8) | (pat()[pos + 1])) & 0x01FFF;

			// Save 
			table.push_back(std::make_pair(program_number, pid));
		}
	}
	// Done
	return std::move(table);

}