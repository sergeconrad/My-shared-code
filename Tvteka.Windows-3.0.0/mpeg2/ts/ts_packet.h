/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef MPEG2_TS_PACKET_H
#define MPEG2_TS_PACKET_H

#include "mpeg2_ts.h"

namespace mpeg2 {
	namespace ts {

		class ts_packet
		{
			// ISO/IEC 13818-1 : 2000 (E)
			// p.18 Table 2-2 – Transport packet

		protected:
			uint8_t packet_[TS_PACKET_SIZE];

		public:
			ts_packet() {}
			~ts_packet() {} // = delete;

			// don't copy
			ts_packet(const ts_packet&) {} // = delete;
			ts_packet& operator=(const ts_packet&) {} // = delete;

			// new() placement operator (map ts_packet on memory)
			void* operator new(size_t sz, uint8_t* mem){ return mem; }
				void operator delete(void* m, uint8_t* pos) {}

			// Transport packet header fields           
			inline uint8_t sync_byte() const
			{
				return packet_[0];
			}

			inline bool transport_error_indicator() const
			{   // '1000 0000' flag
				return (packet_[1] & 0x80) != 0;
			}

			inline bool payload_unit_start_indicator() const
			{   // '0100 0000' flag
				return (packet_[1] & 0x40) != 0;
			}

			inline bool transport_priority() const
			{   // '0010 0000' flag
				return (packet_[1] & 0x20) != 0;
			}

			inline uint16_t pid() const
			{   // '0001 1111' '1111 1111' (13-bit) value
				return ((packet_[1] << 8) | packet_[2]) & 0x1FFF;
			}

			inline uint8_t transport_scrambling_control() const
			{   // '1100 0000' (2-bit) value
				return (packet_[3] & 0xC0) >> 6;
			}

			inline uint8_t adaptation_field_control() const
			{   // '0011 0000' (2-bit) value
				return (packet_[3] & 0x30) >> 4;
			}

			inline uint8_t continuity_counter() const
			{   // '0000 1111' (4-bit) value
				return (packet_[3] & 0x0F);
			}

			// Helper functions

			inline bool has_payload() const
			{   // '0001 0000' 
				return (packet_[3] & 0x10) != 0;
			}

			inline bool has_adaptation_field() const
			{   // '0010 0000'
				return (packet_[3] & 0x20) != 0;
			}

			inline uint16_t adaptation_field_length() const
			{
				//return has_adaptation_field() ? packet_[ADAPTATION_FIELD_OFFSET] : 0; // ORIGINAL 2.0.1
				
				// From 2.0.2-dev 28/01/2015
				//int afc = adaptation_field_control();
				if (adaptation_field_control() == 0x10) // 2 = 0x10 - adaptation field, no payload
				{
					return 183;
				}
				//bool af = has_adaptation_field();
				return has_adaptation_field() ? packet_[ADAPTATION_FIELD_OFFSET] : 0;
			}

			// Get pointer to section (PAT, PMT, ... PSI data)
			inline const uint8_t* section_data() const
			{
				/* original 2.0.1
				int pointer_field_offset = HEADER_SIZE + adaptation_field_length();
				int section_offset = pointer_field_offset + packet_[pointer_field_offset] + 1;
				return packet_ + section_offset;
				*/

				// From 2.0.2-dev 28/01/2015
				int pointer_field_offset = HEADER_SIZE + adaptation_field_length();
				int section_offset = pointer_field_offset;

				if (payload_unit_start_indicator())
				{
					if (has_adaptation_field())
					{
						pointer_field_offset += 1;// +1 25/01/2015 (1 is one byte of adaptation_field_length)
					}
					section_offset = pointer_field_offset + packet_[pointer_field_offset] + 1;
				}
				return (packet_ + section_offset);
			}
		};
	}
}

#endif