/****************************** Module Header ******************************\
Copyright (c) 2014 Serge Conrad.
All other rights reserved.

MPEG2 TS Demultiplexor

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "pch.h"
#include "pes_stream_types.h"

using namespace mpeg2::ts;

std::map<type_t, pes_stream_type>
pes_stream_types::initialize_types()
{
	std::map<type_t, pes_stream_type> types;

	types[0x00] = pes_stream_type(0x00, media::type::UNKNOWN, media::supported::NO, L"Reserved");
	types[0x01] = pes_stream_type(0x01, media::type::VIDEO, media::supported::NO, L"MPEG-1 VIDEO (ISO_IEC_11172_2_VIDEO)");
	types[0x02] = pes_stream_type(0x02, media::type::VIDEO, media::supported::NO, L"MPEG-2 VIDEO (ISO_IEC_13818_2_VIDEO)");
	types[0x03] = pes_stream_type(0x04, media::type::AUDIO, media::supported::NO, L"MPEG-2 AUDIO (ISO_IEC_13818_3_AUDIO)");
	types[0x04] = pes_stream_type(0x04, media::type::AUDIO, media::supported::NO, L"MPEG-2 AUDIO (ISO_IEC_13818_3_AUDIO)");
	types[0x05] = pes_stream_type(0x05, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-1 private sections (ISO_IEC_13818_1_PRIVATE_SECTION)");
	types[0x06] = pes_stream_type(0x06, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-1 PES private data (ISO_IEC_13818_1_PES)");
	types[0x07] = pes_stream_type(0x07, media::type::UNKNOWN, media::supported::NO, L"ISO 13522 MHEG (ISO_IEC_13522_MHEG)");
	types[0x08] = pes_stream_type(0x08, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-1 DSM-CC (ANNEX_A_DSM_CC)");
	types[0x09] = pes_stream_type(0x09, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-1 auxiliary (ITU_T_REC_H_222_1)");
	types[0x0A] = pes_stream_type(0x0A, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-6 multi-protocol encap (ISO_IEC_13818_6_TYPE_A)");
	types[0x0B] = pes_stream_type(0x0B, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-6 DSM-CC U-N msgs (ISO_IEC_13818_6_type_B)");
	types[0x0C] = pes_stream_type(0x0C, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-6 stream descriptors (ISO_IEC_13818_6_TYPE_C");
	types[0x0D] = pes_stream_type(0x0D, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-6 sections (ISO_IEC_13818_6_TYPE_D)");
	types[0x0E] = pes_stream_type(0x0E, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-1 auxiliary (ISO_IEC_13818_1_AUXILIARY)");

	types[0x0F] = pes_stream_type(0x0F, media::type::AUDIO, media::supported::YES, L"MPEG-2 AAC AUDIO (ISO/IEC 13818-7 AUDIO with ADTS transport syntax)");

	types[0x10] = pes_stream_type(0x10, media::type::AUDIO, media::supported::NO, L"MPEG-4 VIDEO (ISO_IEC_14496_2_VISUAL)");
	types[0x11] = pes_stream_type(0x11, media::type::AUDIO, media::supported::NO, L"MPEG-4 LATM AAC AUDIO (ISO_IEC_14496_3_AUDIO)");
	types[0x12] = pes_stream_type(0x12, media::type::UNKNOWN, media::supported::NO, L"MPEG-4 generic (ISO_IEC_14496_1_IN_PES)");
	types[0x13] = pes_stream_type(0x13, media::type::UNKNOWN, media::supported::NO, L"ISO 14496-1 SL-packetized (ISO_IEC_14496_1_IN_SECTION)");
	types[0x14] = pes_stream_type(0x14, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-6 Synchronized Download Protocol (ISO_IEC_13818_6_DOWNLOAD)");
	types[0x15] = pes_stream_type(0x15, media::type::UNKNOWN, media::supported::NO, L"(METADATA_IN_PES)");
	types[0x16] = pes_stream_type(0x16, media::type::UNKNOWN, media::supported::NO, L"(METADATA_IN_SECTION)");
	types[0x17] = pes_stream_type(0x17, media::type::UNKNOWN, media::supported::NO, L"(METADATA_IN_DATA_CAROUSEL)");
	types[0x18] = pes_stream_type(0x18, media::type::UNKNOWN, media::supported::NO, L"(METADATA_IN_OBJECT_CAROUSEL)");
	types[0x19] = pes_stream_type(0x19, media::type::UNKNOWN, media::supported::NO, L"(METADATA_IN_DOWNLOAD_PROTOCOL)");
	types[0x1A] = pes_stream_type(0x1A, media::type::UNKNOWN, media::supported::NO, L"(IRPM_STREAMM)");

	types[0x1B] = pes_stream_type(0x1B, media::type::VIDEO, media::supported::YES, L"H.264 VIDEO (ITU_T_H264)");

	types[0x80] = pes_stream_type(0x80, media::type::VIDEO, media::supported::NO, L"DigiCipher II VIDEO (ISO_IEC_USER_PRIVATE)");
	types[0x81] = pes_stream_type(0x81, media::type::AUDIO, media::supported::NO, L"A52/AC-3 AUDIO (DOLBY_AC3_AUDIO)");
	types[0x82] = pes_stream_type(0x82, media::type::AUDIO, media::supported::NO, L"HDMV DTS AUDIO");
	types[0x83] = pes_stream_type(0x83, media::type::AUDIO, media::supported::NO, L"LPCM AUDIO");
	types[0x84] = pes_stream_type(0x84, media::type::AUDIO, media::supported::NO, L"SDDS AUDIO");
	types[0x85] = pes_stream_type(0x85, media::type::UNKNOWN, media::supported::NO, L"ATSC Program ID");
	types[0x86] = pes_stream_type(0x86, media::type::AUDIO, media::supported::NO, L"DTS-HD AUDIO");
	types[0x87] = pes_stream_type(0x87, media::type::AUDIO, media::supported::NO, L"E-AC-3 AUDIO (DOLBY_DIGITAL_PLUS_AUDIO_ATSC)");
	types[0x8A] = pes_stream_type(0x8A, media::type::AUDIO, media::supported::NO, L"DTS AUDIO");
	types[0x91] = pes_stream_type(0x91, media::type::AUDIO, media::supported::NO, L"A52b/AC-3 AUDIO");
	types[0x92] = pes_stream_type(0x92, media::type::UNKNOWN, media::supported::NO, L"DVD_SPU vls Subtitle");
	types[0x94] = pes_stream_type(0x94, media::type::AUDIO, media::supported::NO, L"SDDS AUDIO");
	types[0xA0] = pes_stream_type(0xA0, media::type::VIDEO, media::supported::NO, L"MSCODEC VIDEO");
	types[0xEA] = pes_stream_type(0xEA, media::type::UNKNOWN, media::supported::NO, L"Private ES (VC-1)");

	// UNKNOWN type - return in NOt found case
	types[pes_stream_type::UNKNOWN_TYPE_CODE] = pes_stream_type(pes_stream_type::UNKNOWN_TYPE_CODE, media::type::UNKNOWN, media::supported::NO, L"UNKNOWN TYPE");

	return std::move(types);
}

const pes_stream_type&
pes_stream_types::lookup(type_t type_code)
{
	try {
		const pes_stream_type& type = types_.at(type_code);
		return type;
	}
	catch (std::out_of_range&)
	{
	}
	return (*types_.find(pes_stream_type::UNKNOWN_TYPE_CODE)).second;

}

std::map<type_t, pes_stream_type>
pes_stream_types::types_ = pes_stream_types::initialize_types();

/*
{
// taken from http://www.sNO.phy.queensu.ca/~phil/exiftool/TagNames/M2TS.html
// and http://msdn.microsoft.com/en-us/library/windows/hardware/ff567738(v=vs.85).aspx

{ 0x00, { 0x00, media::type::UNKNOWN, media::supported::NO, L"Reserved" } },
{ 0x01, { 0x01, media::type::VIDEO, media::supported::NO, L"MPEG-1 VIDEO (ISO_IEC_11172_2_VIDEO)" } },
{ 0x02, { 0x02, media::type::VIDEO, media::supported::NO, L"MPEG-2 VIDEO (ISO_IEC_13818_2_VIDEO)" } },
{ 0x03, { 0x03, media::type::AUDIO, media::supported::NO, L"MPEG-1 AUDIO (ISO_IEC_11172_3_AUDIO)" } },
{ 0x04, { 0x04, media::type::AUDIO, media::supported::NO, L"MPEG-2 AUDIO (ISO_IEC_13818_3_AUDIO)" } },
{ 0x05, { 0x05, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-1 private sections (ISO_IEC_13818_1_PRIVATE_SECTION)" } },
{ 0x06, { 0x06, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-1 PES private data (ISO_IEC_13818_1_PES)" } },
{ 0x07, { 0x07, media::type::UNKNOWN, media::supported::NO, L"ISO 13522 MHEG (ISO_IEC_13522_MHEG)" } },
{ 0x08, { 0x08, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-1 DSM-CC (ANNEX_A_DSM_CC)" } },
{ 0x09, { 0x09, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-1 auxiliary (ITU_T_REC_H_222_1)" } },
{ 0x0A, { 0x0A, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-6 multi-protocol encap (ISO_IEC_13818_6_TYPE_A)" } },
{ 0x0B, { 0x0B, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-6 DSM-CC U-N msgs (ISO_IEC_13818_6_type_B)" } },
{ 0x0C, { 0x0C, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-6 stream descriptors (ISO_IEC_13818_6_TYPE_C" } },
{ 0x0D, { 0x0D, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-6 sections (ISO_IEC_13818_6_TYPE_D)" } },
{ 0x0E, { 0x0E, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-1 auxiliary (ISO_IEC_13818_1_AUXILIARY)" } },

{ 0x0F, { 0x0F, media::type::AUDIO, media::supported::YES, L"MPEG-2 AAC AUDIO (ISO/IEC 13818-7 AUDIO with ADTS transport syntax)" } },

{ 0x10, { 0x10, media::type::AUDIO, media::supported::NO, L"MPEG-4 VIDEO (ISO_IEC_14496_2_VISUAL)" } },
{ 0x11, { 0x11, media::type::AUDIO, media::supported::NO, L"MPEG-4 LATM AAC AUDIO (ISO_IEC_14496_3_AUDIO)" } },
{ 0x12, { 0x12, media::type::UNKNOWN, media::supported::NO, L"MPEG-4 generic (ISO_IEC_14496_1_IN_PES)" } },
{ 0x13, { 0x13, media::type::UNKNOWN, media::supported::NO, L"ISO 14496-1 SL-packetized (ISO_IEC_14496_1_IN_SECTION)" } },
{ 0x14, { 0x14, media::type::UNKNOWN, media::supported::NO, L"ISO 13818-6 Synchronized Download Protocol (ISO_IEC_13818_6_DOWNLOAD)" } },
{ 0x15, { 0x15, media::type::UNKNOWN, media::supported::NO, L"(METADATA_IN_PES)" } },
{ 0x16, { 0x16, media::type::UNKNOWN, media::supported::NO, L"(METADATA_IN_SECTION)" } },
{ 0x17, { 0x17, media::type::UNKNOWN, media::supported::NO, L"(METADATA_IN_DATA_CAROUSEL)" } },
{ 0x18, { 0x18, media::type::UNKNOWN, media::supported::NO, L"(METADATA_IN_OBJECT_CAROUSEL)" } },
{ 0x19, { 0x19, media::type::UNKNOWN, media::supported::NO, L"(METADATA_IN_DOWNLOAD_PROTOCOL)" } },
{ 0x1A, { 0x1A, media::type::UNKNOWN, media::supported::NO, L"(IRPM_STREAMM)" } },

{ 0x1B, { 0x1B, media::type::VIDEO, media::supported::YES, L"H.264 VIDEO (ITU_T_H264)" } },

{ 0x80, { 0x80, media::type::VIDEO, media::supported::NO, L"DigiCipher II VIDEO (ISO_IEC_USER_PRIVATE)" } },
{ 0x81, { 0x81, media::type::AUDIO, media::supported::NO, L"A52/AC-3 AUDIO (DOLBY_AC3_AUDIO)" } },
{ 0x82, { 0x82, media::type::AUDIO, media::supported::NO, L"HDMV DTS AUDIO" } },
{ 0x83, { 0x83, media::type::AUDIO, media::supported::NO, L"LPCM AUDIO" } },
{ 0x84, { 0x84, media::type::AUDIO, media::supported::NO, L"SDDS AUDIO" } },
{ 0x85, { 0x85, media::type::UNKNOWN, media::supported::NO, L"ATSC Program ID" } },
{ 0x86, { 0x86, media::type::AUDIO, media::supported::NO, L"DTS-HD AUDIO" } },
{ 0x87, { 0x87, media::type::AUDIO, media::supported::NO, L"E-AC-3 AUDIO (DOLBY_DIGITAL_PLUS_AUDIO_ATSC)" } },
{ 0x8A, { 0x8A, media::type::AUDIO, media::supported::NO, L"DTS AUDIO" } },
{ 0x91, { 0x91, media::type::AUDIO, media::supported::NO, L"A52b/AC-3 AUDIO" } },
{ 0x92, { 0x92, media::type::UNKNOWN, media::supported::NO, L"DVD_SPU vls Subtitle" } },
{ 0x94, { 0x94, media::type::AUDIO, media::supported::NO, L"SDDS AUDIO" } },
{ 0xA0, { 0xA0, media::type::VIDEO, media::supported::NO, L"MSCODEC VIDEO" } },
{ 0xEA, { 0xEA, media::type::UNKNOWN, media::supported::NO, L"Private ES (VC-1)" } },

// UNKNOWN type - return in NOt found case
{ pes_stream_type::UNKNOWN_TYPE_CODE, { pes_stream_type::UNKNOWN_TYPE_CODE, media::type::UNKNOWN, media::supported::NO, L"UNKNOWN TYPE" } }
};
*/
