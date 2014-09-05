/*
 *  tonemap.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/12/06.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "tonemap.h"


#if GL_ARB_imaging
#if GL_EXT_framebuffer_object
#if GL_ARB_shader_objects
#if GL_ARB_multitexture

GLuint tex_screen, tex_blur, tex_blurMB;
static int sw, sh;
float TM_exposure=1.f;
static GLhandleARB prg_toneMap;
static GLhandleARB prg_toneBlur;


const int tm_downSample=4;

void TM_init(){
	sw=larger_pow2(screen->w);
	sh=larger_pow2(screen->h);
	
	GLint mx;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mx);
	
	consoleLog("TM_init: required texture size is %dx%d\n", sw, sh);
	
	if(sw>mx || sh>mx){
		consoleLog("TM_init: too big texture, disabling tone map\n");
		cap_toneMap=false;
		use_toneMap=false;
		return;
	}
	
	consoleLog("TM_init: allocating screen buffer\n");
	
	glGenTextures(1, &tex_screen);
	glBindTexture(GL_TEXTURE_2D, tex_screen);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, 
				 sw,sh, 0,
				 GL_RGB, GL_HALF_FLOAT_ARB, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	clearTexture(sw, sh);
	
	
	consoleLog("TM_init: blur buffer allocating\n");
	
	
	glGenTextures(1, &tex_blur);
	glBindTexture(GL_TEXTURE_2D, tex_blur);
	

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, 
				 sw/tm_downSample,sh/tm_downSample, 0,
				 GL_RGB, GL_HALF_FLOAT_ARB, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	clearTexture(sw/tm_downSample, sh/tm_downSample);
	
	consoleLog("TM_init: blur MB buffer allocating\n");
	
	
	glGenTextures(1, &tex_blurMB);
	glBindTexture(GL_TEXTURE_2D, tex_blurMB);
	
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, 
				 sw/tm_downSample,sh/tm_downSample, 0,
				 GL_RGB, GL_HALF_FLOAT_ARB, NULL);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	clearTexture(sw/tm_downSample, sh/tm_downSample);
	
	if(cap_glsl){
		prg_toneMap=create_program("res/shaders/tonemap.vs", "res/shaders/tonemap.fs");
		if(prg_toneMap)
			consoleLog("TM_init: compiled program \"tonemap\"\n");
		else
			consoleLog("TM_init: couldn't compile program \"tonemap\"\n");
		
		prg_toneBlur=create_program("res/shaders/toneblur.vs", "res/shaders/toneblur.fs");
		if(prg_toneBlur)
			consoleLog("TM_init: compiled program \"toneblur\"\n");
		else
			consoleLog("TM_init: couldn't compile program \"toneblur\"\n");
	}else{
		consoleLog("TM_init: doesn't support ARB_shader_objects, disabling tone mapping\n");
		cap_toneMap=false;
		use_toneMap=false;
	}
}

void TM_apply(){
	if(!use_toneMap)
		return;	// disabled
	if(!use_hdr)
		return; // impossible
	
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
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
	glBindTexture(GL_TEXTURE_2D, tex_screen);
	
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, screen->w, screen->h);
	
	glColor4f(1.f, 1.f, 1.f, 1.f);
	
	// downsample scene
	glUseProgramObjectARB(prg_toneBlur);
	
	int bw, bh;
	int tw, th;
	
	tw=sw/tm_downSample;
	th=sh/tm_downSample;
	bw=screen->w/tm_downSample;
	bh=screen->h/tm_downSample;
	
	glUniform1iARB(glGetUniformLocationARB(prg_toneBlur, "tex"),
				   0);
	glUniform2fARB(glGetUniformLocationARB(prg_toneBlur, "blurSize"),
				   .25f/(float)sw, .25/(float)sh);
	
	glBindTexture(GL_TEXTURE_2D, tex_blurMB);
	
	glBegin(GL_QUADS);
	glTexCoord2f(0.f, (float)bh/(float)th);
	glVertex2i(0, screen->h-bh);
	glTexCoord2f(0.f, 0.f);
	glVertex2i(0, screen->h);
	glTexCoord2f((float)bw/(float)tw, 0.f);
	glVertex2i(bw, screen->h);;
	glTexCoord2f((float)bw/(float)tw, (float)bh/(float)th);
	glVertex2i(bw, screen->h-bh);
	glEnd();
	totalPolys+=2;
	
	glBindTexture(GL_TEXTURE_2D, tex_screen);
	
	glColor4f(1.f, 1.f, 1.f, .05f);
	
	glBegin(GL_QUADS);
	glTexCoord2f(0.f, (float)screen->h/sh);
	glVertex2i(0, screen->h-bh);
	glTexCoord2f(0.f, 0.f);
	glVertex2i(0, screen->h);
	glTexCoord2f((float)screen->w/sw, 0.f);
	glVertex2i(bw, screen->h);;
	glTexCoord2f((float)screen->w/sw, (float)screen->h/sh);
	glVertex2i(bw, screen->h-bh);
	glEnd();
	totalPolys+=2;
	
	glColor4f(1.f, 1.f, 1.f, 1.f);
	
	// copy to blurMB
	glBindTexture(GL_TEXTURE_2D, tex_blurMB);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, bw, bh);
	
	glUniform2fARB(glGetUniformLocationARB(prg_toneBlur, "blurSize"),
				   .25f/(float)tw, .25/(float)th);

	glBindTexture(GL_TEXTURE_2D, tex_blur);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, bw, bh);
	
	
	
	while(bw!=1 || bh!=1){
		int nw, nh; // calc next size
		nw=(bw+tm_downSample-1)/tm_downSample;
		nh=(bh+tm_downSample-1)/tm_downSample;
		
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glUniform2fARB(glGetUniformLocationARB(prg_toneBlur, "blurSize"),
					   .25f*((float)bw/tw)/(float)nw, .25*((float)bh/th)/(float)nh);
		
		glBegin(GL_QUADS);
		glTexCoord2f(0.f, (float)bh/(float)th);
		glVertex2i(0, screen->h-nh);
		glTexCoord2f(0.f, 0.f);
		glVertex2i(0, screen->h);
		glTexCoord2f((float)bw/tw, 0.f);
		glVertex2i(nw, screen->h);;
		glTexCoord2f((float)bw/tw, (float)bh/th);
		glVertex2i(nw, screen->h-nh);
		glEnd();
		totalPolys+=2;
		
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, nw, nh);
		
		bw=nw;
		bh=nh;
		
	}

	// render scene
	
	glBindTexture(GL_TEXTURE_2D, tex_screen);
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glBindTexture(GL_TEXTURE_2D, tex_blur);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	
	glUseProgramObjectARB(prg_toneMap);
	glUniform1iARB(glGetUniformLocationARB(prg_toneMap, "texSrc"),
				   0);
	glUniform1iARB(glGetUniformLocationARB(prg_toneMap, "texBlur"),
				   1);
	glUniform2fARB(glGetUniformLocationARB(prg_toneMap, "maxSrc"),
				   (float)(0)/(float)(tw), (float)(bh-1)/(float)th);
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
	glUseProgramObjectARB(0);
	
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

#else

void TM_init(){
	consoleLog("TM_init: tone map is not compiled\n");
}

void TM_apply(){
	
}

#endif
#else

void TM_init(){
	consoleLog("TM_init: tone map is not compiled\n");
}

void TM_apply(){
	
}

#endif
#else

void TM_init(){
	consoleLog("TM_init: tone map is not compiled\n");
}

void TM_apply(){
	
}

#endif
#else

void TM_init(){
	consoleLog("TM_init: tone map is not compiled\n");
}

void TM_apply(){
	
}

#endif
