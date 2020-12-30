/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "pch.h"
#include "audio_stream_params.h"

using namespace mpeg2::ts;

uint32_t 
audio_stream_params::frequency_table_[] = {
	96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 0, 0, 0, 0
};

uint32_t
audio_stream_params::get_frequency(uint8_t frequency_index)
{
	return frequency_table_[frequency_index];
}

uint8_t
audio_stream_params::get_channels(uint8_t channel_config)
{
	return channel_config;
}