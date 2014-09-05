/*
 *  dof.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/30.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "dof.h"

extern SDL_Surface *screen;
static GLuint tex_screen;
static GLuint tex_blur;
#if GL_ARB_depth_texture
static GLuint tex_depth;
#endif
#if GL_ARB_shader_objects
static GLhandleARB prg_dof;
#endif
static int bw, bh;
static int sw, sh;



static float dof_at=120.f;
static int dof_smooth=2;
dof_method_t dof=DM_none;

void D_at(float d){
	dof_at=d;
}

void D_init(){
	
	dof=DM_none;
	bw=512; bh=512;
	sw=larger_pow2(screen->w);
	sh=larger_pow2(screen->h);
	
	GLint mx;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mx);
	
	consoleLog("D_init: required texture size is %dx%d\n", sw, sh);
	
	if(sw>mx || sh>mx){
		consoleLog("D_init: too big texture, disabling dof\n");
		dof=DM_none;
		return;
	}
	
	consoleLog("D_init: allocating screen buffer\n");
	
	glGenTextures(1, &tex_screen);
	glBindTexture(GL_TEXTURE_2D, tex_screen);
#if GL_EXT_framebuffer_object
	if(use_hdr){
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, 
					 sw,sh, 0,
					 GL_RGB, GL_HALF_FLOAT_ARB, NULL);
	}else{
#endif
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 
					 sw,sh, 0,
					 GL_RGB, GL_UNSIGNED_BYTE, NULL);
#if GL_EXT_framebuffer_object
	}
#endif
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	clearTexture(sw, sh);
	
	
	consoleLog("D_init: allocating depth buffer\n");
	
#if GL_ARB_depth_texture
	glGenTextures(1, &tex_depth);
	glBindTexture(GL_TEXTURE_2D, tex_depth);
	int comp;
	GLint i;
	glGetIntegerv(GL_DEPTH_BITS, &i);
	switch(i){
		case 16:
			comp=GL_DEPTH_COMPONENT16_ARB;
			break;
		case 24:
			comp=GL_DEPTH_COMPONENT24_ARB;
			break;
		case 32:
			comp=GL_DEPTH_COMPONENT32_ARB;
			break;
		default:
			comp=GL_DEPTH_COMPONENT;
			break;
	}
	comp=GL_DEPTH_COMPONENT;
	if(use_hdr)
		comp=GL_DEPTH_COMPONENT24_ARB;
	
	glTexImage2D(GL_TEXTURE_2D, 0, comp, 
				 sw,sh, 0,
				 GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#endif

	consoleLog("D_init: blur buffer allocating\n");

	
	glGenTextures(1, &tex_blur);
	glBindTexture(GL_TEXTURE_2D, tex_blur);

#if GL_EXT_framebuffer_object
	if(use_hdr){
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, 
					 bw,bh, 0,
					 GL_RGB, GL_HALF_FLOAT_ARB, NULL);
	}else{
#endif
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 
					 bw,bh, 0,
					 GL_RGB, GL_UNSIGNED_BYTE, NULL);
#if GL_EXT_framebuffer_object
	}
#endif
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
		
#if GL_ARB_shader_objects
	if(cap_glsl){
		prg_dof=create_program("res/shaders/dof.vs", "res/shaders/dof.fs");
		if(prg_dof)
			consoleLog("D_init: compiled program \"dof\"\n");
		else
			consoleLog("D_init: couldn't compile program \"dof\"\n");
	}else{
#endif
	
		consoleLog("D_init: no programs to compile\n");

#if GL_ARB_shader_objects
	}
#endif

	
}
void D_apply(){
	int n;
	
	if(multiSamples!=1 && dof!=DM_none){
		consoleLog("D_apply: DOF can't be used with multisample, disabling");
		dof=DM_none;
	}
	
	if(dof==DM_none)
		return;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glScalef(2.f/screen->w, -2.f/screen->h, 1.f);
	glTranslatef(-screen->w*.5f, -screen->h*.5f, 0.f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
	glDisable(GL_LIGHTING);
	glDepthMask(GL_FALSE);
#if GL_ARB_shader_objects
	if(dof==DM_glsl){
		glBindTexture(GL_TEXTURE_2D, tex_depth);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, screen->w, screen->h);
	}
#endif
	glBindTexture(GL_TEXTURE_2D, tex_screen);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, screen->w, screen->h);
	
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glBegin(GL_QUADS);
	glTexCoord2f(0.f, (float)screen->h/sh);
	glVertex2i(0, screen->h-bh);
	glTexCoord2f(0.f, 0.f);
	glVertex2i(0, screen->h);
	glTexCoord2f((float)screen->w/sw, 0.f);
	glVertex2i(bw, screen->h);
	glTexCoord2f((float)screen->w/sw, (float)screen->h/sh);
	glVertex2i(bw, screen->h-bh);
	glEnd();
	totalPolys+=2;
	
	glBindTexture(GL_TEXTURE_2D, tex_blur);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, bw, bh);
	for(n=0;n<dof_smooth;n++){
		
		//clear
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.f, 0.f, 0.f, 1.f);
		glBegin(GL_QUADS);
		glVertex2i(0, screen->h-bh);
		glVertex2i(0, screen->h);
		glVertex2i(bw, screen->h);
		glVertex2i(bw, screen->h-bh);
		glEnd();
		glEnable(GL_TEXTURE_2D);
		totalPolys+=2;
		
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		
		
		glBegin(GL_QUADS);
		glColor4f(1.f, 1.f, 1.f, 6.f/16.f);
		glTexCoord2f(0.f, 1.f);
		glVertex2i(0, screen->h-bh);
		glTexCoord2f(0.f, 0.f);
		glVertex2i(0, screen->h);
		glTexCoord2f(1.f, 0.f);
		glVertex2i(bw, screen->h);
		glTexCoord2f(1.f, 1.f);
		glVertex2i(bw, screen->h-bh);
		glColor4f(1.f, 1.f, 1.f, 5.f/16.f);
		glTexCoord2f(0.f, 1.f);
		glVertex2f(-1.25f, screen->h-bh);
		glTexCoord2f(0.f, 0.f);
		glVertex2f(-1.25f, screen->h);
		glTexCoord2f(1.f, 0.f);
		glVertex2f(bw-1.25f, screen->h);
		glTexCoord2f(1.f, 1.f);
		glVertex2f(bw-1.25f, screen->h-bh);
		glTexCoord2f(0.f, 1.f);
		glVertex2f(1.25f, screen->h-bh);
		glTexCoord2f(0.f, 0.f);
		glVertex2f(1.25f, screen->h);
		glTexCoord2f(1.f, 0.f);
		glVertex2f(bw+1.25f, screen->h);
		glTexCoord2f(1.f, 1.f);
		glVertex2f(bw+1.25f, screen->h-bh);
		glEnd();
		totalPolys+=6;
		
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, bw, bh);
		
		
		
		//clear
		glDisable(GL_TEXTURE_2D);
		glColor4f(0.f, 0.f, 0.f, 1.f);
		glBegin(GL_QUADS);
		glVertex2i(0, screen->h-bh);
		glVertex2i(0, screen->h);
		glVertex2i(bw, screen->h);
		glVertex2i(bw, screen->h-bh);
		glEnd();
		totalPolys+=2;
		glEnable(GL_TEXTURE_2D);
		
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		
		
		glBegin(GL_QUADS);
		glColor4f(1.f, 1.f, 1.f, 6.f/16.f);
		glTexCoord2f(0.f, 1.f);
		glVertex2i(0, screen->h-bh);
		glTexCoord2f(0.f, 0.f);
		glVertex2i(0, screen->h);
		glTexCoord2f(1.f, 0.f);
		glVertex2i(bw, screen->h);
		glTexCoord2f(1.f, 1.f);
		glVertex2i(bw, screen->h-bh);
		glColor4f(1.f, 1.f, 1.f, 5.f/16.f);
		glTexCoord2f(0.f, 1.f);
		glVertex2f(0.f, screen->h-bh-1.25f);
		glTexCoord2f(0.f, 0.f);
		glVertex2f(0.f, screen->h-1.25f);
		glTexCoord2f(1.f, 0.f);
		glVertex2f(bw, screen->h-1.25f);
		glTexCoord2f(1.f, 1.f);
		glVertex2f(bw, screen->h-bh-1.25f);
		glTexCoord2f(0.f, 1.f);
		glVertex2f(0.f, screen->h-bh+1.25f);
		glTexCoord2f(0.f, 0.f);
		glVertex2f(0.f, screen->h+1.25f);
		glTexCoord2f(1.f, 0.f);
		glVertex2f(bw, screen->h+1.25f);
		glTexCoord2f(1.f, 1.f);
		glVertex2f(bw, screen->h-bh+1.25f);
		glEnd();
		totalPolys+=6;
		
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, bw, bh);
	}
	
	glColor4f(1.f, 1.f, 1.f, 1.f);
	
	if(dof==DM_basic){
		// restore screen
		glBindTexture(GL_TEXTURE_2D, tex_screen);
		glBegin(GL_QUADS);
		glTexCoord2f(0.f, (float)screen->h/sh);
		glVertex2i(0, 0);
		glTexCoord2f(0.f, 0.f);
		glVertex2i(0, screen->h);
		glTexCoord2f((float)screen->w/sw, 0.f);
		glVertex2i(screen->w, screen->h);
		glTexCoord2f((float)screen->w/sw, (float)screen->h/sh);
		glVertex2i(screen->w, 0);
		glEnd();
		totalPolys+=2;
		
		float zz;
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_GREATER);
		
		
		
		zz=1.f-1.f/5.f; // near blur
		
		glColor4f(1.f, 1.f, 1.f, 1.f);
		glBindTexture(GL_TEXTURE_2D, tex_blur);
		glBegin(GL_QUADS);
		glTexCoord2i(0, 1);
		glVertex3f(0, 0, zz);
		glTexCoord2i(0, 0);
		glVertex3f(0, screen->h, zz);
		glTexCoord2i(1, 0);
		glVertex3f(screen->w, screen->h, zz);
		glTexCoord2i(1, 1);
		glVertex3f(screen->w, 0, zz);
		glEnd();
		totalPolys+=2;
		glDepthFunc(GL_LESS);
		
		zz=1.f-1.f/dof_at*1.3f; // far blur
		
		glColor4f(1.f, 1.f, 1.f, .5f);
		glBindTexture(GL_TEXTURE_2D, tex_blur);
		glBegin(GL_QUADS);
		glTexCoord2i(0, 1);
		glVertex3f(0, 0, zz);
		glTexCoord2i(0, 0);
		glVertex3f(0, screen->h, zz);
		glTexCoord2i(1, 0);
		glVertex3f(screen->w, screen->h, zz);
		glTexCoord2i(1, 1);
		glVertex3f(screen->w, 0, zz);
		glEnd();
		totalPolys+=2;
		
		zz=1.f-1.f/dof_at; // far blur
		
		glColor4f(1.f, 1.f, 1.f, 1.f);
		glBindTexture(GL_TEXTURE_2D, tex_blur);
		glBegin(GL_QUADS);
		glTexCoord2i(0, 1);
		glVertex3f(0, 0, zz);
		glTexCoord2i(0, 0);
		glVertex3f(0, screen->h, zz);
		glTexCoord2i(1, 0);
		glVertex3f(screen->w, screen->h, zz);
		glTexCoord2i(1, 1);
		glVertex3f(screen->w, 0, zz);
		glEnd();
		totalPolys+=2;
	}
#if GL_ARB_shader_objects
	else if(dof==DM_glsl){
		// restore screen
		glBindTexture(GL_TEXTURE_2D, tex_screen);
		glBegin(GL_QUADS);
		glTexCoord2f(0.f, (float)screen->h/sh);
		glVertex2i(0, 0);
		glTexCoord2f(0.f, 0.f);
		glVertex2i(0, screen->h);
		glTexCoord2f((float)screen->w/sw, 0.f);
		glVertex2i(screen->w, screen->h);
		glTexCoord2f((float)screen->w/sw, (float)screen->h/sh);
		glVertex2i(screen->w, 0);
		glEnd();
		totalPolys+=2;
		
		glUseProgramObjectARB(prg_dof);
		glUniform1fARB(glGetUniformLocationARB(prg_dof, "depthCenter"),
					dof_at);
		glUniform1fARB(glGetUniformLocationARB(prg_dof, "depthRangeI"),
					1.f/dof_at/4.f);
		glUniform1iARB(glGetUniformLocationARB(prg_dof, "tex"),
					0);
		glUniform1iARB(glGetUniformLocationARB(prg_dof, "depth"),
					1);
		
		glActiveTexture(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, tex_depth);
		glActiveTexture(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_2D, tex_blur);
		
		glBegin(GL_QUADS);
		
		glTexCoord2i(0, 1);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.f, (float)screen->h/sh);
		glVertex2i(0, 0);
		glTexCoord2i(0, 0);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.f, 0.f);
		glVertex2i(0, screen->h);
		glTexCoord2i(1, 0);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB, (float)screen->w/sw, 0.f);
		glVertex2i(screen->w, screen->h);
		glTexCoord2i(1, 1);
		glMultiTexCoord2fARB(GL_TEXTURE1_ARB, (float)screen->w/sw, (float)screen->h/sh);
		glVertex2i(screen->w, 0);
		totalPolys+=2;
		
		glEnd();
		
		glUseProgramObjectARB(0);
	}
#endif
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LIGHTING);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_FOG);
	glDepthMask(GL_TRUE);

}
