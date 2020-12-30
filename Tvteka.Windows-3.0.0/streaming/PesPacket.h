/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef HLS_STREAMING_PES_PACKET_H
#define HLS_STREAMING_PES_PACKET_H

#include "core/debug_message.h"
#include "mpeg2/ts/pes_packet.h"
#include <memory>
#include <string>

using namespace mpeg2::ts;

using namespace Platform;
using namespace Windows::Storage::Streams;

namespace hls {
	namespace streaming {

		public ref class PesPacket sealed
		{
		internal:
			PesPacket(pes_packet* packet)
				: packet_(packet)
			{}

		public:
			// get es packet data length
			size_t GetEsPacketLength()
			{
				return packet_->es_length();
			}

			// get PTS
			uint64 Pts()
			{
				return packet_->pts();
			}
			
			// get raw elemenary stream data
			Array<uint8>^ GetEsPacketData()
			{
				Array<uint8>^ es_packet = ref new Array<uint8>(const_cast<uint8_t*>(packet_->es_data()), packet_->es_length());				
				return es_packet;
			}

			IBuffer^ GetSampleBuffer()
			{
				IBuffer^ buffer = nullptr;
				const Array<uint8_t>^ data = nullptr;
				DataWriter^ writer = nullptr;

				try 
				{
					data = ref new Array<uint8_t>(const_cast<uint8_t*>(packet_->es_data()), packet_->es_length());
					writer = ref new DataWriter();					
					writer->WriteBytes(data);
				}
				catch (const std::exception& e)
				{
					DEBUG_MESSAGE("PesPacket::GetSampleBuffer(): Exception: " << e.what());
					std::string m = e.what();
				}

				buffer = writer->DetachBuffer();
				
				delete data;
				delete writer;
				
				return buffer;
			}

		private:
			std::unique_ptr<const pes_packet> packet_;
		};
	}
}

#endif