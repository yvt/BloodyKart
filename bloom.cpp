/*
 *  bloom.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/30.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "bloom.h"
#include "map.h"

extern SDL_Surface *screen;
bloom_method_t bloom=BM_none;
bloom_style_t bloom_style=BS_basic;
bloom_quality_t bloom_quality=BQ_high;
static GLuint tex_screen;
static GLuint tex_bloom;
static GLuint tex_depth;

#if GL_ARB_shader_objects
static GLhandleARB prg_bloom1, prg_bloom2, prg_bloom3, prg_bloom4;
#endif

#if GL_ARB_texture_rectangle

static int bloom_darken=1;
static int bloom_smooth=2;
static const int bloom_downsample=8;
static float bloom_dry=.1f;
static float bloom_nearDry=.2f;
static float bloom_wet=1.f;

static int sw, sh;


void B_init(){
	
	sw=larger_pow2(screen->w);
	sh=larger_pow2(screen->h);
	
	GLint mx;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mx);
	
	consoleLog("B_init: required texture size is %dx%d\n", sw, sh);
	
	if(sw>mx || sh>mx){
		consoleLog("B_init: too big texture, disabling bloom\n");
		bloom=BM_none;
		return;
	}
	
	consoleLog("B_init: allocating screen buffer\n");
	
	glEnable(GL_TEXTURE_2D);
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
	
	consoleLog("B_init: allocating bloom buffer\n");
	
	glGenTextures(1, &tex_bloom);
	glBindTexture(GL_TEXTURE_2D, tex_bloom);
#if GL_EXT_framebuffer_object
	if(use_hdr){
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, 
					 sw/bloom_downsample, sh/bloom_downsample, 0,
					 GL_RGB, GL_HALF_FLOAT_ARB, NULL);
	}else{
#endif
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 
					 sw/bloom_downsample, sh/bloom_downsample, 0,
					 GL_RGB, GL_UNSIGNED_BYTE, NULL);
#if GL_EXT_framebuffer_object
	}
#endif
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	clearTexture(sw/bloom_downsample, sh/bloom_downsample);
	
	
	consoleLog("B_init: allocating depth buffer\n");
	
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
	
	
	clearTexture(sw/bloom_downsample, sh/bloom_downsample);
	glDisable(GL_TEXTURE_2D);
	
#if GL_ARB_shader_objects
	if(cap_glsl){
		prg_bloom1=create_program("res/shaders/bloom.vs", "res/shaders/bloom1.fs");
		if(prg_bloom1)
			consoleLog("B_init: compiled program \"bloom1\"\n");
		else
			consoleLog("B_init: couldn't compile program \"bloom1\"\n");
		prg_bloom2=create_program("res/shaders/bloom.vs", "res/shaders/bloom2.fs");
		if(prg_bloom2)
			consoleLog("B_init: compiled program \"bloom2\"\n");
		else
			consoleLog("B_init: couldn't compile program \"bloom2\"\n");
		prg_bloom3=create_program("res/shaders/bloom.vs", "res/shaders/bloom3.fs");
		if(prg_bloom3)
			consoleLog("B_init: compiled program \"bloom3\"\n");
		else
			consoleLog("B_init: couldn't compile program \"bloom3\"\n");
		prg_bloom4=create_program("res/shaders/bloom.vs", "res/shaders/bloom4.fs");
		if(prg_bloom4)
			consoleLog("B_init: compiled program \"bloom4\"\n");
		else
			consoleLog("B_init: couldn't compile program \"bloom4\"\n");
	}else{
#endif
		consoleLog("B_init: no programs to compile\n");

#if GL_ARB_shader_objects
	}
#endif
	
	bloom=BM_overlap;
	bloom_style=BS_basic;
	bloom_quality=BQ_high;
	
#if GL_ARB_shader_objects
	if(use_glsl)
		bloom=BM_glsl;
#endif
}
void B_apply(){
	int n, x, y;
	if(bloom==BM_none)
		return;
	
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glScalef(2.f/screen->w, -2.f/screen->h, 1.f);
	glTranslatef(-screen->w*.5f, -screen->h*.5f, 0.f);
	glMatrixMode(GL_TEXTURE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
	glDepthMask(GL_FALSE);
	
	
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glBindTexture(GL_TEXTURE_2D, tex_screen);
	glPushMatrix();
	glScalef(1.f/(float)sw, 1.f/(float)sh, 1.f);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 
					 screen->w, screen->h);
	
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PRIMARY_COLOR);
	
	// downsample
	
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	
	if(!use_hdr){
		
		glColor4f(1.f, 1.f, 1.f, .5f*.5f);
		
		glBegin(GL_QUADS);
		
		for(x=0;x<2;x++){
			for(y=0;y<2;y++){
				float sx, sy;
				sx=((float)x-.5f)*.5f;
				sy=((float)y-.5f)*.5f;
				glTexCoord2f(0, screen->h);
				glVertex2f(sx, screen->h-screen->h/bloom_downsample+sy);
				
				glTexCoord2f(0, 0);
				glVertex2f(sx, screen->h+sy);
				
				glTexCoord2f(screen->w, 0);
				glVertex2f(screen->w/bloom_downsample+sx, screen->h+sy);
				
				glTexCoord2f(screen->w, screen->h);
				glVertex2f(screen->w/bloom_downsample+sx, screen->h-screen->h/bloom_downsample+sy);
				totalPolys+=2;
			}
		}
		
		glEnd();
		
		glBindTexture(GL_TEXTURE_2D, tex_bloom);
		glPopMatrix();
		glPushMatrix();
		glScalef((float)bloom_downsample/(float)sw, (float)bloom_downsample/(float)sh, 1.f);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
							screen->w/bloom_downsample, screen->h/bloom_downsample);
	}else{
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
							screen->w/bloom_downsample, screen->h/bloom_downsample);
		int divs=1;
		int div2;
		int stepCount=1;
		float weights, weight;
		weights=1.f;
		do{
			divs<<=1;
			weight=logf(2.f)/logf((float)divs*2.f);
			weights+=weight;
			stepCount++;
		}while((float)(screen->w/bloom_downsample)/(float)divs>=1.f || (float)(screen->h/bloom_downsample)/(float)divs>=1.f);
		div2=divs; 
		int oww, ohh;;
		//printf("%d\n", divs);
		while(divs){
			int cww, chh;
			weight=logf(2.f)/logf((float)divs*2.f);
			cww=(screen->w/bloom_downsample+divs-1)/(divs);//ceilf((float)(screen->w/bloom_downsample)/(float)divs);
			chh=(screen->h/bloom_downsample+divs-1)/(divs);//ceilf((float)(screen->h/bloom_downsample)/(float)divs);
			if(divs!=div2){
				// upsample previous stage
				/*
				glBindTexture(GL_TEXTURE_2D, tex_bloom);
				glPopMatrix();
				glPushMatrix();
				glScalef((float)bloom_downsample/(float)sw, (float)bloom_downsample/(float)sh, 1.f);
				 */
				glBlendFunc(GL_ONE,GL_ZERO);
				//glDisable(GL_BLEND);
				glBegin(GL_QUADS);
				glTexCoord2f(0, ohh);
				glVertex2f(0, screen->h-chh);
				glTexCoord2f(0, 0);
				glVertex2f(0, screen->h);
				glTexCoord2f(oww, 0);
				glVertex2f(cww, screen->h);
				glTexCoord2f(oww, ohh);
				glVertex2f(cww, screen->h-chh);
				glEnd();
				glBlendFunc(GL_SRC_ALPHA,GL_ONE);
				totalPolys+=2;
				//glEnable(GL_BLEND);
			}	
			// downsample original
			
			glColor4f(1.f, 1.f, 1.f, weight/weights*.25f*.25f);
			glBindTexture(GL_TEXTURE_2D, tex_screen);
			glPopMatrix(); glPushMatrix();
			glScalef(1.f/(float)sw, 1.f/(float)sh, 1.f);
			glBegin(GL_QUADS);
			for(x=0;x<4;x++)
				for(y=0;y<4;y++){
					float sx, sy;
					sx=((float)x-1.5f)*.25f;
					sy=((float)y-1.5f)*.25f;
					glTexCoord2f(0, screen->h);
					glVertex2f(sx, screen->h-chh+sy);
					glTexCoord2f(0, 0);
					glVertex2f(sx, screen->h+sy);
					glTexCoord2f(screen->w,  0);
					glVertex2f(cww+sx, screen->h+sy);
					glTexCoord2f(screen->w, screen->h);
					glVertex2f(cww+sx, screen->h-chh+sy);
				}
			glEnd();
			totalPolys+=2;
			// make bloom texture
			glBindTexture(GL_TEXTURE_2D, tex_bloom);
			glPopMatrix(); glPushMatrix();
			glScalef((float)bloom_downsample/(float)sw, (float)bloom_downsample/(float)sh, 1.f);
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
								(int)cww, (int)chh);
			//printf("%d %d\n", (int)cww, (int)chh);
			
			oww=cww;
			ohh=chh;
			
			divs>>=1;
			
		}
	}
	
	// darken
	
	glBlendFunc(GL_DST_COLOR,GL_ZERO);
	
	for(n=0;n<bloom_darken;n++){
		glBegin(GL_QUADS);
		
		glTexCoord2i(0, screen->h/bloom_downsample);
		glVertex2i(0, screen->h-screen->h/bloom_downsample);
		
		glTexCoord2i(0, 0);
		glVertex2i(0, screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, 0);
		glVertex2i(screen->w/bloom_downsample, screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
		glVertex2i(screen->w/bloom_downsample, screen->h-screen->h/bloom_downsample);
		
		glEnd();
		totalPolys+=2;
	}
	
	//glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
	//				 screen->w/bloom_downsample, screen->h/bloom_downsample);
		
		
	
	
	// blur
	
	glBindTexture(GL_TEXTURE_2D, tex_bloom);
	glPopMatrix();
	glPushMatrix();
	glScalef((float)bloom_downsample/(float)sw, (float)bloom_downsample/(float)sh, 1.f);
	
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	
	for(int i=0;i<bloom_smooth;i++){
		glClear(GL_COLOR_BUFFER_BIT);
		if(bloom==BM_basic){

		}else if(bloom==BM_overlap){
			
			
			const float bloom_overlap_interval=1.25f;
		
			float d;
			
			glBegin(GL_QUADS);
			
			x=0; y=0; d=6.f/16.f;
			glColor4f(d, d, d, 1.f);
			
			glTexCoord2i(0, screen->h/bloom_downsample);
			glVertex2f((float)x*bloom_overlap_interval, 
					   (float)y*bloom_overlap_interval+screen->h-screen->h/bloom_downsample);
			
			glTexCoord2i(0, 0);
			glVertex2f((float)x*bloom_overlap_interval, 
					   (float)y*bloom_overlap_interval+screen->h);
			
			glTexCoord2i(screen->w/bloom_downsample, 0);
			glVertex2f((float)x*bloom_overlap_interval+screen->w/bloom_downsample, 
					   (float)y*bloom_overlap_interval+screen->h);
			
			glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
			glVertex2f((float)x*bloom_overlap_interval+screen->w/bloom_downsample, 
					   (float)y*bloom_overlap_interval+screen->h-screen->h/bloom_downsample);
			totalPolys+=2;
			
			x=-1; y=0; d=5.f/16.f;
			glColor4f(d, d, d, 1.f);
			
			glTexCoord2i(0, screen->h/bloom_downsample);
			glVertex2f((float)x*bloom_overlap_interval, 
					   (float)y*bloom_overlap_interval+screen->h-screen->h/bloom_downsample);
			
			glTexCoord2i(0, 0);
			glVertex2f((float)x*bloom_overlap_interval, 
					   (float)y*bloom_overlap_interval+screen->h);
			
			glTexCoord2i(screen->w/bloom_downsample, 0);
			glVertex2f((float)x*bloom_overlap_interval+screen->w/bloom_downsample, 
					   (float)y*bloom_overlap_interval+screen->h);
			
			glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
			glVertex2f((float)x*bloom_overlap_interval+screen->w/bloom_downsample, 
					   (float)y*bloom_overlap_interval+screen->h-screen->h/bloom_downsample);
			totalPolys+=2;
			
			x=1; y=0; d=5.f/16.f;
			glColor4f(d, d, d, 1.f);
			
			glTexCoord2i(0, screen->h/bloom_downsample);
			glVertex2f((float)x*bloom_overlap_interval, 
					   (float)y*bloom_overlap_interval+screen->h-screen->h/bloom_downsample);
			
			glTexCoord2i(0, 0);
			glVertex2f((float)x*bloom_overlap_interval, 
					   (float)y*bloom_overlap_interval+screen->h);
			
			glTexCoord2i(screen->w/bloom_downsample, 0);
			glVertex2f((float)x*bloom_overlap_interval+screen->w/bloom_downsample, 
					   (float)y*bloom_overlap_interval+screen->h);
			
			glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
			glVertex2f((float)x*bloom_overlap_interval+screen->w/bloom_downsample, 
					   (float)y*bloom_overlap_interval+screen->h-screen->h/bloom_downsample);
			totalPolys+=2;
				
			
			glEnd();
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
								screen->w/bloom_downsample, screen->h/bloom_downsample);
			
			glClear(GL_COLOR_BUFFER_BIT);
			
			glBegin(GL_QUADS);
			
			x=0; y=0; d=6.f/16.f;
			glColor4f(d, d, d, 1.f);
			
			glTexCoord2i(0, screen->h/bloom_downsample);
			glVertex2f((float)x*bloom_overlap_interval, 
					   (float)y*bloom_overlap_interval+screen->h-screen->h/bloom_downsample);
			
			glTexCoord2i(0, 0);
			glVertex2f((float)x*bloom_overlap_interval, 
					   (float)y*bloom_overlap_interval+screen->h);
			
			glTexCoord2i(screen->w/bloom_downsample, 0);
			glVertex2f((float)x*bloom_overlap_interval+screen->w/bloom_downsample, 
					   (float)y*bloom_overlap_interval+screen->h);
			
			glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
			glVertex2f((float)x*bloom_overlap_interval+screen->w/bloom_downsample, 
					   (float)y*bloom_overlap_interval+screen->h-screen->h/bloom_downsample);
			totalPolys+=2;
			
			x=0; y=-1; d=5.f/16.f;
			glColor4f(d, d, d, 1.f);
			
			glTexCoord2i(0, screen->h/bloom_downsample);
			glVertex2f((float)x*bloom_overlap_interval, 
					   (float)y*bloom_overlap_interval+screen->h-screen->h/bloom_downsample);
			
			glTexCoord2i(0, 0);
			glVertex2f((float)x*bloom_overlap_interval, 
					   (float)y*bloom_overlap_interval+screen->h);
			
			glTexCoord2i(screen->w/bloom_downsample, 0);
			glVertex2f((float)x*bloom_overlap_interval+screen->w/bloom_downsample, 
					   (float)y*bloom_overlap_interval+screen->h);
			
			glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
			glVertex2f((float)x*bloom_overlap_interval+screen->w/bloom_downsample, 
					   (float)y*bloom_overlap_interval+screen->h-screen->h/bloom_downsample);
			totalPolys+=2;
			
			x=0; y=1; d=5.f/16.f;
			glColor4f(d, d, d, 1.f);
			
			glTexCoord2i(0, screen->h/bloom_downsample);
			glVertex2f((float)x*bloom_overlap_interval, 
					   (float)y*bloom_overlap_interval+screen->h-screen->h/bloom_downsample);
			
			glTexCoord2i(0, 0);
			glVertex2f((float)x*bloom_overlap_interval, 
					   (float)y*bloom_overlap_interval+screen->h);
			
			glTexCoord2i(screen->w/bloom_downsample, 0);
			glVertex2f((float)x*bloom_overlap_interval+screen->w/bloom_downsample, 
					   (float)y*bloom_overlap_interval+screen->h);
			
			glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
			glVertex2f((float)x*bloom_overlap_interval+screen->w/bloom_downsample, 
					   (float)y*bloom_overlap_interval+screen->h-screen->h/bloom_downsample);
			totalPolys+=2;
			
			
			glEnd();
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
								screen->w/bloom_downsample, screen->h/bloom_downsample);
			
		}
