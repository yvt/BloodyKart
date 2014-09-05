/*
 *  client.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/23.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#ifndef _CLIENT_H
#define _CLIENT_H

#include "global.h"
#include "sfx.h"

extern float c_maxHealth;
extern float c_regenTime;
extern float c_availTime;
extern float c_respawnTime;
extern float c_invincibleTime;
extern float c_spawnTime;

class client_t;

class controller_t{
public:
	client_t *client;
	virtual void bind(client_t * c){client=c;}
	virtual void framenext(float dt){}
	virtual float get_steering(){return 0.f;}
	virtual int get_accel(){return 0;}
	virtual void hit(vec3_t){}
	virtual bool fire(){return false;}
	virtual void clientHit(int by, vec3_t pos){}
};

class spectate_controller_t : public controller_t{
public:	
	virtual float get_strafe(){return 0.f;}
	virtual float get_vertical(){return 0.f;}
	virtual float get_pitch(){return 0.f;}
};

#define MAX_HIST		16

// dead reason
#define BY_MAP			-1

// dead flag
#define BYF_WEAPON		0x00000000	// dodged weapon
#define BYF_WEAPONSPL	0x00010000	// weapon's splash damage
#define BYF_COLISION	0x00020000	// striked too strong
#define BYF_EXPLOSION	0x00030000	// exploded when dying

#define BYF_MASKCLIENT	0x0000ffff
#define BYF_MASKFLAG	0xffff0000


class client_t{
	
public:
	IPaddress addr;
	Uint32 oresp;
	char name[64];
	bool ready;
	
	bool enable;
	bool spectate;
	vec3_t pos;
	vec3_t vel;
	vec3_t ang;
	vec3_t mang;
	controller_t *ctrl;
	char model[64];
	
	int slipsnd;
	float slip;
	
	float now_speed;
	
	float max_speed;
	float grip;
	float brake_damp;
	float power;
	float damp;
	float steer;
	float slipfactor;
	
	char weapon[5];
	int ammo;
	float weapon_wait; // if it>0 weapon is being choosed...
	float weapon_delay;
	
	float view_steer;
	
	float health;
	float regen_delay;
	
	float dead_time;
	float alive_time;
	float speedScale; // changes depending on rank
	float suicideTime;
	
	vec3_t hist[MAX_HIST]; // history of pos (only when on the ground)
	vec3_t hist_ang[MAX_HIST];
	float hist_delay; int hist_pos;
	
	bool ofire;
	float weapFrame; // for client; weapon animation frame
	
	bool fired; // for client; only true in one frame when fired
	
	int score;
	
	bool has_weapon();
	bool is_dead();
	bool is_avail();
	
	void respawn();
	void damage(float amount, vec3_t from, int by);
	void instantKill(int by);
	void explode();
	vec3_t getWeaponPos();
	
	int getID();
	
protected:
	
	void killedBy(int by);
	
};

#define MAX_CLIENTS 16

extern client_t cli[MAX_CLIENTS];
void C_init();
void C_framenext(float dt);
void C_cframenext(float dt);
int C_spawn(const char *, controller_t *, bool spectate=false);
void C_setup(int);
int C_count(); // entering player only
int C_getRank(int); // 0=best

#endif


