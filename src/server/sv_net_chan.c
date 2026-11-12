/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "server.h"

#if DO_NET_ENCODE
/*
==============
SV_Netchan_Encode

	// first four bytes of the data are always:
	long reliableAcknowledge;

==============
*/
static void SV_Netchan_Encode( client_t *client, msg_t *msg ) {
	long reliableAcknowledge, i, index;
	byte key, *string;
	int srdc, sbit, soob;

	if ( msg->cursize < SV_ENCODE_START ) {
		return;
	}

	srdc = msg->readcount;
	sbit = msg->bit;
	soob = msg->oob;

	msg->bit = 0;
	msg->readcount = 0;
	msg->oob = 0;

	reliableAcknowledge = MSG_ReadLong( msg );

	msg->oob = soob;
	msg->bit = sbit;
	msg->readcount = srdc;

	string = (byte *)client->lastClientCommandString;
	index = 0;
	// xor the client challenge with the netchan sequence number
	key = client->challenge ^ client->netchan.outgoingSequence;
	for ( i = SV_ENCODE_START; i < msg->cursize; i++ ) {
		// modify the key with the last received and with this message acknowledged client command
		if ( !string[index] ) {
			index = 0;
		}
		if ( string[index] > 127 || string[index] == '%' ) {
			key ^= '.' << ( i & 1 );
		} else {
			key ^= string[index] << ( i & 1 );
		}
		index++;
		// encode the data with this key
		*( msg->data + i ) = *( msg->data + i ) ^ key;
	}
}

/*
==============
SV_Netchan_Decode

	// first 12 bytes of the data are always:
	long serverId;
	long messageAcknowledge;
	long reliableAcknowledge;

==============
*/
static void SV_Netchan_Decode( client_t *client, msg_t *msg ) {
	int serverId, messageAcknowledge, reliableAcknowledge;
	int i, index, srdc, sbit, soob;
	byte key, *string;

	srdc = msg->readcount;
	sbit = msg->bit;
	soob = msg->oob;

	msg->oob = 0;

	serverId = MSG_ReadLong( msg );
	messageAcknowledge = MSG_ReadLong( msg );
	reliableAcknowledge = MSG_ReadLong( msg );

	msg->oob = soob;
	msg->bit = sbit;
	msg->readcount = srdc;

	string = (byte *)SV_GetReliableCommand( client, reliableAcknowledge & ( MAX_RELIABLE_COMMANDS - 1 ) );
	index = 0;
	//
	key = client->challenge ^ serverId ^ messageAcknowledge;
	for ( i = msg->readcount + SV_DECODE_START; i < msg->cursize; i++ ) {
		// modify the key with the last sent and acknowledged server command
		if ( !string[index] ) {
			index = 0;
		}
		if ( string[index] > 127 || string[index] == '%' ) {
			key ^= '.' << ( i & 1 );
		} else {
			key ^= string[index] << ( i & 1 );
		}
		index++;
		// decode the data with this key
		*( msg->data + i ) = *( msg->data + i ) ^ key;
	}
}
#endif

/*
=================
SV_Netchan_TransmitNextFragment
=================
*/
void SV_Netchan_TransmitNextFragment( netchan_t *chan ) {
	Netchan_TransmitNextFragment( chan );
}


/*
===============
SV_Netchan_Transmit
================
*/

//extern byte chksum[65536];
/* Exact encoded EOF preflight.  Scratch padding protects the legacy MSG
 * writer's final symbol, while maxsize remains the logical wire limit. */
qboolean SV_MessageFitsWithEOF( const msg_t *msg ) {
	byte bytes[SV_MESSAGE_BUFFER_SIZE];
	msg_t probe;
	int savedOldsize;
	extern int oldsize;
	if ( !msg || !msg->data || msg->oob || msg->overflowed ||
		 msg->maxsize < 4 || msg->maxsize > MAX_MSGLEN ||
		 msg->cursize < 0 || msg->cursize > msg->maxsize ||
		 msg->bit < 0 || msg->bit > msg->maxsize * 8 ||
		 ( msg->cursize == 0 ? msg->bit != 0 :
			msg->cursize != ( msg->bit >> 3 ) + 1 ) ) return qfalse;
	probe = *msg;
	probe.data = bytes;
	memcpy( bytes, msg->data, msg->cursize );
	savedOldsize = oldsize;
	oldsize = 0;
	MSG_WriteByte( &probe, svc_EOF );
	oldsize = savedOldsize;
	return !probe.overflowed && probe.cursize <= probe.maxsize &&
		probe.bit <= probe.maxsize * 8 &&
		probe.cursize <= SV_MESSAGE_PAYLOAD_LIMIT;
}

qboolean SV_Netchan_Transmit( client_t *client, msg_t *msg ) {
//	int i;
	if ( !SV_MessageFitsWithEOF( msg ) ) return qfalse;
	MSG_WriteByte( msg, svc_EOF );
//	for(i=SV_ENCODE_START;i<msg->cursize;i++) {
//		chksum[i-SV_ENCODE_START] = msg->data[i];
//	}
//	Huff_Compress( msg, SV_ENCODE_START );
#if DO_NET_ENCODE
	SV_Netchan_Encode( client, msg );
#endif
	Netchan_Transmit( &client->netchan, msg->cursize, msg->data );
	return qtrue;
}

/*
=================
Netchan_SV_Process
=================
*/
qboolean SV_Netchan_Process( client_t *client, msg_t *msg ) {
	int ret;
//	int i;
	ret = Netchan_Process( &client->netchan, msg );
	if ( !ret ) {
		return qfalse;
	}
#if DO_NET_ENCODE
	SV_Netchan_Decode( client, msg );
#endif
//	Huff_Decompress( msg, SV_DECODE_START );
//	for(i=SV_DECODE_START+msg->readcount;i<msg->cursize;i++) {
//		if (msg->data[i] != chksum[i-(SV_DECODE_START+msg->readcount)]) {
//			Com_Error(ERR_DROP,"bad\n");
//		}
//	}
	return qtrue;
}

