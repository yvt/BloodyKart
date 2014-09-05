/*
 *  clienthud.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/11/27.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "clienthud.h"
#include "client.h"
#include "map.h"
#include "font.h"

struct clienthud_t{
	float fade;
	vec3_t center;
};

static clienthud_t hud[MAX_CLIENTS];

void CH_init(){
	int n;
	for(n=0;n<MAX_CLIENTS;n++){
		hud[n].fade=0.f;
	}
	consoleLog("CH_init: all clienthud reseted\n");
}

void CH_cframenext(float dt){
	int n;
	for(n=0;n<MAX_CLIENTS;n++){
		client_t& c=cli[n];
		clienthud_t& h=hud[n];
		if(!c.enable){
			h.fade=0.f;
			continue;
		}
		bool visible=false;
		vec3_t cent=c.pos+vec3_t(0.f, .5f, 0.f);
		
		// raycast from camera to client
		vector<isect_t> *lst=mp->m->raycast(camera_from, cent);
		visible=!lst->size();
		delete lst;
		
		if(visible){
			h.fade+=dt*4.f;
			if(h.fade>1.f)
				h.fade=1.f;
		}else{
			h.fade-=dt*4.f;
			if(h.fade<0.f)
				h.fade=0.f;
		}
		h.center=cent;
	}
}

void CH_render(){
	
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	int n;
	for(n=0;n<MAX_CLIENTS;n++){
		client_t& c=cli[n];
		clienthud_t& h=hud[n];
		if(h.fade<.01f)
			continue;
		double xx, yy, zz;
		vec3_t v=h.center;
		gluProject(v.x, v.y, v.z, osd_old_modelview, osd_old_proj, viewport, &xx, &yy, &zz);
		if(zz>1.)
			continue;
		yy=(double)screen->h-yy;
		
		glColor4f(0.f, 0.f, 0.f, .3f*h.fade);
		font_roman36->drawhalf(c.name, xx-font_roman36->width(c.name)*.25f+.8f, yy-20.f+.8f);
		glColor4f(1.f, 1.f, 1.f, .5f*h.fade);
		font_roman36->drawhalf(c.name, xx-font_roman36->width(c.name)*.25f, yy-20.f);
	}
}