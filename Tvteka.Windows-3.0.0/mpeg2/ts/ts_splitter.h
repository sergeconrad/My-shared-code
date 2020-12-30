/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef MPEG2_TS_SPLITTER_H
#define MPEG2_TS_SPLITTER_H

#include "mpeg2_ts.h"
#include "pes_packet_stream.h"

#include <map>
#include <vector>

//  warning C4251: class needs to have dll-interface to be used by clients of class 'DLL'
// http://support2.microsoft.com/default.aspx?scid=KB;EN-US;168958
// 'you must export the instantiation of the STL class that you use to create the data member'
//  example: template class __declspec(dllexport) std::unique_ptr < name >;

#pragma warning(push)
#pragma warning(disable:4251)

namespace mpeg2 {
	namespace ts {

		class ts_packet;
		class pes_packet;
		class pes_stream_type;

		class __declspec(dllexport) ts_splitter
		{
		public:
			ts_splitter();
			~ts_splitter();

			void split_buffer(const uint8_t* begin, std::size_t length);

			pes_packet_stream* get_audio_stream();
			pes_packet_stream* get_video_stream();

		private:
			// packet handler functon pointer, and handlers map type definitions
			typedef void (ts_splitter::*packet_handler)(const ts_packet*);
			typedef std::map<pid_t, packet_handler> packet_handler_map;

		private:
			packet_handler_map handlers_;

			// TS PSI packet handlers (packet_handler type)
			void handle_pat_packet(const ts_packet*); // 0x0000 PAT Packet Association Table
			void handle_pmt_packet(const ts_packet*); // 0xXXXX PMT Program Map Table - Assignment indicated in the PAT
			void handle_cat_packet(const ts_packet*); // 0x0001 CAT	Conditional Access Table
			void handle_tsd_packet(const ts_packet*); // 0x0002 TSD Transport Stream Description Table
			void handle_nit_packet(const ts_packet*); // 0xXXXX NIT Network Information Table - Assignment indicated in the PAT
			void handle_nul_packet(const ts_packet*); // 0x1FFF NULL packet

			// TS PES packet handler (packet_handler type)
			void handle_pes_packet(const ts_packet*); // 0xXX PES packet PID indicated in the PMT				

		private:
			pes_packet_stream* get_pes_stream(media::type);

		private:
			program_t current_program_;
			std::map<program_t, std::vector<pid_t>> program_pes_pids_; // 'program' -> 'pes pids'			
			std::map<pid_t, std::pair<program_t, const pes_stream_type*>> pid_program_types_; // 'pid' -> 'program number','stream type' 
			std::map<pid_t, std::pair<pes_packet_stream*, pes_packet*>> pes_streams_; // 'pid' -> 'pes stream', 'local storage' pes_packet is used to assemble full pes_packet
		};
	}
}


#pragma warning(pop)
#endif