/*
 *  effect.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/09/29.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

// effect works in client

void E_init();
void E_clear();
void E_cframenext(float);
void E_render();

void E_mark(vec3_t pos, vec3_t proj, vec3_t rgb, float a,
			float radius, float ang, GLuint tex,
			float tx, float ty, float tw, float th);
int E_renderMark(vec3_t pos, vec3_t proj, vec3_t rgb, float a,
				 float radius, float ang, GLuint tex,
				 float tx, float ty, float tw, float th);

void E_blood(vec3_t);
void E_bloodBig(vec3_t);
void E_smoke(vec3_t);
void E_explode(vec3_t);
void E_explodeBig(vec3_t);
void E_bulletMark(vec3_t pos, vec3_t proj);
void E_muzzleFlash(int c, vec3_t shift);

void E_addMapItems();
void E_removeMapItems();
