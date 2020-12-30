/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef MPEG2_TS_AAC_ADTS_HEADER_H
#define MPEG2_TS_AAC_ADTS_HEADER_H

#include <cstddef>
#include <cstdint>

namespace mpeg2 {
	namespace ts {
		
		// ISO/IEC 13818-7:2006(E)

		class aac_adts_header
		{
			uint8_t header_[7]; // 56 bit fixed [0-27] + variable [28-55] headers

		private:
			aac_adts_header(const aac_adts_header&);
			aac_adts_header& operator=(const aac_adts_header&);

		public:
			aac_adts_header() {}
			~aac_adts_header() {}

			void* operator new(std::size_t size, uint8_t* header){ return header; }
				void operator delete(void* m, uint8_t* h) {}

			inline uint16_t syncword() const
			{
				return ((header_[0] << 8) | (header_[1])) >> 4;
			}

			inline uint8_t id() const
			{	// '0000' '1000'
				return ((header_[1] & 0x08) >> 3);
			}

			inline uint8_t layer() const
			{	// '0000' '0110'
				return ((header_[1] & 0x06) >> 1);
			}

			inline uint8_t protection_absent() const
			{	// '0000' '0001'
				return (header_[1] & 0x01);
			}

			/* Table 31 — Profiles index profile
				0 Main profile
				1 Low Complexity profile(LC)
				2 Scalable Sampling Rate profile(SSR)
				3 (reserved)
			*/
			inline uint16_t profile() const
			{
				return ((header_[2] & 0xC0) >> 6);
			}

			/* Table 35 — Sampling frequency dependent on sampling_frequency_index
				sampling_frequency_index sampling frequeny[Hz]
				0x0	96000
				0x1	88200
				0x2 64000
				0x3 48000
				0x4 44100
				0x5 32000
				0x6 24000
				0x7 22050
				0x8 16000
				0x9 12000
				0xa 11025
				0xb 8000
				0xc reserved
				0xd reserved
				0xe reserved
				0xf reserved
			*/
			inline uint8_t sampling_frequency_index() const
			{ 
				return ((header_[2] & 0x3C) >> 2); 
			}

			// Table 42 — Channel Configuration
			// Number of channels 
			inline uint8_t channel_configuration() const
			{
				return ((header_[2] & 0x01) << 2) | ((header_[3] & 0xC0) >> 6);
			}
		};
	}
}

#endif
