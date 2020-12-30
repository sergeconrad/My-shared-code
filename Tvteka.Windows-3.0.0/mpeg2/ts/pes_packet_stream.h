/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef MPEG2_PES_PACKET_STREAM_H
#define MPEG2_PES_PACKET_STREAM_H

#include "debug_message.h"

#include "pes_packet.h"
#include "pes_stream_types.h"
#include "audio_stream_params.h"

#include <cstddef>
#include <memory>
#include <queue>

//  warning C4251: class 'M' needs to have dll-interface to be used by clients of class 'DLL'
// http://support2.microsoft.com/default.aspx?scid=KB;EN-US;168958
// 'you must export the instantiation of the STL class that you use to create the data member'
//  example: template class __declspec(dllexport) std::unique_ptr < name >;

#pragma warning(push)
#pragma warning(disable:4251)

namespace mpeg2 {
	namespace ts {

		class __declspec(dllexport) pes_packet_stream
		{
		private:
			// storage type for pes packets
			typedef std::deque<std::unique_ptr<pes_packet>> stream_storage;

			struct debug_deleter
			{
				void operator() (stream_storage* p)
				{
					//DEBUG_MESSAGE("pes_packet_stream::stream_storage at [0x" << p << "] deleted");
					delete p;
				}
			};
			
			// delete standard construct/copy
			pes_packet_stream(const pes_packet_stream&); // = delete;
			pes_packet_stream& operator=(const pes_packet_stream&); // = delete;

		public:
			pes_packet_stream(int pid, const pes_stream_type& type);
			~pes_packet_stream() {}

			// move copy constructor
			pes_packet_stream(pes_packet_stream&& stream);
			// move assignment (don't copy _queue)
			pes_packet_stream& operator=(pes_packet_stream&& stream);

		public:
			// add PES packet to back of the queue
			void add(pes_packet*);

			// get PES packet of the front of queue
			pes_packet* get_next_pes_packet();

			audio_stream_params get_audio_stream_params() const;

			// stream type
			const pes_stream_type&  stream_type() const;

			// PES packets count in the queue
			std::size_t count() const;

		private:
			int pid_; // pid of the stream            
			const pes_stream_type* type_; // point to static pes_stream_type (don't try to delete)

			// packets storage
			std::unique_ptr<stream_storage, debug_deleter> packets_;

		};

	}
}

#pragma warning(pop)
#endif