#if GL_ARB_shader_objects
		else if(bloom==BM_glsl){
			glUseProgramObjectARB(prg_bloom1);
			glUniform1iARB(glGetUniformLocationARB(prg_bloom1, "tex"),
						0);
			
			glBegin(GL_QUADS);
			
			glColor4f(1.f, 1.f, 1.f, 1.f);
			
			glTexCoord2i(0, screen->h/bloom_downsample);
			glVertex2f(0, 
					   screen->h-screen->h/bloom_downsample);
			
			glTexCoord2i(0, 0);
			glVertex2f(0, 
					   screen->h);
			
			glTexCoord2i(screen->w/bloom_downsample, 0);
			glVertex2f(screen->w/bloom_downsample, 
					   screen->h);
			
			glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
			glVertex2f(screen->w/bloom_downsample, 
					   screen->h-screen->h/bloom_downsample);
			totalPolys+=2;
			glEnd();
			
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
								screen->w/bloom_downsample, screen->h/bloom_downsample);
			glClear(GL_COLOR_BUFFER_BIT);
			glUseProgramObjectARB(prg_bloom2);
			
			glUniform1iARB(glGetUniformLocationARB(prg_bloom2, "tex"),
						0);
			
			glBegin(GL_QUADS);
			
			glColor4f(1.f, 1.f, 1.f, 1.f);
			
			glTexCoord2i(0, screen->h/bloom_downsample);
			glVertex2f(0, 
					   screen->h-screen->h/bloom_downsample);
			
			glTexCoord2i(0, 0);
			glVertex2f(0, 
					   screen->h);
			
			glTexCoord2i(screen->w/bloom_downsample, 0);
			glVertex2f(screen->w/bloom_downsample, 
					   screen->h);
			
			glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
			glVertex2f(screen->w/bloom_downsample, 
					   screen->h-screen->h/bloom_downsample);
			totalPolys+=2;
			glEnd();
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
								screen->w/bloom_downsample, screen->h/bloom_downsample);
			
			glUseProgramObjectARB(0);
		}
