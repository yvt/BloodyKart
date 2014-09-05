/*
 *  flare.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/12/20.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "flare.h"
#include "map.h"
#include "glpng.h"

static GLuint tex_flare;
static float flareFade[MAX_LIGHTS+1];

void F_init(){
	int n;
	glGenTextures(1, &tex_flare);
	glBindTexture(GL_TEXTURE_2D, tex_flare);
	glpngLoadTexture("res/sprites/flare.png", true);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	consoleLog("F_init: loading res/sprites/flare.png\n");
	for(n=0;n<MAX_LIGHTS;n++)
		flareFade[n]=0.f;
}
void F_cframenext(float dt){
	int n;
	for(n=0;n<mp->lights+1;n++){
		vec3_t cent;
		if(n<mp->lights){
			cent=mp->lightPos[n];
		}else if(n==mp->lights){
			cent=camera_from+mp->sunPos*1000.f;
		}
		
		// margin
		cent+=(camera_from-cent).normalize()*.01f;
		
		// raycast from camera to client
		vector<isect_t> *lst=mp->m->raycast(camera_from, cent);
		bool visible=!lst->size();
		delete lst;
		
		if(visible){
			flareFade[n]+=dt*4.f;
			if(flareFade[n]>1.f)
				flareFade[n]=1.f;
		}else{
			flareFade[n]-=dt*4.f;
			if(flareFade[n]<0.f)
				flareFade[n]=0.f;
		}
		//flareFade[n]=1.f;
	}
	
}
void F_render(){
	
	GLint viewport[4];
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	osdEnter2D();
	
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	int n;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glBindTexture(GL_TEXTURE_2D, tex_flare);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glBegin(GL_QUADS);
	
	float cx, cy, px, py;
	cx=(float)(screen->w)*.5f;
	cy=(float)(screen->h)*.5f;
	
	for(n=0;n<mp->lights+1;n++){
		if(flareFade[n]<.01f)
			continue;
		double xx, yy, zz;
		
		float siz;
		vec3_t v;
		if(n<mp->lights){
			v=mp->lightPos[n];
		}else if(n==mp->lights){
			v=camera_from+mp->sunPos*10000.f;
		}
		gluProject(v.x, v.y, v.z, osd_old_modelview, osd_old_proj, viewport, &xx, &yy, &zz);
		if(zz>1.)
			continue;
		yy=(double)screen->h-yy;
		
		float dist;
		float fade;
		dist=vec3_t::dot((camera_at-camera_from).normalize(), v-camera_from);
		
		siz=(float)(viewport[2]-viewport[0]);
		
		vec3_t col;
		if(n<mp->lights){
			col=mp->lightColor[n];
			fade=flareFade[n]*max(col.x, max(col.y, col.z));		
			if(dist>.3f)
				fade/=dist/.3f;
			if(fade>1.f)
				fade=1.f;
			col/=max(col.x, max(col.y, col.z));
		}else if(n==mp->lights){
			col=mp->sunColor;
			fade=flareFade[n]*max(col.x, max(col.y, col.z));
		}
		
		if(fade<.01f)
			continue;
		
		
		
		// flare
		
		glColor4f(col.x, col.y, col.z, fade*.5f);
		siz=(float)(viewport[2]-viewport[0])*.2f;
		px=(xx-cx)*1.f+cx; py=(yy-cy)*1.f+cy;
		glTexCoord2f(0.f, 0.f);
		glVertex2f(px-siz, py-siz);
		glTexCoord2f(0.5f, 0.f);
		glVertex2f(px+siz, py-siz);
		glTexCoord2f(0.5f, 0.5f);
		glVertex2f(px+siz, py+siz);
		glTexCoord2f(0.f, 0.5f);
		glVertex2f(px-siz, py+siz);
	
		glColor4f(col.x, col.y, col.z, fade*2.f);
		siz=(float)(viewport[2]-viewport[0])*.06f;
		px=(xx-cx)*1.f+cx; py=(yy-cy)*1.f+cy;
		glTexCoord2f(0.f, 0.f);
		glVertex2f(px-siz, py-siz);
		glTexCoord2f(0.5f, 0.f);
		glVertex2f(px+siz, py-siz);
		glTexCoord2f(0.5f, 0.5f);
		glVertex2f(px+siz, py+siz);
		glTexCoord2f(0.f, 0.5f);
		glVertex2f(px-siz, py+siz);
		
		glColor4f(col.x, col.y, col.z, fade*16.f);
		siz=(float)(viewport[2]-viewport[0])*.02f;
		px=(xx-cx)*1.f+cx; py=(yy-cy)*1.f+cy;
		glTexCoord2f(0.f, 0.f);
		glVertex2f(px-siz, py-siz);
		glTexCoord2f(0.5f, 0.f);
		glVertex2f(px+siz, py-siz);
		glTexCoord2f(0.5f, 0.5f);
		glVertex2f(px+siz, py+siz);
		glTexCoord2f(0.f, 0.5f);
		glVertex2f(px-siz, py+siz);
		
		glColor4f(col.x, col.y, col.z, fade*0.1f);
		siz=(float)(viewport[2]-viewport[0])*.2f;
		px=(xx-cx)*1.f+cx; py=(yy-cy)*1.f+cy;
		glTexCoord2f(0.5f, 0.f);
		glVertex2f(px-siz, py-siz);
		glTexCoord2f(1.0f, 0.f);
		glVertex2f(px+siz, py-siz);
		glTexCoord2f(1.0f, 0.5f);
		glVertex2f(px+siz, py+siz);
		glTexCoord2f(0.5f, 0.5f);
		glVertex2f(px-siz, py+siz);
		
		glColor4f(col.x*0.1f, col.y*0.1f, col.z*0.5f, fade*.2f);
		siz=(float)(viewport[2]-viewport[0])*.06f;
		px=(xx-cx)*.2f+cx; py=(yy-cy)*.2f+cy;
		glTexCoord2f(0.0f, 0.5f);
		glVertex2f(px-siz, py-siz);
		glTexCoord2f(0.5f, 0.5f);
		glVertex2f(px+siz, py-siz);
		glTexCoord2f(0.5f, 1.0f);
		glVertex2f(px+siz, py+siz);
		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(px-siz, py+siz);
		
		glColor4f(col.x*0.1f, col.y*0.1f, col.z*0.5f, fade*.2f);
		siz=(float)(viewport[2]-viewport[0])*.08f;
		px=(xx-cx)*.15f+cx; py=(yy-cy)*.15f+cy;
		glTexCoord2f(0.5f, 0.5f);
		glVertex2f(px-siz, py-siz);
		glTexCoord2f(1.0f, 0.5f);
		glVertex2f(px+siz, py-siz);
		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(px+siz, py+siz);
		glTexCoord2f(0.5f, 1.0f);
		glVertex2f(px-siz, py+siz);
		
		glColor4f(col.x*0.0f, col.y*0.5f, col.z*0.3f, fade*.15f);
		siz=(float)(viewport[2]-viewport[0])*.15f;
		px=(xx-cx)*-.8f+cx; py=(yy-cy)*-.8f+cy;
		glTexCoord2f(0.0f, 0.5f);
		glVertex2f(px-siz, py-siz);
		glTexCoord2f(0.5f, 0.5f);
		glVertex2f(px+siz, py-siz);
		glTexCoord2f(0.5f, 1.0f);
		glVertex2f(px+siz, py+siz);
		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(px-siz, py+siz);
		
		glColor4f(col.x*0.0f, col.y*1.0f, col.z*0.6f, fade*0.5f);
		siz=(float)(viewport[2]-viewport[0])*.02f;
		px=(xx-cx)*-.75f+cx; py=(yy-cy)*-.75f+cy;
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(px-siz, py-siz);
		glTexCoord2f(0.5f, 0.0f);
		glVertex2f(px+siz, py-siz);
		glTexCoord2f(0.5f, 0.5f);
		glVertex2f(px+siz, py+siz);
		glTexCoord2f(0.0f, 0.5f);
		glVertex2f(px-siz, py+siz);
		
		glColor4f(col.x*0.7f, col.y*0.6f, col.z*0.6f, fade*0.5f);
		siz=(float)(viewport[2]-viewport[0])*.02f;
		px=(xx-cx)*-.79f+cx; py=(yy-cy)*-.79f+cy;
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(px-siz, py-siz);
		glTexCoord2f(0.5f, 0.0f);
		glVertex2f(px+siz, py-siz);
		glTexCoord2f(0.5f, 0.5f);
		glVertex2f(px+siz, py+siz);
		glTexCoord2f(0.0f, 0.5f);
		glVertex2f(px-siz, py+siz);
		
		totalPolys+=9;
		
		
	}
	glEnd();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}

