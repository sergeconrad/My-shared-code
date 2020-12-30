/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef HLS_STREAMING_AUDIO_STREAM_PARAMS_H
#define HLS_STREAMING_AUDIO_STREAM_PARAMS_H

#include "mpeg2/ts/audio_stream_params.h"

using namespace mpeg2::ts;

namespace hls {
	namespace streaming {

		public ref class AudioStreamParams sealed
		{
		internal:
			AudioStreamParams(mpeg2::ts::audio_stream_params as_params)
				:audio_params_(as_params)
			{}

		public:
			uint32 Frequency()
			{
				return audio_params_.frequency();
			}
			uint32 Channels()
			{
				return audio_params_.channels();
			}

		private:
			audio_stream_params audio_params_;
		};
	}
}
#endif