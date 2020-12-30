/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef HLS_STREAMING_PES_SEGMENT_STREAM_H
#define HLS_STREAMING_PES_SEGMENT_STREAM_H

#include "PesPacket.h"

#include "core/debug_message.h"
#include "core/segment_stream.h"

#include <memory>

namespace hls {
	namespace streaming {

		public ref class PesSegmentStream sealed
		{
			static int DESTROYS;

		internal:
			PesSegmentStream(segment_stream* seg_stream)
				: segment_stream_(seg_stream)
			{}

		public:
			virtual ~PesSegmentStream()
			{
				++DESTROYS;
				//DEBUG_MESSAGE("PesSegmentStream::~PesSegmentStream() DESTROYED: " << DESTROYS);
			}

		public:
			// Segment Info
			property bool IsDiscontinuous
			{
				bool get()
				{
					return segment_stream_->discontinuity();
				}
			}
			property int Duration
			{
				int get()
				{
					return segment_stream_->duration();
				}
			}
			property String^ FileName
			{
				String^ get()
				{
					return ref new String(segment_stream_->file_name().c_str());
				}
			}

			// Stream Data
			PesPacket^ GetNextPesPacket()
			{
				PesPacket^ packet = nullptr;
				pes_packet* pes_pack = segment_stream_->get_next_pes_packet();

				if (pes_pack != nullptr) {
					packet = ref new PesPacket(pes_pack);
				}
				return packet;
			}

			property size_t Count
			{
				size_t get()
				{
					return segment_stream_->packets_count();
				}
			}

		private:
			std::unique_ptr<segment_stream> segment_stream_;
		};

	}
}

#endif
