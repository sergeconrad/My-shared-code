/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef MPEG2_TS_PMT_PACKET_H
#define MPEG2_TS_PMT_PACKET_H

#include "ts_packet.h"
#include <vector>

namespace mpeg2 {
	namespace ts {

		// ISO/IEC 13818-1 : 2000 (E) 
		// p. 46 Table 2-28 – Transport Stream program map section

		class ts_pmt_packet : public ts_packet
		{
			// get pointer to the program map table
			inline const uint8_t* pmt() const
			{
				return section_data();
			}
		public:
			ts_pmt_packet() {}  // = delete
			~ts_pmt_packet() {} // = delete
			ts_pmt_packet(const ts_pmt_packet&) {} // = delete
			ts_pmt_packet& operator=(const ts_pmt_packet&) {} // = delete

			// PMT Table Header fields

			// table_id
			inline uint8_t table_id() const
			{	// '1111' '1111' (8-bit) value
				return pmt()[0];
			}

			// section_syntax_indicator
			inline bool section_syntax() const
			{   // '1000' '0000' (1-bit) flag
				return (pmt()[1] & 0x80) != 0;
			}

			// section_length
			inline uint16_t section_length() const
			{	// '0000' '1111' '1111' '1111' (12-bit) value
				return ((pmt()[1] << 8) | (pmt()[2])) & 0xFFF;
			}

			// program_number
			inline uint16_t program_number() const
			{	// '1111' '1111' '1111' '1111' (16-bit) value
				return ((pmt()[3] << 8) | (pmt()[4]));
			}

			// version_number
			inline uint8_t version_number() const
			{	// '0011' '1110' (5-bit) value
				return (pmt()[5] & 0x3E) >> 1;
			}

			// current_next_indicator
			inline bool current_next_indicator() const
			{	// '0000' '0001' flag
				return (pmt()[5] & 0x01) != 0;
			}

			// section_number
			inline uint8_t section_number() const
			{	// '1111' '1111' (8-bit) value
				return pmt()[6];
			}

			// last_section_number
			inline uint8_t last_section_number() const
			{	// '1111' '1111' (8-bit) value
				return pmt()[7];
			}

			// PCR_PID
			inline uint16_t pcr_pid() const
			{	// '0001' '1111' '1111' '1111' (13-bit) value
				return ((pmt()[8] << 8) | (pmt()[9])) & 0x1FFF;
			}

			// program_info_length
			inline uint16_t program_info_length() const
			{	// '0000' '1111' '1111' '1111' (12-bit) value
				return ((pmt()[10] << 8) | (pmt()[11])) & 0x0FFF;
			}

			// get array of 'pes (elementary) pid' / 'stream type'
			const std::vector<std::pair<pid_t, type_t>> get_pes_pids() const;

			// 0x1B, 101 ITU_T_H264 ( http://msdn.microsoft.com/en-us/library/windows/hardware/ff567738(v=vs.85).aspx )
			// 0x0F, 100 ISO/IEC 13818-7 AUDIO with ADTS transport syntax (ISO/IEC 13818-1 p.48)
		};
	}
}

#endif