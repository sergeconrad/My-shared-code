/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef MPEG2_TS_COMMON_H
#define MPEG2_TS_COMMON_H

#include <cstdint>
#include <cstddef>

namespace mpeg2 {
	namespace ts {

		// Type definitions 
		typedef uint16_t pid_t;		// TS packet PID
		typedef uint16_t program_t;	// Program number value type
		typedef uint8_t  type_t;	// Stream type value type

		// TS Packet Constants
		const std::size_t TS_PACKET_SIZE = 188;

		// TS Packet
		const unsigned int HEADER_SIZE = 4; // [0-3] Header
		const unsigned int ADAPTATION_FIELD_OFFSET = 4; // [0-3] header [4] Adaptation field position

		// PAT, PMT
		const unsigned int PAT_HEADER_LENGTH = 8;
		const unsigned int PMT_HEADER_LENGTH = 12;

		// PSI pids
		const unsigned int PAT_PID = 0x0000; // Program Association Table PID
		const unsigned int CAT_PID = 0x0001; // Conditional Access Table PID
		const unsigned int TSD_PID = 0x0002; // Transport stream Description Table PID
		const unsigned int NUL_PID = 0x1FFF; // Null packet 

		// Reserved pid range
		const unsigned int MIN_RESERVED_PID = 0x0003;
		const unsigned int MAX_RESERVED_PID = 0x000F;

		// PMT, Network, Elementary, PES, ... pids range
		const unsigned int MIN_PID = 0x0010;
		const unsigned int MAX_PID = 0x1FFE;

		// PES start code prefix (24-bit) follows ts header
		const unsigned int PES_START_CODE = 0x000001; // '0000 0000' '0000 0000' '0000 0001'
	}
}


#endif