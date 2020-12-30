/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef HLS_STREAMING_START_SEGMENT_H
#define HLS_STREAMING_START_SEGMENT_H

namespace hls {
	namespace streaming {

		public ref class StartSegmentInfo sealed
		{
		internal:
			StartSegmentInfo(int segment_index, double start_time)
				: segment_index_(segment_index)
				, start_time_(start_time)
			{}

		public:
			property int SegmentIndex
			{
				int get()
				{
					return segment_index_;
				}
			}
			property double StartTime
			{
				double get()
				{
					return start_time_;
				}
			}

		private:
			int segment_index_; // from 0
			double start_time_; // in seconds

		};
	}
}

#endif

