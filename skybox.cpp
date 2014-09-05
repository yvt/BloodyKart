/*
 *  skybox.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/24.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "skybox.h"
#include "glpng.h"

GLuint tex_skybox;
static GLuint list_skybox, list_skybox_glsl;

#if GL_ARB_shader_objects
GLhandleARB prg_skybox;
GLhandleARB prg_skysim;
#endif

static int sbPolys=0;

void SB_load(const char *fn){
	glBindTexture(GL_TEXTURE_2D, tex_skybox);
	glpngLoadTexture(fn, false);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}
void SB_init(){
	glGenTextures(1, &tex_skybox);
	
#if GL_ARB_shader_objects
	
	if(cap_glsl){
		
		// load skybox shader
		
		prg_skybox=create_program("res/shaders/skybox.vs", "res/shaders/skybox.fs");
		if(prg_skybox)
			consoleLog("SB_init: compiled program \"skybox\"\n");
		else
			consoleLog("SB_init: couldn't compile program \"skybox\"\n");
		
		prg_skysim=create_program("res/shaders/skysim.vs", "res/shaders/skysim.fs");
		if(prg_skysim)
			consoleLog("SB_init: compiled program \"skysim\"\n");
		else
			consoleLog("SB_init: couldn't compile program \"skysim\"\n");

	}else{
		
#endif
		
		consoleLog("SB_init: no programs to compile\n");
		
#if GL_ARB_shader_objects
	}
#endif

	
	const int lng=64;
	const int lat=32;
	const float radius=1000.f;
	int i, j;
	
#if GL_ARB_shader_objects
	if(cap_glsl){
		consoleLog("SB_init: creating skybox with GLSL\n");

		list_skybox_glsl=glGenLists(1);
		glNewList(list_skybox_glsl, GL_COMPILE);

		
		glDisable(GL_BLEND);
		for(i=0;i<lat;i++){
			float y1, y2;
			float r1, r2;
			float v1, v2;
			float ang;
			y1=sinf((float)i/(float)lat*M_PI-M_PI*.5f)*radius;
			r1=cosf((float)i/(float)lat*M_PI-M_PI*.5f)*radius;
			y2=sinf((float)(i+1)/(float)lat*M_PI-M_PI*.5f)*radius;
			r2=cosf((float)(i+1)/(float)lat*M_PI-M_PI*.5f)*radius;
			v1=1.f-(float)i/(float)lat;
			v2=1.f-(float)(i+1)/(float)lat;
			if(v1>.5f)
				v1=1.f-v1;
			if(v2>.5f)
				v2=1.f-v2;
			glBegin(GL_TRIANGLE_STRIP);
			glColor4f(1.f, 1.f, 1.f, 1.f);
			
			for(j=0;j<=lng;j++){
				
				ang=(float)j/(float)lng*M_PI*2.f;
				glTexCoord2f((float)j/(float)lng, v1);
				glNormal3f(-sinf(ang)*r1/radius, -y1/radius, -cosf(ang)*r1/radius);
				glVertex3f(sinf(ang)*r1, y1, cosf(ang)*r1);
				
				glTexCoord2f((float)(j)/(float)lng, v2);
				glNormal3f(-sinf(ang)*r1/radius, -y1/radius, -cosf(ang)*r1/radius);
				glVertex3f(sinf(ang)*r2, y2, cosf(ang)*r2);
				
				
			}
			//glVertex3f(sinf(ang)*r2, y2, cosf(ang)*r2);
			glEnd();
		
		}
		glEnable(GL_BLEND);
		
		glUseProgramObjectARB(NULL);
		glEndList();
	}
	
#endif
	// without glsl
	
	consoleLog("SB_init: creating skybox without GLSL\n");
	
	list_skybox=glGenLists(1);
	glNewList(list_skybox, GL_COMPILE);
	
	glDisable(GL_BLEND);
	
	for(i=0;i<lat;i++){
		float y1, y2;
		float r1, r2;
		float v1, v2;
		float ang;
		y1=sinf((float)i/(float)lat*M_PI-M_PI*.5f)*radius;
		r1=cosf((float)i/(float)lat*M_PI-M_PI*.5f)*radius;
		y2=sinf((float)(i+1)/(float)lat*M_PI-M_PI*.5f)*radius;
		r2=cosf((float)(i+1)/(float)lat*M_PI-M_PI*.5f)*radius;
		v1=1.f-(float)i/(float)lat;
		v2=1.f-(float)(i+1)/(float)lat;
		if(v1>.5f)
			v1=1.f-v1;
		if(v2>.5f)
			v2=1.f-v2;
		glBegin(GL_TRIANGLE_STRIP);
		glColor4f(1.f, 1.f, 1.f, 1.f);
		
		for(j=0;j<=lng;j++){
			
			ang=(float)j/(float)lng*M_PI*2.f;
			glTexCoord2f((float)j/(float)lng, v1);
			glNormal3f(-sinf(ang)*r1/radius, -y1/radius, -cosf(ang)*r1/radius);
			glVertex3f(sinf(ang)*r1, y1, cosf(ang)*r1);
			
			glTexCoord2f((float)(j)/(float)lng, v2);
			glNormal3f(-sinf(ang)*r1/radius, -y1/radius, -cosf(ang)*r1/radius);
			glVertex3f(sinf(ang)*r2, y2, cosf(ang)*r2);
			
			
			
		}
		//glVertex3f(sinf(ang)*r2, y2, cosf(ang)*r2);
		glEnd();
		
		sbPolys+=lng*2;
		
	}
	
	glEnable(GL_BLEND);
	
	for(i=0;i<lat;i++){
		float y1, y2;
		float r1, r2;
		float v1, v2;
		float ang;
		y1=sinf((float)i/(float)lat*M_PI-M_PI*.5f)*radius;
		r1=cosf((float)i/(float)lat*M_PI-M_PI*.5f)*radius;
		y2=sinf((float)(i+1)/(float)lat*M_PI-M_PI*.5f)*radius;
		r2=cosf((float)(i+1)/(float)lat*M_PI-M_PI*.5f)*radius;
		v1=1.f-(float)i/(float)lat;
		v2=1.f-(float)(i+1)/(float)lat;
		if(v1>.5f)
			v1=1.f-v1;
		if(v2>.5f)
			v2=1.f-v2;
		glBegin(GL_TRIANGLE_STRIP);
		glColor4f(1.f, 1.f, 1.f, 1.f);
		
		for(j=0;j<=lng;j++){
			
			ang=(float)j/(float)lng*M_PI*2.f;
			glTexCoord2f((float)j/(float)lng, v1);
			glNormal3f(-sinf(ang)*r1/radius, -y1/radius, -cosf(ang)*r1/radius);
			glVertex3f(sinf(ang)*r1, y1, cosf(ang)*r1);
			
			glTexCoord2f((float)(j)/(float)lng, v2);
			glNormal3f(-sinf(ang)*r1/radius, -y1/radius, -cosf(ang)*r1/radius);
			glVertex3f(sinf(ang)*r2, y2, cosf(ang)*r2);
			
			
		}
		//glVertex3f(sinf(ang)*r2, y2, cosf(ang)*r2);
		glEnd();
		
	}

	glEndList();
}

void SB_render(){
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color4f(0.f, 0.f, 0.f, 1.f));
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color4f(0.f, 0.f, 0.f, 1.f));
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color4f(1.f, 1.f, 1.f, 1.f));
	glBlendFunc(GL_SRC_ALPHA,GL_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, tex_skybox);
#if GL_ARB_shader_objects
	if(use_glsl){
		glUseProgramObjectARB(tex_skybox?prg_skybox:prg_skysim);
		glCallList(list_skybox_glsl);
		glUseProgramObjectARB(0);
		totalPolys+=sbPolys;
	}else
#endif
	{	glCallList(list_skybox);
		totalPolys+=sbPolys*2;
	}

	//glCallList(list_skybox);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
}



