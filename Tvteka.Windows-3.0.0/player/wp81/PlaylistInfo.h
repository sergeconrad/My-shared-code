/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

PlaylistInfo HLS Streamer

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#ifndef HLS_PLAYER_PLAYLIST_INFO_H
#define HLS_PLAYER_PLAYLIST_INFO_H

#include "hls/streamer/playlist_info.h"

using namespace Platform;
using namespace hls::streamer;

namespace hls {
	namespace player {

		public ref class PlaylistInfo sealed
		{
			playlist_info playlist_inf_;

		internal:
			PlaylistInfo(const playlist_info& pi)
				: playlist_inf_(pi)
			{}

		public:
			int ProgramId() { return playlist_inf_.program_id(); }
			int Bandwidth() { return playlist_inf_.bandwidth();  }
			String^ Url() { return ref new String(playlist_inf_.url().c_str()); }
		};

	}
}

#endif