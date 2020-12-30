/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "pch.h"
#include "ts_splitter.h"

#include "debug_message.h"
#include "ts_reader.h"
#include "ts_packet.h"
#include "ts_pat_packet.h"
#include "ts_pmt_packet.h"
#include "ts_pes_packet.h"
#include "pes_stream_types.h"
#include "pes_packet_stream.h"

using namespace mpeg2::ts;

ts_splitter::ts_splitter()
	: current_program_(0)
{
	handlers_[PAT_PID] = &ts_splitter::handle_pat_packet;
	handlers_[CAT_PID] = &ts_splitter::handle_cat_packet;
	handlers_[TSD_PID] = &ts_splitter::handle_tsd_packet;
	handlers_[NUL_PID] = &ts_splitter::handle_nul_packet;
}

ts_splitter::~ts_splitter()
{
	DEBUG_MESSAGE("ts_splitter::~ts_splitter()");

	// free memory of pes streams not taken by client
	for (auto& i : pes_streams_)
	{
		pes_packet_stream* stream = i.second.first;
		if (stream != nullptr) 
		{
			DEBUG_MESSAGE("ts_splitter::~ts_splitter(): delete pes_packet_stream: type: " << (int)stream->stream_type().type());
			delete stream;
		}
		pes_packet* packet = i.second.second;
		if (packet != nullptr)
		{
			DEBUG_MESSAGE("ts_splitter::~ts_splitter(): delete pes_packet: " << (packet->is_audio() ? "audio" : "video"));
			delete packet;
		}
	}
}

void
ts_splitter::split_buffer(const uint8_t* begin, std::size_t length)
{
	//DEBUG_MESSAGE("ts_splitter::split_buffer(): " << begin << " : " << length << " bytes");

	ts_reader reader(begin, length);

	std::size_t count = 0;
	while (const ts_packet* packet = reader.read_next_packet())
	{
		uint16_t pid = packet->pid();
		//DEBUG_MESSAGE("ts_splitter::split_buffer(): #" << count << " pid " << pid);

		// find packet handler by the pid
		auto find = handlers_.find(pid);
		if (find != handlers_.end())
		{
			// set handler
			packet_handler handler = (*find).second;
			// call pid handler
			(this->*handler)(packet);
		}
		++count;
	}

	// final stage - add the last pes_packets to pes_streams
	for (auto& key_val : pes_streams_)
	{
		//pid_t pid = key_val.first;
		
		pes_packet_stream* pes_stream = key_val.second.first;
		pes_packet*& packet = key_val.second.second;
		
		if (packet != nullptr) {
			pes_stream->add(packet);
		}
		packet = nullptr;
	}

	//DEBUG_MESSAGE("ts_splitter::split_buffer(): " << count << " packets splitted");
	return;
}

pes_packet_stream*
ts_splitter::get_audio_stream()
{
	return get_pes_stream(media::type::AUDIO);
}

pes_packet_stream*
ts_splitter::get_video_stream()
{
	return get_pes_stream(media::type::VIDEO);
}

pes_packet_stream* 
ts_splitter::get_pes_stream(media::type type)
{
	pes_packet_stream* stream = nullptr;

	if (current_program_ != 0)
	{
		for (const auto& pid : program_pes_pids_[current_program_])
		{
			auto stream_type = pid_program_types_[pid].second;

			if (stream_type->type() == type && stream_type->is_supported() == media::supported::YES)
			{
				// stream to return 
				stream = pes_streams_[pid].first;

				// Set to NULL to not delete it in destructor
				pes_streams_[pid].first = nullptr;

				// Taken the first audio/video stream, don't lookup anymore
				break;
			}
		}
	}
	return stream;
}

// handle PAT TS packets with pid 0x00 (PAT)
// add found pmt pids to the packet handlers map
void
ts_splitter::handle_pat_packet(const ts_packet* packet)
{
	//DEBUG_MESSAGE("ts_splitter::split_buffer(): pat packet pid " << packet->pid());
	// TS Packet is PAT packet
	ts_pat_packet* pat_packet = (ts_pat_packet*)packet;

	// get 'program_id' PMT PIDs
	const std::vector<std::pair<program_t, pid_t>> pmt_table = pat_packet->get_programs_table();
	//DEBUG_MESSAGE("ts_splitter::handle_pat_packet(): " << pmt_table.size() << " program(s) found");

	if (pmt_table.size() != 0)
	{
		// set current program in the first pat only
		if (current_program_ == 0) {
			// Initially split streams of the first program in ts stream
			current_program_ = pmt_table[0].first;
		}

		// add ALL programs PMT PIDs to the packet handler map
		// store all programs pes streams
		for (auto& pmt : pmt_table)
		{
			program_t program_number = pmt.first;
			pid_t pmt_pid = pmt.second;

			//DEBUG_MESSAGE("ts_splitter::handle_pat_packet(): program " 
			//	<< program_number << " associated with PMT PID " << pmt_pid);

			// bind PMT PID to handler
			if (program_number != 0)
			{
				// bind PMT handler to the PID (add new or change existing)
				handlers_[pmt_pid] = &ts_splitter::handle_pmt_packet;
			}
			else
			{
				// Bind NIT Handler to the PID 
				handlers_[pmt_pid] = &ts_splitter::handle_nit_packet;
			}
		}
	}
	else
	{
		DEBUG_MESSAGE("ts_splitter::handle_pat_packet(): no programs found");
	}
	return;
}

