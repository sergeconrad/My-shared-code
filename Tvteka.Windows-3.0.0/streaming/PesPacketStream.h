/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

//
// NOT USED - DEPRECATED
//

#ifndef HLS_STREAMING_PES_PACKET_STREAM_H
#define HLS_STREAMING_PES_PACKET_STREAM_H

#include "PesPacket.h"

#include "core/debug_message.h"
#include "core/segment_stream.h"

#include "mpeg2/ts/pes_packet_stream.h"

#include <memory>

using namespace mpeg2::ts;

namespace hls {
	namespace streaming {
		
		public ref class PesPacketStream sealed
		{
			static int DESTROYS;

		internal:
			PesPacketStream(pes_packet_stream* pes_stream)
				: pes_stream_(pes_stream)
			{}
		
		public:
			virtual ~PesPacketStream()
			{
				++DESTROYS;
				DEBUG_MESSAGE("PesPacketStream::~PesPacketStream() DESTROYED: " << DESTROYS);
				pes_stream_.reset();
			}

		public:
			PesPacket^ GetNextPesPacket()
			{
				PesPacket^ packet = nullptr;				
				pes_packet* pes_pack = pes_stream_->get_next_pes_packet();

				if (pes_pack != nullptr) {
					packet = ref new PesPacket(pes_pack);
				}				
				return packet;
			}

			// number of pes_packets in segment packet stream
			property size_t Count
			{
				size_t get() { return pes_stream_->count(); }
			}
			
			static property int Destroys
			{
				int get() { return DESTROYS; }
			}

		private:
			std::unique_ptr<pes_packet_stream> pes_stream_;
		};

	}
}

#endif