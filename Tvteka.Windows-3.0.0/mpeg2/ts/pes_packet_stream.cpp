/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "pch.h"
#include "pes_packet_stream.h"
#include "aac_adts_header.h"

using namespace mpeg2::ts;

pes_packet_stream::pes_packet_stream(int pid, const pes_stream_type& type)
	: pid_(pid), type_(&type)
	, packets_(std::unique_ptr<stream_storage, debug_deleter>(new stream_storage()))
{}

// move copy constructor
pes_packet_stream::pes_packet_stream(pes_packet_stream&& stream)
	: pid_(stream.pid_), type_(stream.type_)
	, packets_(std::move(stream.packets_))
{
}

pes_packet_stream& // move assignment
pes_packet_stream::operator=(pes_packet_stream&& stream)
{
	if (this != &stream)
	{
		pid_ = stream.pid_;
		type_ = stream.type_;
		packets_ = std::move(stream.packets_);
	}
	return *this;
}

void 
pes_packet_stream::add(pes_packet* packet)
{
	packets_->push_back(std::unique_ptr<pes_packet>(packet));
}

pes_packet*
pes_packet_stream::get_next_pes_packet()
{
	pes_packet* packet = nullptr;

	if (packets_->size() != 0) 
	{
		packet = packets_->front().release();
		packets_->pop_front();
	}	
	return packet;
}

audio_stream_params
pes_packet_stream::get_audio_stream_params() const
{
	if ((packets_->size() != 0) && (stream_type().type() == media::type::AUDIO)) 
	{
		auto& packet = packets_->at(0);
		aac_adts_header* adts = new(const_cast<uint8_t*>(packet->es_data()))aac_adts_header();
		
		uint16_t sw = adts->syncword();
		uint16_t pf = adts->profile();

		uint8_t frequency_index = adts->sampling_frequency_index();
		uint8_t channel_config = adts->channel_configuration();
		
		uint32_t frequency = audio_stream_params::get_frequency(frequency_index);
		uint8_t channels = audio_stream_params::get_channels(channel_config);

		return audio_stream_params(frequency, channels);
	}
	return audio_stream_params();
}

const pes_stream_type& 
pes_packet_stream::stream_type() const
{
	return *type_;
}

std::size_t 
pes_packet_stream::count() const // PES packets count in the queue
{
	return packets_->size();
}
