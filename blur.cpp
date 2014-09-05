/*
 *  blur.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/31.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "blur.h"

extern SDL_Surface *screen;
static GLuint tex_screen;
static int sw, sh;
extern GLfloat mx_old[16];
#if GL_ARB_shader_objects
static GLhandleARB prg_blurSimple;
#if GL_ARB_depth_texture
static GLhandleARB prg_blurDepth;
static GLuint tex_depth;
#endif
#endif

blur_t blur;


void Blur_init(){
	blur=Blur_basic;
	sw=larger_pow2(screen->w);
	sh=larger_pow2(screen->h);
	
	GLint mx;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mx);
	
	consoleLog("Blur_init: required texture size is %dx%d\n", sw, sh);
	
	if(sw>mx || sh>mx){
		consoleLog("Blur_init: too big texture, disabling blur\n");
		blur=Blur_none;
		return;
	}
	
	consoleLog("Blur_init: allocating screen buffer\n");
	
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
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor(0.f, 0.f, 0.0f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, screen->w, screen->h);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	clearTexture(sw, sh);
	
#if GL_ARB_shader_objects
#if GL_ARB_depth_texture
	
	if(cap_glsl){
		
		consoleLog("Blur_init: allocating depth buffer\n");
		
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
		
	}
#endif
#endif
	
#if GL_ARB_shader_objects
	
	if(cap_glsl){
	
		prg_blurSimple=create_program("res/shaders/blur1.vs", "res/shaders/blur1.fs");
		if(prg_blurSimple)
			consoleLog("Blur_init: compiled program \"blur1\"\n");
		else
			consoleLog("Blur_init: couldn't compile program \"blur1\"\n");
	
#if GL_ARB_depth_texture
		prg_blurDepth=create_program("res/shaders/blur2.vs", "res/shaders/blur2.fs");
		if(prg_blurDepth)
			consoleLog("Blur_init: compiled program \"blur2\"\n");
		else
			consoleLog("Blur_init: couldn't compile program \"blur2\"\n");
	
#endif
		
	}else{
		
#endif
	
		consoleLog("Blur_init: no programs to compile\n");
		
#if GL_ARB_shader_objects

	}
		
#endif
	
#if GL_ARB_shader_objects
	if(cap_glsl){
		blur=Blur_glsl_simple;
#if GL_ARB_depth_texture
		blur=Blur_glsl_depth;
#endif
	}
#endif

}
float blurdt=1.f;
void Blur_apply(){
	
	if(multiSamples!=1 && blur==Blur_glsl_depth){
		consoleLog("Blur_apply: Blur_glsl_depth can't be used with multisample, disabling\n");
		blur=Blur_glsl_simple;
	}
	
	if(blur==Blur_none)
		return;
	
	float lastMatrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, lastMatrix);
	
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
	
	if(blur==Blur_basic){
		
		glColor4f(1.f, 1.f, 1.f, powf(.3f, blurdt*60.f));
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
		
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, screen->w, screen->h);
	}
#if GL_ARB_shader_objects
	else if(blur==Blur_glsl_simple){
		glUseProgramObjectARB(prg_blurSimple);
		glUniform1iARB(glGetUniformLocationARB(prg_blurSimple, "screen"),
					   0);
		glUniform2fARB(glGetUniformLocationARB(prg_blurSimple, "texSize"),
					   (float)screen->w/sw, (float)screen->h/sh);
		float imat[16];
		float mat2[16];
		memcpy(imat, mx_old, sizeof(imat));
		inverseMatrix4(imat);
		glUniformMatrix4fvARB(glGetUniformLocationARB(prg_blurSimple, "curMatrix"),
							  1, GL_FALSE, lastMatrix);
		glUniformMatrix4fvARB(glGetUniformLocationARB(prg_blurSimple, "oldMatrix"),
							  1, GL_FALSE, mx_old);
		inverseMatrix4(lastMatrix);
		glUniformMatrix4fvARB(glGetUniformLocationARB(prg_blurSimple, "oldMatrixInverse"),
					   1, GL_FALSE, imat);
		glUniformMatrix4fvARB(glGetUniformLocationARB(prg_blurSimple, "curMatrixInverse"),
							  1, GL_FALSE, lastMatrix);
		glColor4f(1,1,1,1);
		glBindTexture(GL_TEXTURE_2D, tex_screen);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, screen->w, screen->h);
		glBegin(GL_QUADS);
		glTexCoord2f(-1.f, 1.f);
		glVertex2i(0, 0);
		glTexCoord2f(-1.f, -1.f);
		glVertex2i(0, screen->h);
		glTexCoord2f(1.f, -1.f);
		glVertex2i(screen->w, screen->h);
		glTexCoord2f(1.f, 1.f);
		glVertex2i(screen->w, 0);
		glEnd();
		totalPolys+=2;
		glUseProgramObjectARB(0);
	}
#if GL_ARB_depth_texture
	else if(blur==Blur_glsl_depth){
		
		glActiveTexture(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, tex_depth);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, screen->w, screen->h);
		glActiveTexture(GL_TEXTURE0_ARB);
		
		glUseProgramObjectARB(prg_blurDepth);
		glUniform1iARB(glGetUniformLocationARB(prg_blurDepth, "screen"),
					   0);
		glUniform1iARB(glGetUniformLocationARB(prg_blurDepth, "depthTex"),
					   1);
		glUniform2fARB(glGetUniformLocationARB(prg_blurDepth, "texSize"),
					   (float)screen->w/sw, (float)screen->h/sh);
		float imat[16];
		float mat2[16];
		memcpy(imat, mx_old, sizeof(imat));
		inverseMatrix4(imat);
		glUniformMatrix4fvARB(glGetUniformLocationARB(prg_blurDepth, "curMatrix"),
							  1, GL_FALSE, lastMatrix);
		glUniformMatrix4fvARB(glGetUniformLocationARB(prg_blurDepth, "oldMatrix"),
							  1, GL_FALSE, mx_old);
		inverseMatrix4(lastMatrix);
		glUniformMatrix4fvARB(glGetUniformLocationARB(prg_blurDepth, "oldMatrixInverse"),
							  1, GL_FALSE, imat);
		glUniformMatrix4fvARB(glGetUniformLocationARB(prg_blurDepth, "curMatrixInverse"),
							  1, GL_FALSE, lastMatrix);
		glColor4f(1,1,1,1);
		glBindTexture(GL_TEXTURE_2D, tex_screen);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, screen->w, screen->h);
		glBegin(GL_QUADS);
		glTexCoord2f(-1.f, 1.f);
		glVertex2i(0, 0);
		glTexCoord2f(-1.f, -1.f);
		glVertex2i(0, screen->h);
		glTexCoord2f(1.f, -1.f);
		glVertex2i(screen->w, screen->h);
		glTexCoord2f(1.f, 1.f);
		glVertex2i(screen->w, 0);
		glEnd();
		totalPolys+=2;
		glUseProgramObjectARB(0);
	}
#endif
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

void Blur_framenext(float dt){
	blurdt=dt;
}
