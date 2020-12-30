/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef MPEG2_PES_PACKET_H
#define MPEG2_PES_PACKET_H

#include "memory_buffer.h"

#include <cstddef>
#include <cstdint>

//  warning C4251: class 'M' needs to have dll-interface to be used by clients of class 'DLL'
// http://support2.microsoft.com/default.aspx?scid=KB;EN-US;168958
// 'you must export the instantiation of the STL class that you use to create the data member'
//  example: template class __declspec(dllexport) std::unique_ptr < name >;

#pragma warning(push)
#pragma warning(disable:4251)

namespace mpeg2 {
	namespace ts {

		class ts_pes_packet;

		class __declspec(dllexport) pes_packet
		{
			// expandable array when ES packet length is 0
			memory_buffer es_buffer_;

			// actully written bytes count
			std::size_t written_bytes_;

			// ts pes packet holds header only
			ts_pes_packet* header_;
			uint8_t header_sz_;
			
			pes_packet(const pes_packet&);
			pes_packet& operator=(const pes_packet&);

		public:
			pes_packet(const ts_pes_packet*);
			~pes_packet();

			// add PES segment packet data
			void add(const ts_pes_packet*);

			// ES packet size
			std::size_t es_length() const;
			const uint8_t* es_data() const;

			// full PES packet length (header + ES data)
			std::size_t packet_length() const;

			// PTS from TS PES Header
			uint64_t pts() const;

			// type of media data in the PES packet
			bool is_audio() const;
			bool is_video() const;

		};
	}
}

#pragma warning(pop)
#endif
