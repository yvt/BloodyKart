/*
 *  crosshair.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/12/11.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "crosshair.h"
#include "client.h"
#include "glpng.h"
#include "clientgame.h"


#define CROSSHAIRS	1
GLuint tex_crosshair[CROSSHAIRS];

void CrossHair_init(){
	char buf[64];
	int n;
	
	glGenTextures(CROSSHAIRS, tex_crosshair);
	
	for(n=0;n<CROSSHAIRS;n++){
		sprintf(buf, "res/sprites/crosshair%d.png", n+1);
		glBindTexture(GL_TEXTURE_2D, tex_crosshair[n]);
		glpngLoadTexture(buf, true);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}
}
void CrossHair_render(){
	double xx, yy, zz;
	vec3_t tgtPos;
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	if(!cli[yourClient].has_weapon())
		return;
	
	tgtPos=cli[yourClient].pos+rotate(vec3_t(0.f, 0.f, 1000.f) ,cli[yourClient].ang);
	
	gluProject(tgtPos.x, tgtPos.y, tgtPos.z, osd_old_modelview, osd_old_proj, viewport, &xx, &yy, &zz);
	
	if(zz>1.)
		return;
	yy=(double)screen->h-yy;

	glBindTexture(GL_TEXTURE_2D, tex_crosshair[0]);
	
	float cx, cy, sz;
	cx=xx;
	cy=yy;
	sz=(float)screen->w/30.f;
	
	glColor4f(1.f, 1.f, 1.f, 1.f);
	
	glBegin(GL_QUADS);
	
	glTexCoord2i(0, 0);
	glVertex2f(cx-sz, cy-sz);
	
	glTexCoord2i(1, 0);
	glVertex2f(cx+sz, cy-sz);
	
	glTexCoord2i(1, 1);
	glVertex2f(cx+sz, cy+sz);
	
	glTexCoord2i(0, 1);
	glVertex2f(cx-sz, cy+sz);
	
	glEnd();
	
	totalPolys++;
	
	if(SDL_GetTicks()<cg_oHitTime+200){
		glColor4f(1.f, 0.f, 0.f, 1.f);
		
		glBegin(GL_QUADS);
		
		glTexCoord2i(0, 0);
		glVertex2f(cx-sz, cy-sz);
		
		glTexCoord2i(1, 0);
		glVertex2f(cx+sz, cy-sz);
		
		glTexCoord2i(1, 1);
		glVertex2f(cx+sz, cy+sz);
		
		glTexCoord2i(0, 1);
		glVertex2f(cx-sz, cy+sz);
		
		glEnd();
		
		totalPolys++;
	}
	
}

