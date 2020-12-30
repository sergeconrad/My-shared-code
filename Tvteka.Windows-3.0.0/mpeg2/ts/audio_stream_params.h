/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef MPEG2_TS_AUDIO_STREAM_INFO_H
#define MPEG2_TS_AUDIO_STREAM_INFO_H

#include <cstdint>

namespace mpeg2 {
	namespace ts {

		// ISO/IEC 13818-7:2006(E)

		class audio_stream_params
		{
			static uint32_t frequency_table_[16];
		public:
			static uint32_t get_frequency(uint8_t frequency_index);
			static uint8_t  get_channels(uint8_t channel_config);

		public:
			audio_stream_params()
				: frequency_(0)
				, channels_(0)
			{}
			audio_stream_params(uint32_t frequency, uint8_t channels)
				: frequency_(frequency)
				, channels_(channels)
			{}
			audio_stream_params(const audio_stream_params& other)
				: frequency_(other.frequency_)
				, channels_(other.channels_)
			{}
			audio_stream_params& operator=(const audio_stream_params& lh)
			{
				if (this != &lh) {
					frequency_ = lh.frequency_;					
					channels_ = lh.channels_;
				}
				return *this;
			}			

			uint32_t frequency() const
			{
				return frequency_;
			}

			uint8_t channels() const
			{
				return channels_;
			}

		private:			
			uint32_t frequency_;
			uint8_t channels_;
		};

	}
}

#endif