/*
 *  cmds.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/30.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#pragma once

#include "client.h"
#include "weapon.h"

#define PROTOCOL_VERSION		11
#define DEFAULT_PORT			22050
#define PROTOCOL_MAGIC			0x0b10dd1e

enum cmd_type_t{
	CCMD_none=0,
	SCMD_none=0,
	SCMD_initial,
	SCMD_update,
	SCMD_sound,
	SCMD_deny,
	SCMD_msg,
	SCMD_kick,
	SCMD_blood,
	SCMD_bullet_impact,
	SCMD_explode,
	SCMD_explode_big,
	SCMD_screen_kick,
	SCMD_hurt,
	SCMD_fired,
	SCMD_fragged_msg,
	SCMD_hit,
	SCMD_blood_big,
	
	CCMD_connect=16384,
	CCMD_control,
	CCMD_ready,
	CCMD_arrive,
	CCMD_connect_spectate,
	CCMD_enter,	// stop being spectator
	CCMD_leave	// become spectator
	
};

struct cmd_vec3_t{
	float x, y, z;
	
	operator vec3_t() const{
		return vec3_t(x, y, z);
	}
};

static inline cmd_vec3_t cvec3(vec3_t v){
	cmd_vec3_t vv;
	vv.x=v.x; vv.y=v.y; vv.z=v.z;
	return vv;
}

struct scmd_initial_t{
	int magic;					// must be PROTOCOL_MAGIC
	int index;
	cmd_type_t type;			// SCMD_initial
	char mapname[32];
	int yourClient;
};

struct scmd_update_client_t{
	unsigned char enable; unsigned char spectate;
	short score;
	char name[32];
	char model[8];
	cmd_vec3_t pos, vel, ang, mang;
	float slip;
	char weapon[6];
	float weapon_wait;
	float health, dead_time;
	float alive_time;
	short ammo;
	float view_steer;
};
struct scmd_update_projectile_t{
	unsigned char used;
	char weapon[7];
	cmd_vec3_t pos, vel;
};
struct scmd_update_t{
	int magic;					// must be PROTOCOL_MAGIC
	int index;
	cmd_type_t type;			// SCMD_update
	scmd_update_projectile_t projectile[MAX_PROJECTILES];
	float speed;
	scmd_update_client_t clients[MAX_CLIENTS];	
};
struct scmd_sound_t{
	int magic;					// must be PROTOCOL_MAGIC
	int index;
	cmd_type_t type;			// SCMD_sound
	char name[16];
	int is3d;
	cmd_vec3_t pos;
	float factor;
	int speed;
};
struct scmd_effect_t{
	int magic;					// must be PROTOCOL_MAGIC
	int index;
	cmd_type_t type;			// 
	cmd_vec3_t pos;
};
struct scmd_hurt_t{
	int magic;					// must be PROTOCOL_MAGIC
	int index;
	cmd_type_t type;			// SCMD_hurt
	cmd_vec3_t from;
	float amount;
};
struct scmd_screen_kick_t{
	int magic;					// must be PROTOCOL_MAGIC
	int index;
	cmd_type_t type;			// SCMD_screen_kick
	float amount;
};
struct scmd_bullet_impact_t{
	int magic;					// must be PROTOCOL_MAGIC
	int index;
	cmd_type_t type;			// SCMD_bullet_impact
	cmd_vec3_t pos, proj;
};
struct scmd_fired_t{
	int magic;					// must be PROTOCOL_MAGIC
	int index;
	cmd_type_t type;			// SCMD_fired
	int cli;
};
struct scmd_msg_t{
	int magic;					// must be PROTOCOL_MAGIC
	int index;
	cmd_type_t type;			// SCMD_msg/SCMD_fraggedMsg
	char msg[256];
};
struct scmd_hit_t{
	int magic;					// must be PROTOCOL_MAGIC
	int index;
	cmd_type_t type;			// SCMD_hit
	int cli; // target
};

struct ccmd_connect_t{
	int magic;					// must be PROTOCOL_MAGIC
	int index;
	cmd_type_t type;			// CCMD_connect
	int protocol;				// must be PROTOCOL_VERSION
	char model[16];
	char name[32];
};

#define CCCSW_none				0x00000000
#define CCCSW_fire				0x00000001
#define CCCSW_raise				0x00000002
#define CCCSW_lower				0x00000004
struct ccmd_control_t{
	int magic;					// must be PROTOCOL_MAGIC
	int index;
	cmd_type_t type;			// CCMD_control
	int accel;
	float steer;
	int sw;
	float strafe; // for spectator
	float pitch; // for spectator
};

struct ccmd_arrive_t{
	int magic;					// must be PROTOCOL_MAGIC
	int index;
	cmd_type_t type;			// CCMD_arrive
	int cmdindex;
};

union cmd_t{
	struct{
		int magic;					// must be PROTOCOL_MAGIC
		int index;
		cmd_type_t type;
	};
	scmd_initial_t scmd_initial;
	scmd_update_t scmd_update;
	scmd_sound_t scmd_sound;
	scmd_msg_t scmd_msg;
	scmd_effect_t scmd_blood;
	scmd_effect_t scmd_blood_big;
	scmd_effect_t scmd_explode;
	scmd_effect_t scmd_explode_big;
	scmd_hurt_t scmd_hurt;
	scmd_bullet_impact_t scmd_bullet_impact;
	scmd_screen_kick_t scmd_screen_kick;
	scmd_fired_t scmd_fired;
	scmd_hit_t scmd_hit;
	ccmd_connect_t ccmd_connect;
	ccmd_control_t ccmd_control;
	ccmd_arrive_t ccmd_arrive;
};

void CMD_sendCommand(UDPsocket sock, cmd_t cmd, IPaddress addr);
bool CMD_recvCommand(UDPsocket sock, cmd_t *cmd, IPaddress *addr);
int CMD_getCmdSize(const cmd_t& cmd);
