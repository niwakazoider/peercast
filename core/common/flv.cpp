// ------------------------------------------------
// File : flv.cpp
// Date: 14-jan-2017
// Author: niwakazoider
//
// (c) 2002-3 peercast.org
// ------------------------------------------------
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ------------------------------------------------

#include "channel.h"
#include "flv.h"
#include "string.h"
#include "stdio.h"
#ifdef _DEBUG
#include "chkMemoryLeak.h"
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW
#endif


// ------------------------------------------
void FLVStream::readEnd(Stream &, Channel *)
{
}

// ------------------------------------------
void FLVStream::readHeader(Stream &, Channel *)
{
}

// ------------------------------------------
int FLVStream::readPacket(Stream &in, Channel *ch)
{
	if (ch->streamPos == 0) {
		bitrate = 0;
		FLVFileHeader fileHeader = FLVFileHeader();
		fileHeader.read(in);

		char* header = (char *)malloc(8192);
		int offset = 0;
		memcpy(header, fileHeader.data, fileHeader.size);
		offset += fileHeader.size;

		for (int i = 0; i < 3; i++) {
			FLVTag flvTag = FLVTag();
			flvTag.read(in);

			switch (flvTag.type) {
			case TAG_SCRIPTDATA:
			{
				AMFObject amf;
				if (amf.readMetaData(flvTag)) {
					bitrate = amf.bitrate;
					memcpy(header + offset, flvTag.packet, flvTag.packetSize);
					offset += flvTag.packetSize;
				}
				break;
			}
			case TAG_VIDEO:
			{
				//AVC Header
				if (flvTag.data[0] == 0x17 && flvTag.data[1] == 0x00 &&
					flvTag.data[2] == 0x00 && flvTag.data[3] == 0x00) {
					memcpy(header + offset, flvTag.packet, flvTag.packetSize);
					offset += flvTag.packetSize;
				}
				break;
			}
			case TAG_AUDIO:
			{
				//AAC Header
				if (flvTag.data[0] == 0xaf && flvTag.data[1] == 0x00) {
					memcpy(header + offset, flvTag.packet, flvTag.packetSize);
					offset += flvTag.packetSize;
				}
				break;
			}
			}
		}

		if (offset>0) {
			ch->headPack.init(ChanPacket::T_HEAD, ch->headPack.data, offset, ch->streamPos);
			memcpy(ch->headPack.data, header, offset);

			ch->info.bitrate = bitrate;
			ch->headPack.pos = ch->streamPos;
			ch->newPacket(ch->headPack);

			ch->streamPos += ch->headPack.len;
		}

		free(header);
	}


	FLVTag flvTag;
	flvTag.read(in);

	ChanPacket pack;

	MemoryStream mem(flvTag.packet, flvTag.packetSize);

	int rlen = flvTag.packetSize;
	while (rlen)
	{
		int rl = rlen;
		if (rl > ChanMgr::MAX_METAINT)
			rl = ChanMgr::MAX_METAINT;

		pack.init(ChanPacket::T_DATA, pack.data, rl, ch->streamPos);
		mem.read(pack.data, pack.len);
		ch->newPacket(pack);
		ch->checkReadDelay(pack.len);
		ch->streamPos += pack.len;

		rlen -= rl;
	}

	mem.close();

	return 0;
}
