/****************************** Module Header ******************************\
Copyright (c) 2015 Serge Conrad.
All other rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef HLS_STREAMER_START_SEGMENT_INFO_H
#define HLS_STREAMER_START_SEGMENT_INFO_H

namespace hls {
	namespace streamer {

		class start_segment_info
		{
			int segment_index_; // From 0
			long start_time_;	// Seconds
		
		public:
			start_segment_info(int segment_index, long start_time)
				: segment_index_(segment_index)
				, start_time_(start_time)
			{}

			int index() const { return segment_index_; }
			long time() const { return start_time_; }
		};
	}
}

#endif