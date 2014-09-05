/*
 *  cmds.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/30.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "cmds.h"

static int getCmdUpdateLeftBytes(const scmd_update_t& cmd){
	int n, lf;
	lf=0;
	for(n=MAX_CLIENTS-1;n>=0;n--){
		if(cmd.clients[n].enable || cmd.clients[n].spectate)
			break;
		lf+=sizeof(scmd_update_client_t);
	}
	return lf;
}

int CMD_getCmdSize(const cmd_t& cmd){
	if(cmd.type==CCMD_none)
		return 0;
	else if(cmd.type==SCMD_none)
		return 0;
	else if(cmd.type==SCMD_initial)
		return sizeof(scmd_initial_t);
	else if(cmd.type==SCMD_update)
		return sizeof(scmd_update_t)-getCmdUpdateLeftBytes(cmd.scmd_update);
	else if(cmd.type==SCMD_sound)
		return sizeof(scmd_sound_t);
	else if(cmd.type==SCMD_deny)
		return 12;
	else if(cmd.type==SCMD_msg)
		return sizeof(scmd_msg_t);
	else if(cmd.type==SCMD_kick)
		return 12;
	else if(cmd.type==SCMD_blood)
		return sizeof(scmd_effect_t);
	else if(cmd.type==SCMD_bullet_impact)
		return sizeof(scmd_bullet_impact_t);
	else if(cmd.type==SCMD_explode)
		return sizeof(scmd_effect_t);
	else if(cmd.type==SCMD_explode_big)
		return sizeof(scmd_effect_t);
	else if(cmd.type==SCMD_screen_kick)
		return sizeof(scmd_screen_kick_t);
	else if(cmd.type==SCMD_hurt)
		return sizeof(scmd_hurt_t);
	else if(cmd.type==SCMD_fired)
		return sizeof(scmd_fired_t);
	else if(cmd.type==SCMD_fragged_msg)
		return sizeof(scmd_msg_t);
	else if(cmd.type==SCMD_hit)
		return sizeof(scmd_hit_t);
	else if(cmd.type==SCMD_blood_big)
		return sizeof(scmd_effect_t);
	
	else if(cmd.type==CCMD_connect)
		return sizeof(ccmd_connect_t);
	else if(cmd.type==CCMD_control)
		return sizeof(ccmd_control_t);
	else if(cmd.type==CCMD_ready)
		return 12;
	else if(cmd.type==CCMD_arrive)
		return sizeof(ccmd_arrive_t);
	else if(cmd.type==CCMD_connect_spectate)
		return sizeof(ccmd_connect_t);
	else if(cmd.type==CCMD_enter)
		return 12;
	else if(cmd.type==CCMD_leave)
		return 12;
	else
		return sizeof(cmd_t);
}

void CMD_sendCommand(UDPsocket sock ,cmd_t cmd, IPaddress addr){
	UDPpacket *packet;
	packet=SDLNet_AllocPacket(sizeof(cmd));
	//memset(packet, 0, sizeof(UDPpacket));
	int n;
	uint32_t *ptr=(uint32_t *)(&cmd);
	for(n=0;n<sizeof(cmd);n+=4){
		SDLNet_Write32(*ptr++,packet->data+n); 
	}
	
	packet->len=packet->maxlen=CMD_getCmdSize(cmd);//sizeof(cmd);
	packet->address=addr;
	packet->channel=-1;
	if(!SDLNet_UDP_Send(sock, -1, packet)){
		//throw "failed to send data";
	}
	
	SDLNet_FreePacket(packet);
}

bool CMD_recvCommand(UDPsocket sock, cmd_t *cmd, IPaddress *addr){
	UDPpacket *packet;
	packet=SDLNet_AllocPacket(sizeof(*cmd));
	if(SDLNet_UDP_Recv(sock, packet)){
		int n;
		uint8_t *ptr=(uint8_t *)packet->data;
		uint32_t *ptr2=(uint32_t *)cmd;
		memset(cmd, 0, sizeof(cmd_t));
		for(n=0;n<sizeof(*cmd);n+=4){
			if(n>=packet->len) 
				break;  // underflow; reduced?
			*ptr2++=SDLNet_Read32(ptr+n); 
		}
		*addr=packet->address;
		
		SDLNet_FreePacket(packet);
		
		return true;
		
	}
	
	SDLNet_FreePacket(packet);
	
	return false;
}