#endif
	}
#if GL_ARB_shader_objects
	if(bloom==BM_glsl){
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgramObjectARB(prg_bloom3);
		glUniform1iARB(glGetUniformLocationARB(prg_bloom1, "tex"),
					   0);
		
		glBegin(GL_QUADS);
		
		glColor4f(1.f, 1.f, 1.f, 1.f);
		
		glTexCoord2i(0, screen->h/bloom_downsample);
		glVertex2f(0, 
				   screen->h-screen->h/bloom_downsample);
		
		glTexCoord2i(0, 0);
		glVertex2f(0, 
				   screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, 0);
		glVertex2f(screen->w/bloom_downsample, 
				   screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
		glVertex2f(screen->w/bloom_downsample, 
				   screen->h-screen->h/bloom_downsample);
		totalPolys+=2;
		glEnd();
		
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
							screen->w/bloom_downsample, screen->h/bloom_downsample);
				
		glUseProgramObjectARB(0);
	}
#endif

	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
	//apply bloom!
	
	if(bloom_style==BS_color){
		
		float x1, x2;
		float y1, y2;
		float scale;
		
	
		glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
		
		scale=.997f;
		x1=float(screen->w>>1)-screen->w*scale*.5f;
		x2=float(screen->w>>1)+screen->w*scale*.5f;
		y1=float(screen->h>>1)-screen->h*scale*.5f;
		y2=float(screen->h>>1)+screen->h*scale*.5f;
		
		glBegin(GL_QUADS);
		glColor4f(bloom_wet, bloom_wet, bloom_wet, 1.f);
		
		glTexCoord2i(0, screen->h/bloom_downsample);
		glVertex2i(x1, y1);
		
		glTexCoord2i(0, 0);
		glVertex2i(x1, y2);
		
		glTexCoord2i(screen->w/bloom_downsample, 0);
		glVertex2i(x2, y2);
		
		glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
		glVertex2i(x2, y1);
		totalPolys+=2;
		
		glEnd();
		
		glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
		
		scale=.999f;
		x1=float(screen->w>>1)-screen->w*scale*.5f;
		x2=float(screen->w>>1)+screen->w*scale*.5f;
		y1=float(screen->h>>1)-screen->h*scale*.5f;
		y2=float(screen->h>>1)+screen->h*scale*.5f;
		
		glBegin(GL_QUADS);
		glColor4f(bloom_wet, bloom_wet, bloom_wet, 1.f);
		
		glTexCoord2i(0, screen->h/bloom_downsample);
		glVertex2i(x1, y1);
		
		glTexCoord2i(0, 0);
		glVertex2i(x1, y2);
		
		glTexCoord2i(screen->w/bloom_downsample, 0);
		glVertex2i(x2, y2);
		
		glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
		glVertex2i(x2, y1);
		totalPolys+=2;
		
		glEnd();
		
		glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_TRUE);
		
		scale=1.035f;
		x1=float(screen->w>>1)-screen->w*scale*.5f;
		x2=float(screen->w>>1)+screen->w*scale*.5f;
		y1=float(screen->h>>1)-screen->h*scale*.5f;
		y2=float(screen->h>>1)+screen->h*scale*.5f;
		
		glBegin(GL_QUADS);
		glColor4f(bloom_wet, bloom_wet, bloom_wet, 1.f);
		
		glTexCoord2i(0, screen->h/bloom_downsample);
		glVertex2i(x1, y1);
		
		glTexCoord2i(0, 0);
		glVertex2i(x1, y2);
		
		glTexCoord2i(screen->w/bloom_downsample, 0);
		glVertex2i(x2, y2);
		
		glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
		glVertex2i(x2, y1);
		totalPolys+=2;
		
		glEnd();
		
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		
		glColor4f(bloom_wet*.06f, bloom_wet*.07f, bloom_wet*.07f, 1.f);
		scale=-1.5f;
		x1=float(screen->w>>1)-screen->w*scale*.5f;
		x2=float(screen->w>>1)+screen->w*scale*.5f;
		y1=float(screen->h>>1)-screen->h*scale*.5f;
		y2=float(screen->h>>1)+screen->h*scale*.5f;
		
		glBegin(GL_QUADS);
		
		glTexCoord2i(0, screen->h/bloom_downsample);
		glVertex2i(x1, y1);
		
		glTexCoord2i(0, 0);
		glVertex2i(x1, y2);
		
		glTexCoord2i(screen->w/bloom_downsample, 0);
		glVertex2i(x2, y2);
		
		glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
		glVertex2i(x2, y1);
		totalPolys+=2;
		
		glEnd();
		
		glColor4f(bloom_wet*.06f, bloom_wet*.07f, bloom_wet*.08f, 1.f);
		scale=-1.7f;
		x1=float(screen->w>>1)-screen->w*scale*.5f;
		x2=float(screen->w>>1)+screen->w*scale*.5f;
		y1=float(screen->h>>1)-screen->h*scale*.5f;
		y2=float(screen->h>>1)+screen->h*scale*.5f;
		
		glBegin(GL_QUADS);
		
		glTexCoord2i(0, screen->h/bloom_downsample);
		glVertex2i(x1, y1);
		
		glTexCoord2i(0, 0);
		glVertex2i(x1, y2);
		
		glTexCoord2i(screen->w/bloom_downsample, 0);
		glVertex2i(x2, y2);
		
		glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
		glVertex2i(x2, y1);
		totalPolys+=2;
		
		glEnd();
		
		
	}else if(bloom_style==BS_cross){
		n=bloom_downsample*4;
		glColor4f(bloom_wet/9.f, bloom_wet/9.f, bloom_wet/9.f, 1.f);
		glBegin(GL_QUADS);
		
		glTexCoord2i(0, screen->h/bloom_downsample);
		glVertex2i(0, 0);
		
		glTexCoord2i(0, 0);
		glVertex2i(0, screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, 0);
		glVertex2i(screen->w, screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
		glVertex2i(screen->w, 0);
		totalPolys+=2;
		
		glEnd();
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		glBegin(GL_QUADS);
		
		glTexCoord2i(0, screen->h/bloom_downsample);
		glVertex2i(n, 0);
		
		glTexCoord2i(0, 0);
		glVertex2i(n, screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, 0);
		glVertex2i(n+screen->w, screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
		glVertex2i(n+screen->w, 0);
		totalPolys+=2;
		
		
		glTexCoord2i(0, screen->h/bloom_downsample);
		glVertex2i(-n, 0);
		
		glTexCoord2i(0, 0);
		glVertex2i(-n, screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, 0);
		glVertex2i(-n+screen->w, screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
		glVertex2i(-n+screen->w, 0);
		totalPolys+=2;
		
		
		glTexCoord2i(0, screen->h/bloom_downsample);
		glVertex2i(0, n);
		
		glTexCoord2i(0, 0);
		glVertex2i(0, n+screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, 0);
		glVertex2i(screen->w, n+screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
		glVertex2i(screen->w, n);
		
		totalPolys+=2;
		
		
		glTexCoord2i(0, screen->h/bloom_downsample);
		glVertex2i(0, -n);
		
		glTexCoord2i(0, 0);
		glVertex2i(0, -n+screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, 0);
		glVertex2i(screen->w, -n+screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
		glVertex2i(screen->w, -n);
		totalPolys+=2;
		
		n*=2;
		glTexCoord2i(0, screen->h/bloom_downsample);
		glVertex2i(n, 0);
		
		glTexCoord2i(0, 0);
		glVertex2i(n, screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, 0);
		glVertex2i(n+screen->w, screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
		glVertex2i(n+screen->w, 0);
		totalPolys+=2;
		
		
		glTexCoord2i(0, screen->h/bloom_downsample);
		glVertex2i(-n, 0);
		
		glTexCoord2i(0, 0);
		glVertex2i(-n, screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, 0);
		glVertex2i(-n+screen->w, screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
		glVertex2i(-n+screen->w, 0);
		totalPolys+=2;
		
		
		glTexCoord2i(0, screen->h/bloom_downsample);
		glVertex2i(0, n);
		
		glTexCoord2i(0, 0);
		glVertex2i(0, n+screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, 0);
		glVertex2i(screen->w, n+screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
		glVertex2i(screen->w, n);
		
		totalPolys+=2;
		
		
		glTexCoord2i(0, screen->h/bloom_downsample);
		glVertex2i(0, -n);
		
		glTexCoord2i(0, 0);
		glVertex2i(0, -n+screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, 0);
		glVertex2i(screen->w, -n+screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
		glVertex2i(screen->w, -n);
		totalPolys+=2;
		
		
		glEnd();
	}else{
	
		glBegin(GL_QUADS);
		glColor4f(bloom_wet, bloom_wet, bloom_wet, 1.f);
		
		glTexCoord2i(0, screen->h/bloom_downsample);
		glVertex2i(0, 0);
		
		glTexCoord2i(0, 0);
		glVertex2i(0, screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, 0);
		glVertex2i(screen->w, screen->h);
		
		glTexCoord2i(screen->w/bloom_downsample, screen->h/bloom_downsample);
		glVertex2i(screen->w, 0);
		
		totalPolys+=2;
		
		glEnd();
		
	}
	
	// restore screen
	
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, tex_screen);
	glPopMatrix();
	glPushMatrix();
	glScalef(1.f/(float)sw, 1.f/(float)sh, 1.f);
	
	glBegin(GL_QUADS);
	glColor4f(1.f, 1.f, 1.f, bloom_dry);
	
	
	glTexCoord2i(0, screen->h);
	glVertex2i(0, 0);
	
	glTexCoord2i(0, 0);
	glVertex2i(0, screen->h);
	
	glTexCoord2i(screen->w, 0);
	glVertex2i(screen->w, screen->h);
	
	glTexCoord2i(screen->w, screen->h);
	glVertex2i(screen->w, 0);
	
	totalPolys+=2;
	
	glEnd();
	
	// near object is more sharp
	
	if(multiSamples!=1 && bloom_quality==BQ_high){
		consoleLog("B_apply: High quality bloom can't be used with multisample, disabling");
		bloom_quality=BQ_low;
	}
	
	if(bloom_quality==BQ_high){
		
	#if GL_ARB_shader_objects
		if(use_glsl && use_hdr){
			
			glBindTexture(GL_TEXTURE_2D, tex_depth);
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, screen->w, screen->h);
			glBindTexture(GL_TEXTURE_2D, tex_screen);
			
			glActiveTexture(GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_2D, tex_depth);
			glActiveTexture(GL_TEXTURE0_ARB);
			
			glUseProgramObjectARB(prg_bloom4);
			glUniform1iARB(glGetUniformLocationARB(prg_bloom4, "texDry"),
						   0);
			glUniform1iARB(glGetUniformLocationARB(prg_bloom4, "texDepth"),
						   1);
			
			
			glBegin(GL_QUADS);
			glColor4f(1.f, 1.f, 1.f, bloom_dry);
			
			
			glTexCoord2i(0, screen->h);
			glVertex2i(0, 0);
			
			glTexCoord2i(0, 0);
			glVertex2i(0, screen->h);
			
			glTexCoord2i(screen->w, 0);
			glVertex2i(screen->w, screen->h);
			
			glTexCoord2i(screen->w, screen->h);
			glVertex2i(screen->w, 0);
			
			totalPolys+=2;
			
			glEnd();
			
			glUseProgramObjectARB(0);
			
		}else
	#endif
		{
			glDepthFunc(GL_GREATER);
			glEnable(GL_DEPTH_TEST);
			
			float zz;
			
			zz=1.f-1.f/(mp->fog*.5f);
			
			glBegin(GL_QUADS);
			glColor4f(1.f, 1.f, 1.f, bloom_nearDry);
			
			
			glTexCoord2i(0, screen->h);
			glVertex3f(0, 0, zz);
			
			glTexCoord2i(0, 0);
			glVertex3f(0, screen->h, zz);
			
			glTexCoord2i(screen->w, 0);
			glVertex3f(screen->w, screen->h, zz);
			
			glTexCoord2i(screen->w, screen->h);
			glVertex3f(screen->w, 0, zz);

			totalPolys+=2;
			
			glEnd();
			
			float lv, olv;
			olv=1.f;
			
			for(int j=8;j>=0;j-=2){
				
				lv=(float)j/10.f;
				
				zz=1.f-1.f/(mp->fog*((float)j/10.f));
				
				glBegin(GL_QUADS);
				glColor4f(1.f, 1.f, 1.f, 1.f-lv/olv);
				
				
				glTexCoord2i(0, screen->h);
				glVertex3f(0, 0, zz);
				
				glTexCoord2i(0, 0);
				glVertex3f(0, screen->h, zz);
				
				glTexCoord2i(screen->w, 0);
				glVertex3f(screen->w, screen->h, zz);
				
				glTexCoord2i(screen->w, screen->h);
				glVertex3f(screen->w, 0, zz);
				
				totalPolys+=2;
				
				glEnd();
				
				olv=lv;
				
			}
			
			glDisable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
				
		}
		
	}
		
	// end - near object
	
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
	//glDisable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	
	
	glPopMatrix();
	
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_FOG);
	glDepthMask(GL_TRUE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
}

#else

void B_init(){
	// bloom disabled
}

void B_apply(){
	// bloom disabled
}

#endif
