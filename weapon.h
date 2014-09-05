/*
 *  weapon.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/09/13.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#pragma once

#include "mesh.h"
#include "scenetree.h"

#define MAX_PROJECTILES			64

enum weapon_mode_t{
	WM_hitscan,
	WM_projectile
};
enum weapon_apperance_t{
	WAM_bullet,
	WAM_rocket
};


class weapon_t{
public:
	char name[5];
	char fullName[256];
	weapon_mode_t mode;
	weapon_apperance_t apperance;
	float interval;
	float speed, damage;
	char sound[4][64];
	bool issound[4];
	float sndscale[4];
	int defaultAmmo;
	float splashRange;
	float splashDamage;
	char fragMsg[128];
	char splashFragMsg[128];
	char suicideMsg[128];
	float spread;
	float gravityScale;
	float lifeTime;
	float halfSpeedTime;
	float level;
	
	int getFrames;
	int idleFrames;
	int fireFrames;
	
	GLuint icon;
	scene_t *m;
	vec3_t muzzle;
	vec3_t fpPos;
	
	weapon_t(const char *);
	void render(int c, vec3_t shift);
	vec3_t getFirePos();
	float calcDamage(float time);
};

class projectile_t{
public:
	bool used;
	vec3_t pos, vel;
	char weapon[5];
	int by;
	float frm;
	bool operator <(const projectile_t& p)const{
		return frm<p.frm;
	}
};

extern projectile_t projectile[MAX_PROJECTILES];


void W_init();
weapon_t *W_find(const char *);
weapon_t *W_find(int);
void W_framenext(float);
void W_cframenext(float);
void W_fire(int c, float delay);
void W_clear();
weapon_t *W_choose(int rank); // rank 0=best
int W_indexOf(weapon_t *);
int W_count();
void W_render(); // for projectiles