void
ts_splitter::handle_pmt_packet(const ts_packet* packet)
{
	//DEBUG_MESSAGE("ts_splitter::split_buffer(): pmt packet pid " << packet->pid());

	ts_pmt_packet* pmt_packet = (ts_pmt_packet*)packet;

	program_t program_number = pmt_packet->program_number();
	uint16_t pcr_pid = pmt_packet->pcr_pid();

	// add all stream pids for processing
	for (const auto& item : pmt_packet->get_pes_pids())
	{
		pid_t pes_pid = item.first;
		type_t pes_type = item.second;

		// check if pid already in program table
		auto& pid_vec = program_pes_pids_[program_number];
		auto pid_found = std::find(pid_vec.begin(), pid_vec.end(), pes_pid);

		// add only new pes pid to the program 
		if (pid_found == pid_vec.end()) {
			program_pes_pids_[program_number].push_back(pes_pid);
		}

		// get stream type for the PID
		const pes_stream_type& stream_type = pes_stream_types::lookup(pes_type);

		// add the pid to types table
		pid_program_types_[pes_pid] = std::make_pair(program_number, &stream_type);
	
		//DEBUG_MESSAGE("ts_splitter::handle_pmt_packet(): pes pid " << pes_pid 
		//	<< " program " << program_number
		//	<< " type 0x" << std::hex << std::setfill(L'0') << std::setw(2) << pes_type
		//	<< " " << stream_type.notes() << " found");
		
		// bind PES handler to the PID
		handlers_[pes_pid] = &ts_splitter::handle_pes_packet;

		//DEBUG_MESSAGE("ts_splitter::handle_pmt_packet(): handler for " << pes_pid << " added to program " << program_number);
		
		// create pes_stream for the PID (initially local PesPacket is null)
		if (pes_streams_[pes_pid].first == nullptr) {
			pes_streams_[pes_pid] = std::make_pair(new pes_packet_stream(pes_pid, stream_type), nullptr);
		}
	}
	return;
}

void
ts_splitter::handle_pes_packet(const ts_packet* packet)
{
	//DEBUG_MESSAGE("ts_splitter::split_buffer(): pes packet pid " << packet->pid());
	ts_pes_packet* ts_pes_pack = (ts_pes_packet*)packet;

	pid_t pid = packet->pid();

	// get program number of the pid
	program_t program = pid_program_types_[pid].first;

	// process pid only for current program
	if (program == current_program_)
	{
		// get stream type of pes stream
		auto type = pid_program_types_[pid].second;

		// take supported audio/video streams only
		// if ((type->IsSupported() == Media::Supported::Yes)
		//    && (type->Type() == Media::Type::Audio || type->Type() == Media::Type::Video)) {

		// take ALL streams of the current program

		// pes_packet_stream*, pes_packet* pair
		auto& pes_stream_data = pes_streams_[pid];

		// get pes_packet_stream for the pid
		pes_packet_stream* pes_stream = pes_stream_data.first;

		// get pes_packet for the pes_packet_stream 
		// (& ref. because we will change value in the container)
		pes_packet*& pes_pack = pes_stream_data.second;

		// new pes_packet arrived
		if (ts_pes_pack->payload_unit_start_indicator())
		{
			if (pes_pack != nullptr)
			{
				// add to the pes stream completed pes packet
				pes_stream->add(pes_pack);

				//DEBUG_MESSAGE("ts_splitter::handle_pes_packet(): added full pes packet: " << packet->pid() 
				//	<< " es_length=" << pes_pack->es_length());
			}
			// start new pes packet
			pes_pack = new pes_packet(ts_pes_pack);

			//DEBUG_MESSAGE("ts_splitter::handle_pes_packet(): created new  pes packet: " << packet->pid() 
			//	<< " es_length " << ts_pes_pack->es_packet_length());
		}
		// existing pes packet segment
		else
		{
			if (pes_pack != nullptr)
			{
				// Add es stream data to the PesPacket
				pes_pack->add(ts_pes_pack);
			}
		}
	}
	return;

}

void
ts_splitter::handle_cat_packet(const ts_packet* packet)
{
	DEBUG_MESSAGE("ts_splitter::split_buffer(): cat packet pid " << packet->pid());
}

void
ts_splitter::handle_tsd_packet(const ts_packet* packet)
{
	DEBUG_MESSAGE("ts_splitter::split_buffer(): tsd packet pid " << packet->pid());
}

void
ts_splitter::handle_nit_packet(const ts_packet* packet)
{
	DEBUG_MESSAGE("ts_splitter::split_buffer(): nit packet pid " << packet->pid());
}

void
ts_splitter::handle_nul_packet(const ts_packet* packet)
{
	DEBUG_MESSAGE("ts_splitter::split_buffer(): nul packet pid " << packet->pid());
}