/*
 *  hurtfx.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/11/23.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "hurtfx.h"
#include "client.h"
#include "clientgame.h"

#define MAX_HURTS		64

struct hurt_t{
	bool used;
	float ang;
	float time;
	bool operator <(const hurt_t& h) const{
		return time<h.time;
	}
};

hurt_t hurt[MAX_HURTS];

float h_hurtTime=1.2f;
float h_hurtFadeTime=.5f;
float h_hurtRadius=.5f;
float h_hurtSize=.08f;



void H_init(){
	int n;
	for(n=0;n<MAX_HURTS;n++){
		hurt[n].used=false;
	}
	consoleLog("H_init: all hurtfx reseted\n");
}
void H_cframenext(float dt){
	int n;
	for(n=0;n<MAX_HURTS;n++){
		hurt_t& h=hurt[n];
		if(!h.used)
			continue;
		h.time+=dt;
		if(h.time>h_hurtTime)
			h.used=false;
	}
}
static int AllocHurt(){
	for(int n=0;n<MAX_HURTS;n++)
		if(!hurt[n].used)
			return n;
	consoleLog("AllocHurt: hurtfx count limit reached, removing oldest hurtfx\n");
	return max_element(hurt, hurt+MAX_HURTS)-hurt;
}
void H_hurt(float amount, vec3_t pos){
	hurt_t& h=hurt[AllocHurt()];
	h.used=true;
	
	// calculate angle
	vec3_t dif;
	dif=pos-cli[yourClient].pos;
	
	h.ang=atan2f(dif.x, dif.z);
	h.time=0.f;
}
void H_render(){
	int n;
	
	glDisable(GL_TEXTURE_2D);
	
	float cx, cy, rr;
	float siz;
	float baseAng;
	
	cx=(float)screen->w*.5f;
	cy=(float)screen->h*.5f;
	rr=cy*h_hurtRadius;
	siz=cy*h_hurtSize;
	baseAng=atan2f(camera_at.x-camera_from.x, camera_at.z-camera_from.z);
	
	float hl=0.f;
	glDisable(GL_CULL_FACE);
	glBegin(GL_TRIANGLES);
	for(n=0;n<MAX_HURTS;n++){
		hurt_t& h=hurt[n];
		if(!h.used)
			continue;
		
		float ang, dx, dy;
		float xx, yy;
		ang=h.ang-baseAng;
		dx=sinf(ang); dy=-cosf(ang);
		
		glColor4f(1.f, 0.f, 0.f, min(1.f, (h_hurtTime-h.time)/h_hurtFadeTime));
		glVertex2f(cx+dx*rr, cy+dy*rr);
		
		xx=cx+dx*rr*(1.f-h_hurtSize);
		yy=cy+dy*rr*(1.f-h_hurtSize);
		glColor4f(1.f, 0.f, 0.f, 0.f);
		glVertex2f(xx+dy*h_hurtSize*rr, yy-dx*h_hurtSize*rr);
		glVertex2f(xx-dy*h_hurtSize*rr, yy+dx*h_hurtSize*rr);
		
		hl=max(hl, .2f-h.time);
		
		totalPolys++;
		
	}
	glEnd();
	
	if(hl>.001f){
	glColor4f(1.f, 0.f, 0.f, hl/.2f*.6f);
	glBegin(GL_QUADS);
	glVertex2i(0, 0); glVertex2i(0, screen->h);
	glVertex2i(screen->w, screen->h); glVertex2i(screen->w, 0);
	glEnd();
		totalPolys+=2;
	}
	
	glEnable(GL_TEXTURE_2D);
}

