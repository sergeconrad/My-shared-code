/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef MPEG2_TS_PAT_PACKET_H
#define MPEG2_TS_PAT_PACKET_H

#include "ts_packet.h"
#include <vector>

namespace mpeg2 {
	namespace ts {

		// ISO/IEC 13818-1 : 2000 (E) 
		// p.43 Table 2-25 – Program association section

		class ts_pat_packet : public ts_packet
		{
			// Get pointer to the Program association table
			inline const uint8_t* pat() const
			{
				return section_data();
			}

		public:
			ts_pat_packet() {}  // = delete
			~ts_pat_packet() {} // = delete
			ts_pat_packet(const ts_pat_packet&) {} // = delete
			ts_pat_packet& operator=(const ts_pat_packet&) {} // = delete

			inline uint8_t table_id() const
			{	// '1111' '1111' (8-bit) value
				return pat()[0];
			}

			inline bool section_syntax_indicator() const
			{   // '1000' '0000' (1-bit) flag
				return (pat()[1] & 0x80) != 0;
			}

			inline uint16_t section_length() const
			{	// '0000' '1111' '1111' '1111' (12-bit) value
				return ((pat()[1] << 8) | (pat()[2])) & 0x0FFF;
			}

			inline uint16_t transport_stream_id() const
			{	// '1111' '1111' '1111' '1111' (16-bit) value
				return ((pat()[3] << 8) | (pat()[4]));
			}

			inline uint8_t version_number() const
			{	// '0011' '1110' (5-bit) value
				return (pat()[5] & 0x3E) >> 1;
			}

			inline bool current_next_indicator() const
			{	// '0000' '0001' (i-bit) flag
				return (pat()[5] & 0x01) != 0;
			}

			inline uint8_t section_number() const
			{	// '1111' '1111' (8-bit) value
				return pat()[6];
			}

			inline uint8_t last_section_number() const
			{	// '1111' '1111' (8-bit) value
				return pat()[7];
			}

			// get array of 'program id','pmt pid' pairs            
			const std::vector<std::pair<program_t, pid_t>> get_programs_table() const;
		};
	}
}

#endif