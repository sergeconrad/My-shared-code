/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef MPEG2_TS_PES_PACKET_H
#define MPEG2_TS_PES_PACKET_H

#include "ts_packet.h"

namespace mpeg2 {
	namespace ts {

		// ISO/IEC 13818-1 : 2000 (E)
		// p.31 Table 2-17 – PES packet

		class ts_pes_packet : public ts_packet
		{
			// get pointer to pes header
			inline const uint8_t* pes() const
			{
				const uint8_t* pes_p = packet_ + HEADER_SIZE;
				pes_p += has_adaptation_field() ? (packet_[ADAPTATION_FIELD_OFFSET] + 1) : 0;
				return pes_p;
			}
		public:
			ts_pes_packet() {}  // = delete
			~ts_pes_packet() {} // = delete
			ts_pes_packet(const ts_pes_packet&) {} // = delete
			ts_pes_packet& operator=(const ts_pes_packet&) {} // = delete

			// pes packet header fields

			// for pes packets it must be '0x000001'
			inline uint32_t packet_start_code_prefix() const
			{	// '1111' '1111' '1111' '1111' '1111' '1111' (24-bit) value
				return (pes()[0] << 16) | (pes()[1] << 8) | pes()[2];
			}

			inline uint8_t stream_id() const
			{	// '1111' '1111' (8-bit) value
				return pes()[3];
			}

			inline uint16_t pes_packet_lenght() const
			{	// '1111' '1111' '1111' '1111' (16-bit) value
				return (pes()[4] << 8) | pes()[5];
			}

			inline uint8_t pts_dts_flags() const
			{
				// '1100' '0000' (2-bit) value
				return (pes()[7] & 0xC0) >> 6;
			}

			inline int pes_header_data_length() const
			{	// '1111' '1111' (8-bit) valie
				return pes()[8];
			}

			// helpers

			inline bool has_pts() const
			{	// '1000' '0000'
				return (pes()[7] & 0x80) != 0;
			}

			inline bool has_dts() const
			{
				// '0100' '0000'
				return (pes()[7] & 0x40) != 0;
			}

			// PTS in milliseconds
			uint64_t pts() const;

			// is audio packet
			bool is_audio() const;

			// is video packet
			bool is_video() const;

			uint8_t stream_number() const;

			// ES data functons

			// complete ES frame length
			uint16_t es_packet_length() const;

			// point to start ES data in the TS packet
			const uint8_t* es_data_start() const;

			// Length of ES data in the TS packet
			int es_data_length() const;

			// Private
			// Applicable to H.264 (video) pes packets only
			//int GetNalUnitType(const unsigned char*) const;

			//const unsigned char* GetNextNalUnit(const unsigned char* nal_unit_ptr) const;

			// Get pointer to and length of CodecPrivateDate in TsPesPacket
			//std::pair<const unsigned char*, size_t>
			//    GetCodecPrivateData() const;
		};
	}
}

#endif