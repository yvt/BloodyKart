/*
 *  effect.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/09/29.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "glpng.h"
#include "effect.h"
#include "map.h"
#include "mesh.h"
#include "client.h"
#include "weapon.h"
#include "server.h"

#define MAX_EFFECTS		4096
#define MAX_MARKS		4096

enum effect_type_t{
	ET_tire_smoke,
	ET_blood1, // fly
	ET_blood2, //fly 
	ET_blood3, //fly 
	ET_blood4, //fly
	ET_blood5, //splatter
	ET_blood6, // smoke
	ET_smoke1, // smoke
	ET_smoke2, // smoke
	ET_smoke3, // smoke
	ET_smoke4, // smoke
	ET_burst1, // smoke
	ET_burst2, // smoke
	ET_burst3, // smoke
	ET_burst4, // smoke
	ET_burn1, // smoke
	ET_burn2, // smoke
	ET_burn3, // smoke
	ET_burn4, // smoke
	ET_explode, // emitter
	ET_smoke1f, // fly
	ET_smoke2f, // fly
	ET_smoke3f, // fly
	ET_smoke4f, // fly
	ET_item,	// based on map
	ET_muzzle_flash,
	
	ET_count
};

enum effect_material_t{
	EM_none,
	EM_splatter,
	EM_splatter_bumped,
	EM_smoke,
	EM_smoke_addive,
	EM_burn,
	EM_mesh,
	EM_mesh_reflect,
	EM_muzzle_flash
};

static vec3_t camera_dir;

struct effect_t{
	bool used;
	effect_type_t type;
	vec3_t pos, vel;
	vec3_t ang;
	float frm;
	float r1, r2, r3;
	vec3_t v1, v2, v3;
	bool critical;
	int i1, i2, i3;
	
	float depth;
	
	void calc_depth(){
		depth=vec3_t::dot(pos-camera_from, camera_dir);
	}
	bool operator <(const effect_t& e) const{
		if((!(used)) && (e.used))
			return true;
		if((used) && (!(e.used)))
			return false;
		if((!(used)) && (!(e.used)))
			return false;
		return depth<e.depth;
	}
};

static effect_t eff[MAX_EFFECTS];

struct mark_t{
	bool used;
	GLuint tex;
	vec3_t pos;
	Uint32 otick;
	int polys;
};

static mark_t mark[MAX_MARKS];
static GLuint mlist;

static GLuint tex_splatter; // g_blood=true
static GLuint tex_safesplt; // g_blood=false
static GLuint tex_bulletMark;
static GLuint tex_smoke;
static GLuint tex_item;
static GLuint tex_reflect;
static GLuint tex_muzzle;
static GLuint tex_bloodStain;
// static GLuint tex_refract; // will be needed in the future

static int rw, rh; // tex_reflect size

static mesh *m_item;

#if GL_ARB_shader_objects
static GLhandleARB prg_splatter;
static GLhandleARB prg_splatter_bumped;
static GLhandleARB prg_smoke;
static GLhandleARB prg_burn;
static GLhandleARB prg_muzzle;
static int attr_time;
static int attr_fireSrc;
#endif

static int allocEffect(effect_type_t type){
	int n;
	float mins; int mini=-1;
	for(n=0;n<MAX_EFFECTS;n++){
		if(!eff[n].used){
			eff[n].type=type;
			eff[n].frm=0.f;
			eff[n].used=true;
			eff[n].r1=rnd(); eff[n].r2=rnd(); eff[n].r3=rnd();
			return n;
		}else if(!eff[n].critical){
			if(eff[n].frm>mins || mini==-1){
				mini=n;
				mins=eff[n].frm;
			}
		}
	}
	n=mini;
	eff[n].used=true;
	eff[n].type=type;
	eff[n].frm=0.f;
	eff[n].critical=false;
	eff[n].r1=rnd(); eff[n].r2=rnd(); eff[n].r3=rnd();
	return n;
}

static void createReflectionTexture(){
	int sw, sh;
	
	// reflection will be warped, so its texture doesn't need to be big
	
	sw=larger_pow2(screen->w)>>1;
	sh=larger_pow2(screen->h)>>1;
	
	GLint mx;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mx);
	
	consoleLog("createReflectionTexture: required texture size is %dx%d\n", sw, sh);
	
	if(sw>mx || sh>mx){
		consoleLog("createReflectionTexture: too big texture, shrink to %d\n", mx);
		sw=mx; sh=mx;
	}
	
	glGenTextures(1, &tex_reflect);
	glBindTexture(GL_TEXTURE_2D, tex_reflect);
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	clearTexture(sw, sh);
	
	rw=sw;
	rh=sh;
}



void E_init(){
	
	consoleLog("E_init: loading res/textures/splatter.png\n");
	
	glGenTextures(1, &tex_splatter);
	glBindTexture(GL_TEXTURE_2D, tex_splatter);
	glpngLoadTexture("res/textures/splatter.png", false);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	consoleLog("E_init: loading res/textures/safesplt.png\n");
	
	glGenTextures(1, &tex_safesplt);
	glBindTexture(GL_TEXTURE_2D, tex_safesplt);
	glpngLoadTexture("res/textures/safesplt.png", false);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	consoleLog("E_init: loading res/textures/bulletMark.png\n");
	
	glGenTextures(1, &tex_bulletMark);
	glBindTexture(GL_TEXTURE_2D, tex_bulletMark);
	glpngLoadTexture("res/textures/bulletMark.png", true);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	consoleLog("E_init: loading res/sprites/smoke.png\n");
	
	glGenTextures(1, &tex_smoke);
	glBindTexture(GL_TEXTURE_2D, tex_smoke);
	glpngLoadTexture("res/sprites/smoke.png", false);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	consoleLog("E_init: loading res/sprites/item.png\n");
	
	glGenTextures(1, &tex_item);
	glBindTexture(GL_TEXTURE_2D, tex_item);
	glpngLoadTexture("res/sprites/item.png", false);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	consoleLog("E_init: loading res/sprites/muzzle.png\n");
	
	glGenTextures(1, &tex_muzzle);
	glBindTexture(GL_TEXTURE_2D, tex_muzzle);
	glpngLoadTexture("res/sprites/muzzle.png", true);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#if GL_TEXTURE_MAX_ANISOTROPY_EXT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
#endif
	
	consoleLog("E_init: loading res/sprites/bloodstain.png\n");
	
	glGenTextures(1, &tex_bloodStain);
	glBindTexture(GL_TEXTURE_2D, tex_bloodStain);
	glpngLoadTexture("res/sprites/bloodstain2.png", true);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#if GL_TEXTURE_MAX_ANISOTROPY_EXT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
#endif
	
	createReflectionTexture();
	
	consoleLog("E_init: loading res/models/item.x\n");
	m_item=new mesh("res/models/item.x");
	m_item->allowGLSL=false;
	m_item->mat[0].texed=true;
	m_item->mat[0].tex=tex_reflect;
	
	mlist=glGenLists(MAX_MARKS);
	
#if GL_ARB_shader_objects
	
	if(cap_glsl){
		
		// load bump shader
		
		prg_splatter=create_program("res/shaders/spltflat.vs", "res/shaders/spltflat.fs");
		if(prg_splatter)
			consoleLog("E_init: compiled program \"spltflat\"\n");
		else
			consoleLog("E_init: couldn't compile program \"spltflat\"\n");
		
		prg_splatter_bumped=create_program("res/shaders/splt.vs", "res/shaders/splt.fs");
		if(prg_splatter_bumped)
			consoleLog("E_init: compiled program \"splt\"\n");
		else
			consoleLog("E_init: couldn't compile program \"splt\"\n");
		
		prg_smoke=create_program("res/shaders/smoke.vs", "res/shaders/smoke.fs");
		if(prg_smoke)
			consoleLog("E_init: compiled program \"smoke\"\n");
		else
			consoleLog("E_init: couldn't compile program \"smoke\"\n");
		
		prg_burn=create_program("res/shaders/burn.vs", "res/shaders/burn.fs");
		if(prg_burn)
			consoleLog("E_init: compiled program \"burn\"\n");
		else
			consoleLog("E_init: couldn't compile program \"burn\"\n");
		
		prg_muzzle=create_program("res/shaders/muzzle.vs", "res/shaders/muzzle.fs");
		if(prg_muzzle)
			consoleLog("E_init: compiled program \"muzzle\"\n");
		else
			consoleLog("E_init: couldn't compile program \"muzzle\"\n");
		
		attr_time=glGetAttribLocationARB(prg_splatter_bumped, "time2");
		attr_fireSrc=glGetAttribLocationARB(prg_burn, "time2");
		
	}else{
#endif
	
		consoleLog("E_init: no programs to compile\n");
		
#if GL_ARB_shader_objects
	}	
#endif
	
	E_clear();
}
void E_clear(){
	int n;
	for(n=0;n<MAX_EFFECTS;n++){
		eff[n].used=false;
	}
	for(n=0;n<MAX_MARKS;n++){
		mark[n].used=false;
	}
}
void E_cframenext(float dt){
	int n;
	//dt*=.1f;
	for(n=0;n<MAX_EFFECTS;n++){
		effect_t& e=eff[n];
		if(!e.used)
			continue;
		vec3_t opos=e.pos;
		e.pos+=e.vel*dt;
		switch(e.type){
			case ET_tire_smoke:
				e.frm+=dt;
				if(e.frm>.6f)
					e.used=false;
				break;
			case ET_blood1:
				e.frm+=dt;
				e.vel.y-=7.f*dt;
				if(e.frm>1.5f*e.r3)
					e.used=false;
				break;
			case ET_blood2:
				e.frm+=dt;
				e.vel.y-=18.f*dt;
				/*if(e.frm>.7f)
					e.used=false;*/
				break;
			case ET_blood3:
				e.frm+=dt;
				e.vel.y-=6.3f*dt;
				if(e.frm>1.f*e.r3)
					e.used=false;
				break;
			case ET_blood4:
				e.frm+=dt;
				e.vel.y-=4.2f*dt;
				if(e.frm>2.f*e.r3)
					e.used=false;
				break;
			case ET_blood5:
				e.frm+=dt;
				e.vel.y-=3.f*dt;
				if(e.frm>.5f)
					e.used=false;
				break;
			case ET_blood6:
				e.frm+=dt;
				//e.vel.y=-.3f;
				if(e.frm>1.f)
					e.used=false;
				break;
			case ET_smoke1:
			case ET_smoke2:
			case ET_smoke3:
			case ET_smoke4:
				e.frm+=dt+e.vel.length()*2.*dt*e.r1;
				e.vel*=powf(.1f, dt);
				
				if(e.frm>4.f)
					e.used=false;
				break;
			case ET_smoke1f:
			case ET_smoke2f:
			case ET_smoke3f:
			case ET_smoke4f:
				e.frm+=dt;
				e.vel*=powf(.1f, dt);
				
				if(e.frm>2.f)
					e.used=false;
				break;
			case ET_burst1:
			case ET_burst2:
			case ET_burst3:
			case ET_burst4:
				e.frm+=dt;
				if(e.frm>.2f)
					e.used=false;
				break;
			case ET_burn1:
			case ET_burn2:
			case ET_burn3:
			case ET_burn4:
				e.frm+=dt;
				e.vel*=powf(.1f, dt);
				if(e.frm>1.5f)
					e.used=false;
				break;
			case ET_explode:
				e.frm+=dt;
				if(e.frm>.8f)
					e.used=false;
				e.r1-=dt*16.f;
				while(e.r1<0.f){
					if(e.frm<.3f){
						for(int i=0;i<3;i++){
							effect_t& ee=eff[allocEffect((effect_type_t)(ET_burst1+(rand()%4)))];
							ee.pos=e.pos; ee.vel=e.vel+vec3_t(rnd()-rnd(), rnd()-rnd(), rnd()-rnd())*.3f;
							//ee.pos+=ee.vel*3.f;
							ee.ang.z=rnd()*M_PI*2.f;
						}
					}
					for(int i=0;i<2;i++){
						effect_t& ee=eff[allocEffect((effect_type_t)(ET_smoke1+(rand()%4)))];
						ee.pos=e.pos; ee.vel=e.vel+vec3_t(rnd()-rnd(), rnd(), rnd()-rnd())*4.35f;
						ee.ang.z=rnd()*M_PI*2.f;
					}
					{
						effect_t& ee=eff[allocEffect((effect_type_t)(ET_burn1+(rand()%4)))];
						ee.pos=e.pos; ee.vel=e.vel+vec3_t(rnd()-rnd(), -rnd(), rnd()-rnd())*vec3_t(6.f, 2.5f, 6.f);
						ee.ang.z=rnd()*M_PI*2.f;
						ee.v1=e.v1;
					}
					e.r1+=rnd();
				}
				break;
			case ET_item:
				e.frm+=dt;
				break;
			case ET_muzzle_flash:
				e.used=false;
				break;
		}
		if(e.type==ET_blood2){
			vec3_t v1, v2, v3(0, 0, 0);
			vec3_t vv1, vv2;
			vector<isect_t> *res;
			v1=opos; v2=e.pos;
			if(e.type<=ET_blood4){
				//v3=rotate(vec3_t(0.f, 0.f, .4f*e.r2), e.ang);
				v3.y+=1.f;
				v1+=rotate(vec3_t(0.f, 0.f, (e.frm-dt)*(1.f+powf(e.r1, 16.f)*.4f+e.r2)), e.ang);
				v2+=rotate(vec3_t(0.f, 0.f, (e.frm)*(1.f+powf(e.r1, 16.f)*.4f+e.r2)), e.ang);
			}else{
				v3.x=(e.r2-.5f)*.8f;
				v3.y=0.f;
				v3.z=(e.r3-.5f)*.8f;
			}
			vv1=v1+v3; vv2=v2+v3;
			res=mp->m->raycast(vv1, vv2);
			sort(res->begin(), res->end());
			if(res->size()){
				int f=(*res)[0].face;
				v1=mp->m->vertex[mp->m->face[(f<<2)  ]];
				v2=mp->m->vertex[mp->m->face[(f<<2)+1]];
				v3=mp->m->vertex[mp->m->face[(f<<2)+2]];
				vec3_t proj, pos;
				proj=-(plane_t(v1, v2, v3).toward(v1).n);
				proj*=.3f;
				pos=vv1+(vv2-vv1).normalize()*(*res)[0].dist;
				if(g_blood)
					E_mark(pos, proj, /*vec3_t(.4f, .01f, .03f)*/vec3_t(1.f, 1.f, 1.f), 1.f,
						   (.8f+rnd()*.6f)*e.r3, rnd()*M_PI*2.f, tex_bloodStain,
						   0.f, 0.f, 1.f, 1.f);
				else
					E_mark(pos, proj, vec3_t(.0f, .0f, .0f), .5f,
						   (.8f+rnd()*.6f)*e.r3, rnd()*M_PI*2.f, tex_safesplt,
						   .0f, .5f, .25f, .5f);
				if(rand()&1)
					S_sound("fluid1", pos, .8f);
				else
					S_sound("fluid2", pos, .8f);
				e.used=false;
			}
			delete res;
		}
	}
}

void E_beginMaterial(effect_material_t mat){
	if(mat==EM_splatter){
		glBindTexture(GL_TEXTURE_2D, tex_splatter);
		if(!g_blood)
			glBindTexture(GL_TEXTURE_2D, tex_safesplt);
#if GL_ARB_shader_objects && GL_ARB_multitexture
		if(use_glsl){
			glUseProgramObjectARB(prg_splatter);
			glUniform1iARB(glGetUniformLocationARB(prg_splatter, "tex"),
						   0);
		}
#endif
		glBegin(GL_QUADS);
	}
	if(mat==EM_splatter_bumped){
		glBindTexture(GL_TEXTURE_2D, tex_splatter);
		if(!g_blood)
			glBindTexture(GL_TEXTURE_2D, tex_safesplt);
#if GL_ARB_shader_objects && GL_ARB_multitexture
		if(use_glsl){
			glUseProgramObjectARB(prg_splatter_bumped);
			glUniform1iARB(glGetUniformLocationARB(prg_splatter_bumped, "tex"),
						   0);
		}
#endif
		//glDisable(GL_TEXTURE_2D);
		//glUseProgramObjectARB(NULL);
		glBegin(GL_QUADS);
	}
	if(mat==EM_smoke){
		glBindTexture(GL_TEXTURE_2D, tex_smoke);
#if GL_ARB_shader_objects && GL_ARB_multitexture
		if(use_glsl){
			glUseProgramObjectARB(prg_smoke);
			glUniform1iARB(glGetUniformLocationARB(prg_smoke, "tex"),
						   0);
		}
#endif
		glBegin(GL_QUADS);
	}
	if(mat==EM_smoke_addive){
		glBindTexture(GL_TEXTURE_2D, tex_smoke);
		glDisable(GL_LIGHTING);
		glDisable(GL_FOG);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		glBegin(GL_QUADS);
	}
	if(mat==EM_burn){
		glBindTexture(GL_TEXTURE_2D, tex_smoke);
#if GL_ARB_shader_objects && GL_ARB_multitexture
		if(use_glsl){
			glUseProgramObjectARB(prg_burn);
			glUniform1iARB(glGetUniformLocationARB(prg_burn, "tex"),
						   0);
			glUniform3fARB(glGetUniformLocationARB(prg_burn, "fireColor"),
						   .9f, .4f, .1f);
		}
#endif
		glBegin(GL_QUADS);
	}
	if(mat==EM_mesh){
		
	}
	if(mat==EM_mesh_reflect){
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		//glBlendFunc(GL_ONE, GL_ZERO);
		glBindTexture(GL_TEXTURE_2D, tex_reflect);
		glDisable(GL_LIGHTING);
	}
	if(mat==EM_muzzle_flash){
		glBindTexture(GL_TEXTURE_2D, tex_muzzle);
		glDisable(GL_LIGHTING);
		glDisable(GL_FOG);
		//glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_COLOR);
#if GL_ARB_shader_objects && GL_ARB_multitexture
		if(use_glsl && use_hdr){
			glUseProgramObjectARB(prg_muzzle);
			glUniform1iARB(glGetUniformLocationARB(prg_muzzle, "tex"),
						   0);
		}
#endif
		glBegin(GL_QUADS);
	}
}

void E_endMaterial(effect_material_t mat){
	if(mat==EM_splatter){
		glEnd();
	}
	if(mat==EM_splatter_bumped){
		glEnd();
	}
	if(mat==EM_smoke){
		glEnd();
	}
	if(mat==EM_smoke_addive){
		glEnd();
		glEnable(GL_LIGHTING);
		glEnable(GL_FOG);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	}
	if(mat==EM_burn){
		glEnd();
	}
	if(mat==EM_mesh_reflect){
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glEnable(GL_LIGHTING);
		//glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	}
	if(mat==EM_muzzle_flash){
		glEnd();
		glEnable(GL_LIGHTING);
		glEnable(GL_FOG);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	}
#if GL_ARB_shader_objects && GL_ARB_multitexture
	if(use_glsl){
		glUseProgramObjectARB(0);
	}
#endif
	
}

static effect_material_t getEffectMaterial(effect_type_t type){
	switch(type){
		case ET_tire_smoke:
		case ET_blood1:
		case ET_blood2:
		case ET_blood3:
		case ET_blood4:
		case ET_blood5:
		case ET_blood6:
			return EM_splatter_bumped;
		case ET_smoke1:
		case ET_smoke2:
		case ET_smoke3:
		case ET_smoke4:
		case ET_smoke1f:
		case ET_smoke2f:
		case ET_smoke3f:
		case ET_smoke4f:
			return EM_smoke;
		case ET_burst1:
		case ET_burst2:
		case ET_burst3:
		case ET_burst4:
			return EM_smoke_addive;
			break;
		case ET_burn1:
		case ET_burn2:
		case ET_burn3:
		case ET_burn4:
			if(use_glsl)
				return EM_burn;
			else
				return EM_smoke_addive;
		case ET_item:
			return EM_mesh_reflect;
		case ET_muzzle_flash:
			return EM_muzzle_flash;
	}
	return EM_none;
}

static void RenderBillboard(vec3_t pos, float size, float tx, float ty, float tw, float th, bool v3d, float ang){
	vec3_t up, left;
	vec3_t uu, ll;
	vec3_t dir=camera_dir*-1.f;
	left=vec3_t::cross(dir, vec3_t(0,1,0));
	up=vec3_t::cross(left, dir);
	
	uu=up; ll=left;
	up=uu*cosf(ang)-ll*sinf(ang);
	left=uu*sinf(ang)+ll*cosf(ang);
	
#if GL_ARB_multitexture
	if(cap_multiTex){
		glMultiTexCoord3fARB(GL_TEXTURE1_ARB, up.x, up.y, up.z);
		glMultiTexCoord3fARB(GL_TEXTURE2_ARB, -left.x, -left.y, -left.z);
	}
#endif
	
	vec3_t v, n;
	
	n=dir;
	
	v=pos+left*size+up*size;
	if(v3d){
		n=dir+left+up;
		n=n.normalize();
	}
	glTexCoord2f(tx, ty);
	glNormal3f(n.x, n.y, n.z);
	glVertex3f(v.x, v.y, v.z);
	
	v=pos-left*size+up*size;
	if(v3d){
		n=dir-left+up;
		n=n.normalize();
	}
	glTexCoord2f(tx+tw, ty);
	glNormal3f(n.x, n.y, n.z);
	glVertex3f(v.x, v.y, v.z);
	
	v=pos-left*size-up*size;
	if(v3d){
		n=dir-left-up;
		n=n.normalize();
	}
	glTexCoord2f(tx+tw, ty+th);
	glNormal3f(n.x, n.y, n.z);
	glVertex3f(v.x, v.y, v.z);
	
	v=pos+left*size-up*size;
	if(v3d){
		n=dir+left-up;
		n=n.normalize();
	}
	glTexCoord2f(tx, ty+th);
	glNormal3f(n.x, n.y, n.z);
	glVertex3f(v.x, v.y, v.z);
	
	totalPolys+=2;
}
static unsigned int frm=0;

void E_render(){
	int n, i;
	
	glBindTexture(GL_TEXTURE_2D, tex_reflect);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (screen->w-rw)/2, (screen->h-rh)/2, rw, rh);
	
	glDepthMask(GL_FALSE);
	
	camera_dir=(camera_at-camera_from).normalize();
	
	GLuint tex=0;
	frm++;
	
	//glEnable(GL_POLYGON_OFFSET_FILL);
	//glEnable( GL_POLYGON_OFFSET_FILL );
	//glPolygonOffset( 1.0, -4.0 );
	
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,
				 color4f(1.f, 1.f, 1.f, 1.f));
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,
				 color4f(0.f, 0.f, 0.f, 1.f));
	
	for(n=0;n<MAX_MARKS;n++){
		
		mark_t& m=mark[n];
		if(!m.used)
			continue;
		
		if(tex!=m.tex){
			if(tex){
				glEnd();
			}
			glBindTexture(GL_TEXTURE_2D, m.tex);
			tex=m.tex;
			//glBlendEquation(GL_FUNC_ADD);
			glEnable(GL_LIGHTING);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_FOG);
			
			if(tex==tex_bloodStain){
				glDisable(GL_LIGHTING);
				glDisable(GL_FOG);
				glBlendFunc(GL_ZERO, GL_SRC_COLOR);
			}
			
			glBegin(GL_TRIANGLES);
		}
		
		glCallList(mlist+n);
		totalPolys+=m.polys;
		
	}
	
	if(tex){
		glEnd();
	}
	glBlendEquation(GL_FUNC_ADD);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_FOG);
	
	i=0;
	
	for(n=0;n<MAX_EFFECTS;n++)
		if(eff[n].used){
			eff[n].calc_depth();
			i=n+1;
		}
	sort(eff, eff+i);
	
	effect_material_t mat=EM_none;
	
	for(n=i-1;n>=0;n--){
		effect_t& e=eff[n];
		if(!e.used)
			continue;
		if(getEffectMaterial(e.type)!=mat){
			E_endMaterial(mat);
			mat=getEffectMaterial(e.type);
			E_beginMaterial(mat);
		}
		if(e.type==ET_explode){
			// effect without any graphics; this emits other effects.
		}else if(e.type==ET_tire_smoke){
			glColor4f(1.f, 1.f, 1.f, .6f-e.frm);
			RenderBillboard(e.pos, .02f+e.frm*.2f, 0.f, .5f, .25f, .5f, true, 0.f);
		}else if(e.type==ET_blood1||e.type==ET_blood2||e.type==ET_blood3||e.type==ET_blood4){
			
			int steps, step; // motion blur
			float mbPos;
			steps=(int)(1.f*5.f+1.f);
			for(step=0;step<steps;step++){
				
				mbPos=1.f/60.f*(float)step/(float)steps*1.f;
				
				vec3_t vt[4]; vec3_t norm(0, 1, 0);
				float bW=.2f+(e.frm+mbPos)*.8f;
				float tW=.2f+(e.frm+mbPos)*1.2f;
				float l=.05+powf(e.frm+mbPos, .8f)*1.6f*(1.f+powf(e.r1, 16.f)*.4f);
				float tX=.25f*(float)(e.type-ET_blood1);
				float alp;
				alp=1.f;
				if(e.type==ET_blood1){
					l*=2.0f;
					tW*=1.2f;
					alp=.2f-e.frm/1.5f/e.r3*.2f;
				}else if(e.type==ET_blood2){
					l*=2.f;
					tW*=0.8f;
					bW*=0.6f;
				}else if(e.type==ET_blood3){
					l*=1.1f;
					tW*=1.5f;
					bW*=1.0f;
				}else if(e.type==ET_blood4){
					
				}
				bW*=e.r3;
				tW*=e.r3;
				l*=powf(e.r3, .4f);
				alp/=(float)(steps-step);
				
				float dist;
				dist=vec3_t::dot(e.pos-camera_from, camera_dir);
				dist=fabs(dist);
				alp*=min(dist, 1.f);
				
				glColor4f(.4f, .01f, .03f, alp);
				if(!g_blood){
					// safe blood
					if(e.type==ET_blood1){
						glColor4f(.9f, .6f, 0.05f, alp);
					}else if(e.type==ET_blood2){
						glColor4f(1.f, .9f, .1f, alp);
					}else if(e.type==ET_blood3){
						glColor4f(1.f, 1.f, 0.f, alp);
					}else if(e.type==ET_blood4){
						glColor4f(1.f, 1.f, 1.f, alp*.2f);
					}
				}
				l*=.8f+(e.r1*e.r1*e.r1)*.6f;
				bW*=.9f+(e.r1*e.r1*e.r1)*.4f;
				tW*=.9f+(e.r1*e.r1*e.r1)*.4f;
				
				const bool billboard=true;
				
				if(billboard){
					
					vec3_t side;
					vec3_t dir=-camera_dir;
					vec3_t ang;
					
					ang=rotate(vec3_t(0.f, 0.f, 1.f), e.ang);
					side=vec3_t::cross(dir, ang);
					
					vt[0]=-side*bW+(e.pos+e.vel*mbPos);
					vt[1]=side*bW+(e.pos+e.vel*mbPos);
					vt[2]=side*tW+ang*l+(e.pos+e.vel*mbPos);
					vt[3]=-side*tW+ang*l+(e.pos+e.vel*mbPos);
					
					if(vec3_t::dot(rotate(vec3_t(0.f, 1.f, 0.f), e.ang), dir)>=0.f){
						// front side
	#if GL_ARB_multitexture
						if(cap_multiTex){
							norm=rotate(vec3_t(1.f, 0.f, 0.f), e.ang);
							glMultiTexCoord3fARB(GL_TEXTURE1_ARB, norm.x, norm.y, norm.z);
							norm=rotate(vec3_t(0.f, 0.f, 1.f), e.ang);
							glMultiTexCoord3fARB(GL_TEXTURE2_ARB, norm.x, norm.y, norm.z);
						}
						
	#endif
						norm=rotate(vec3_t(0.f, 1.f, 0.f), e.ang);
					}else{
						// front side
	#if GL_ARB_multitexture
						if(cap_multiTex){
							norm=rotate(vec3_t(1.f, 0.f, 0.f), e.ang);
							glMultiTexCoord3fARB(GL_TEXTURE1_ARB, norm.x, norm.y, norm.z);
							norm=rotate(vec3_t(0.f, 0.f, 1.f), e.ang);
							glMultiTexCoord3fARB(GL_TEXTURE2_ARB, norm.x, norm.y, norm.z);
						}
	#endif
						norm=rotate(vec3_t(0.f, -1.f, 0.f), e.ang);
						//glColor4f(1,1,1,0);
					}
					
					norm=dir;

					
					
					
					
				}else{
				
					vt[0]=vec3_t(-bW, 0.f, 0.f);
					vt[1]=vec3_t(bW, 0.f, 0.f);
					vt[2]=vec3_t(tW, 0.f, l);
					vt[3]=vec3_t(-tW, 0.f, l);
					vt[0]=rotate(vt[0], e.ang)+(e.pos+e.vel*mbPos);
					vt[1]=rotate(vt[1], e.ang)+(e.pos+e.vel*mbPos);
					vt[2]=rotate(vt[2], e.ang)+(e.pos+e.vel*mbPos);
					vt[3]=rotate(vt[3], e.ang)+(e.pos+e.vel*mbPos);
					norm=rotate(norm, e.ang);
					
				}
				
				glNormal3f(norm.x, norm.y, norm.z);
				
	#if GL_ARB_shader_objects
				if(use_glsl){
					glVertexAttrib1fARB(attr_time, (e.frm+mbPos)*2.0f);
				}
	#endif
				
				glTexCoord2f(tX, 0.5f);
				glVertex3f(vt[0].x, vt[0].y, vt[0].z);
				glTexCoord2f(tX+.25f, 0.5f);
				glVertex3f(vt[1].x, vt[1].y, vt[1].z);
				glTexCoord2f(tX+.25f, 0.0f);
				glVertex3f(vt[2].x, vt[2].y, vt[2].z);
				glTexCoord2f(tX, 0.0f);
				glVertex3f(vt[3].x, vt[3].y, vt[3].z);
				
				totalPolys+=2;
				
			}
		}else if(e.type==ET_blood5){
			glColor4f(.4f, .01f, .03f, .5f-e.frm);
			if(!g_blood){
				// safe blood
				if(e.type==ET_blood5){
					glColor4f(1.f, 1.f, 1.f, .5f-e.frm);
				}
			}
#if GL_ARB_shader_objects
			if(use_glsl){
				glVertexAttrib1fARB(attr_time, e.frm);
			}
#endif
			RenderBillboard(e.pos, 1.0f+e.frm*4.0f, 0.25f, .5f, .25f, .5f, true, e.ang.z);
		}else if(e.type==ET_blood6){
			glColor4f(.4f, .16f, .18f, (1.f-e.frm)*.6f);
			if(!g_blood){
				// safe blood
				if(e.type==ET_blood6){
					glColor4f(1.f, 1.f, 1.f, .5f-e.frm);
				}
			}
#if GL_ARB_shader_objects
			if(use_glsl){
				glVertexAttrib1fARB(attr_time, 0.f);
			}
#endif
			RenderBillboard(e.pos, .4f+e.frm*0.6f, 0.75f, .5f, .25f, .5f, true, e.ang.z);
		}else if(e.type==ET_smoke1 || e.type==ET_smoke2 || e.type==ET_smoke3 || e.type==ET_smoke4){
			glColor4f(.2f, .2f, .17f, pow((4.f-e.frm)/4., 2.f)*2.f);
			RenderBillboard(e.pos, .4f+e.frm*0.7f, (e.type==ET_smoke2||e.type==ET_smoke4)?0.5f:0.f, 
							(e.type==ET_smoke3||e.type==ET_smoke4)?0.5f:0.f, .5f, .5f, false, e.ang.z);
		}else if(e.type==ET_burst1 || e.type==ET_burst2 || e.type==ET_burst3 || e.type==ET_burst4){
			glColor4f(.9f, .4f, .1f, pow((.2-e.frm)/.2, 2.f)*1.f);
			RenderBillboard(e.pos, .2f+e.frm*4.f, (e.type==ET_burst2||e.type==ET_burst4)?0.5f:0.f, 
							(e.type==ET_burst3||e.type==ET_burst4)?0.5f:0.f, .5f, .5f, false, e.ang.z);
		}else if(e.type==ET_burn1 || e.type==ET_burn2 || e.type==ET_burn3 || e.type==ET_burn4){
			float powr=max(0.f, (.4f-e.frm)/.4f)*3.;
#if GL_ARB_multitexture
			if(cap_multiTex){
				glMultiTexCoord3fARB(GL_TEXTURE3_ARB, e.v1.x, e.v1.y, e.v1.z);
				glMultiTexCoord1fARB(GL_TEXTURE4_ARB, powr*4.f);
			}
#endif
			if(use_glsl)
				glColor4f(.2, .2, .2, pow((1.5f-e.frm)/1.5f, 2.f)*2.f);
			else
			glColor4f(.2+.9f*powr, .2+.4f*powr, .2+.1f*powr, pow((1.5f-e.frm)/1.5f, 2.f)*2.f);
			RenderBillboard(e.pos, .5f+e.frm*1.6f, (e.type==ET_burn2||e.type==ET_burn4)?0.5f:0.f, 
							(e.type==ET_burn3||e.type==ET_burn4)?0.5f:0.f, .5f, .5f, false, e.ang.z);
		}else if(e.type==ET_smoke1f || e.type==ET_smoke2f || e.type==ET_smoke3f || e.type==ET_smoke4f){
			
			vec3_t vt[4]; vec3_t norm(0, 1, 0);
			float bW=.5f+e.frm*0.8f;
			float tW=.5f+e.frm*0.9f;
			float l=.05+powf(e.frm/2.f, .4f)*27.6f*(1.f-e.r1*.7f)*(.3f+sin(e.ang.x)*.8f);
			float tX=(e.type==ET_smoke2||e.type==ET_smoke4)?0.f:.5f;
			float tY=(e.type==ET_smoke3||e.type==ET_smoke4)?0.f:.5f;
			float alp;
			alp=1.f-e.frm/2.f;
			
			
			float dist;
			dist=vec3_t::dot(e.pos-camera_from, camera_dir);
			dist=fabs(dist);
			alp*=min(dist, 1.f);
			
			glColor4f(.2f, .2f, .2f, alp);
		
			l*=.8f+(e.r1*e.r1*e.r1)*.6f;
			
			
				
			vec3_t side;
			vec3_t dir=-camera_dir;
			vec3_t ang;
			
			ang=rotate(vec3_t(0.f, 0.f, 1.f), e.ang);
			side=vec3_t::cross(dir, ang);
			
			vt[0]=-side*bW+e.pos;
			vt[1]=side*bW+e.pos;
			vt[2]=side*tW+ang*l+e.pos;
			vt[3]=-side*tW+ang*l+e.pos;
			
			if(vec3_t::dot(rotate(vec3_t(0.f, 1.f, 0.f), e.ang), dir)>=0.f){
				// front side
#if GL_ARB_multitexture
				if(cap_multiTex){
					norm=rotate(vec3_t(1.f, 0.f, 0.f), e.ang);
					glMultiTexCoord3fARB(GL_TEXTURE1_ARB, norm.x, norm.y, norm.z);
					norm=rotate(vec3_t(0.f, 0.f, 1.f), e.ang);
					glMultiTexCoord3fARB(GL_TEXTURE2_ARB, norm.x, norm.y, norm.z);
				}
				
#endif
				norm=rotate(vec3_t(0.f, 1.f, 0.f), e.ang);
			}else{
				// front side
#if GL_ARB_multitexture
				if(cap_multiTex){
					norm=rotate(vec3_t(1.f, 0.f, 0.f), e.ang);
					glMultiTexCoord3fARB(GL_TEXTURE1_ARB, norm.x, norm.y, norm.z);
					norm=rotate(vec3_t(0.f, 0.f, 1.f), e.ang);
					glMultiTexCoord3fARB(GL_TEXTURE2_ARB, norm.x, norm.y, norm.z);
				}
				
#endif
				norm=rotate(vec3_t(0.f, -1.f, 0.f), e.ang);
				//glColor4f(1,1,1,0);
			}
			
			norm=dir;
				
				
			
			
			glNormal3f(norm.x, norm.y, norm.z);
			
			
			glTexCoord2f(tX, tY+.25f);
			glVertex3f(vt[0].x, vt[0].y, vt[0].z);
			glTexCoord2f(tX+.5f, tY+.25f);
			glVertex3f(vt[1].x, vt[1].y, vt[1].z);
			glTexCoord2f(tX+.5f, tY);
			glVertex3f(vt[2].x, vt[2].y, vt[2].z);
			glTexCoord2f(tX, tY);
			glVertex3f(vt[3].x, vt[3].y, vt[3].z);
			
			totalPolys+=2;
		}else if(e.type==ET_item){
			
			if(!mp->isItemAvailable(e.i1))
				continue; // item doesn't available
			
			float scale;
			scale=min(mp->itemspawn[e.i1], 1.f);
			
			glBindTexture(GL_TEXTURE_2D, tex_item);
			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
			glBegin(GL_QUADS);
			
			
			
			glColor4f(0.1f, 0.2f, 0.3f, 0.8f);
			RenderBillboard(e.pos, 1.8f*scale, 0.f, 0.f, .5f, .5f, false, e.frm);
			
			glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
			RenderBillboard(e.pos, 1.2f*scale, 0.0f, 0.5f, .5f, .5f, false, e.frm*2.f);	
			
			glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
			RenderBillboard(e.pos, 0.6f*scale, 0.5f, 0.f, .5f, .5f, false, 0.f);
			
			glEnd();
			glBindTexture(GL_TEXTURE_2D, tex_reflect);
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
			
			glPushMatrix();
			glTranslatef(e.pos.x, e.pos.y, e.pos.z);
			glScalef(2.5f, 2.5f, 2.5f);
			glScalef(scale, scale, scale);
			glRotatef(e.frm*70.f+((float)e.i1)*107.f, 0.f, 1.f, 0.f);
			
			m_item->render();

			glPopMatrix();
			glEnable(GL_TEXTURE_2D);
			
			glBindTexture(GL_TEXTURE_2D, tex_item);
			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
			glBegin(GL_QUADS);
			
			glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
			RenderBillboard(e.pos, 0.6f*scale, 0.5f, 0.f, .5f, .5f, false, 0.f);
			
			glColor4f(1.0f, 0.2f, 0.1f, 0.5f);
			RenderBillboard(e.pos, 0.5f*scale, 0.0f, 0.5f, .5f, .5f, false, -e.frm);	
			glColor4f(1.0f, 0.2f, 0.1f, 0.8f);
			RenderBillboard(e.pos, 0.17f*scale, 0.0f, 0.5f, .5f, .5f, false, e.frm*2.f);	
			
			glEnd();
			glBindTexture(GL_TEXTURE_2D, tex_reflect);
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
		}else if(e.type==ET_muzzle_flash){
			glColor4f(1.f, 1.f, 1.f, 1.f);
			if(evenFrame)
				drawRay(e.pos, e.pos+e.v1*2., .3f, 0.f, 0.f, 1.f, 1.f);
			else
				drawRay(e.pos, e.pos+e.v1*2., .3f, 0.f, 1.f, 1.f, 0.f);

			
		}else{
			consoleLog("E_render: invalid effect (%d) found, removing\n", (int)e.type);
			e.used=false;
		}
	}
	
	E_endMaterial(mat);
	
	
	
	
	glDepthMask(GL_TRUE);
}



static void cutOffPolygons(vector<vec3_t>& verts, plane_t plane){
	vector<vec3_t> res;
	res.reserve(verts.size()*2);
	
	int n;
	for(n=0;n<verts.size();n+=3){
		vec3_t v1, v2, v3;
		v1=verts[n  ];
		v2=verts[n+1];
		v3=verts[n+2];
		
		
		float d1, d2, d3;
		
		d1=plane.distance(v1);
		d2=plane.distance(v2);
		d3=plane.distance(v3);
		
		vec3_t i1, i2;
		vec3_t o1, o2;
		vec3_t c1, c2;
		
		float per;
		int icnt; // vertex shown
		
		if(d1>=0.f){
			if(d2>=0.f){
				if(d3>=0.f){
					icnt=3;
				}else{
					i1=v1; i2=v2;
					o1=v3;
					icnt=2;
				}
			}else{
				if(d3>=0.f){
					i1=v1; i2=v3;
					o1=v2;
					icnt=2;
				}else{
					i1=v1;
					o1=v2; o2=v3;
					icnt=1;
				}
			}
		}else{
			if(d2>=0.f){
				if(d3>=0.f){
					i1=v2; i2=v3;
					o1=v1;
					icnt=2;
				}else{
					i1=v2;
					o1=v1; o2=v3;
					icnt=1;
				}
			}else{
				if(d3>=0.f){
					i1=v3;
					o1=v1; o2=v2;
					icnt=1;
				}else{
					icnt=0;
				}
			}
		}
		
		if(icnt==0)
			continue; // completely clipped out
		
		if(icnt==3){
			// contains whole polygon
			res.push_back(v1);
			res.push_back(v2);
			res.push_back(v3);
			continue;
		}
		
		if(icnt==2){
			d3=plane.distance(o1);
			d1=plane.distance(i1);
			d2=plane.distance(i2);
			
			per=d1/(d1-d3);
			c1=i1+(o1-i1)*per;
			
			per=d2/(d2-d3);
			c2=i2+(o1-i2)*per;
			
			res.push_back(i1);
			res.push_back(c1);
			res.push_back(c2);
			
			res.push_back(c2);
			res.push_back(i2);
			res.push_back(i1);
			continue;
		}
		
		if(icnt==1){
			d3=plane.distance(i1);
			d1=plane.distance(o1);
			d2=plane.distance(o2);
			
			per=d1/(d1-d3);
			c1=o1+(i1-o1)*per;
			
			per=d2/(d2-d3);
			c2=o2+(i1-o2)*per;
			
			res.push_back(i1);
			res.push_back(c1);
			res.push_back(c2);
		
			continue;
		}
		
	}
	
	res.swap(verts);
}

static int allocMark(){
	int n;
	Uint32 mins; int mini=-1;
	for(n=0;n<MAX_MARKS;n++){
		if(!mark[n].used)
			return n;
		if(mini==-1 || mark[n].otick<mins){
			mins=mark[n].otick;
			mini=n;
		}
	}
	return mini;
}

int E_renderMark(vec3_t pos, vec3_t proj, vec3_t rgb, float a,
				  float radius, float ang, GLuint tex,
				  float tx, float ty, float tw, float th){
	// create 3d square
	vec3_t vt[8];
	vec3_t center;
	vec3_t up, left;
	vec3_t up2, left2;
	vec3_t top, dir;
	plane_t planes[6];
	int n;
	
	tx+=tw/128.f;
	ty+=th/128.f;
	tw-=tw/64.f;
	th-=th/64.f;
	
	proj*=1.125f;
	pos-=proj*(0.125f/1.125f);
	
	dir=proj.normalize()*-1.f;
	if(fabs(dir.y)>.9f){
		top=vec3_t(0.f, 0.f, 1.f);
	}else{
		top=vec3_t(0.f, 1.f, 0.f);
	}
	left2=vec3_t::cross(dir, top).normalize();
	up2=vec3_t::cross(left2, dir).normalize();
	left=left2*cosf(ang)-up2*sinf(ang);
	up=left2*sinf(ang)+up2*cosf(ang);
	
	vt[0]=pos+( left+up)*radius;
	vt[1]=pos+(-left+up)*radius;
	vt[2]=pos+(-left-up)*radius;
	vt[3]=pos+( left-up)*radius;
	
	// extend
	
	for(n=0;n<4;n++){
		vt[n+4]=vt[n]+proj;
	}
	center=pos+proj*.5f;
	
	// create clip planes
	
	planes[0]=plane_t(vt[0], vt[1], vt[2]).toward(center);
	planes[1]=plane_t(vt[4], vt[5], vt[6]).toward(center);
	
	planes[2]=plane_t(vt[0], vt[1], vt[4]).toward(center);
	planes[3]=plane_t(vt[1], vt[2], vt[5]).toward(center);
	planes[4]=plane_t(vt[2], vt[3], vt[6]).toward(center);
	planes[5]=plane_t(vt[3], vt[0], vt[7]).toward(center);
	
	// calc boundary box
	
	aabb_t bound;
	
	for(n=0;n<8;n++)
		bound+=vt[n];
	
	vector<vec3_t> verts;
	mesh *m=mp->m;
	
	// find polygons which may be marked
	
	for(n=0;n<m->count_face;n++){
		vec3_t v1, v2, v3;
		v1=m->vertex[m->face[(n<<2)  ]];
		v2=m->vertex[m->face[(n<<2)+1]];
		v3=m->vertex[m->face[(n<<2)+2]];
		aabb_t bound2(v1, v2, v3);
		if(bound && bound2){
			// overlapped
			verts.push_back(v1);
			verts.push_back(v2);
			verts.push_back(v3);
		}
	}
	
	// cut off and cut off!
	
	for(n=0;n<6;n++){
		
		cutOffPolygons(verts, planes[n]);
		
	}
	
	
	
	glColor4f(rgb.x, rgb.y, rgb.z, a);
	glNormal3f(dir.x, dir.y, dir.z);
	
	for(n=0;n<verts.size();n++){
		
		vec3_t v=verts[n]+dir*.01f;
		vec3_t p=planes[0].project(v);
		float newA;
		
		newA=1.f-fabsf(planes[0].distance(v))/proj.length();
		newA*=a;
		glColor4f(rgb.x, rgb.y, rgb.z, newA);
		
		glTexCoord2f((vec3_t::dot(p-pos, left)/radius*.5+.5)*tw+tx,
					 (vec3_t::dot(p-pos, up)/radius*.5+.5)*th+ty);
		glVertex3f(v.x, v.y, v.z);
		
		
	}
	
	return verts.size()/3;
	
	
}

void E_mark(vec3_t pos, vec3_t proj, vec3_t rgb, float a,
				  float radius, float ang, GLuint tex,
				  float tx, float ty, float tw, float th){
	int mId, cnt;
	mId=allocMark();
	
	mark_t& mk=mark[mId];
	
	glNewList(mlist+mId, GL_COMPILE);
	cnt=E_renderMark(pos, proj, rgb, a, 
					 radius, ang, tex, 
					 tx, ty, tw, th);
	glEndList();
	
	mk.used=true;
	mk.tex=tex;
	mk.pos=pos;
	mk.otick=SDL_GetTicks();
	mk.polys=cnt;
}

void E_smoke(vec3_t vt){
	
	effect_type_t type;
	int n;
	
	for(type=ET_smoke1;type<=ET_smoke4;type=(effect_type_t)((int)type+1)){
		for(n=0;n<1;n++){
			effect_t& e=eff[allocEffect(type)];
			e.pos=vt; e.vel=vec3_t((rnd()-rnd())*1.5f,(rnd()-rnd())*1.0f,(rnd()-rnd())*1.5f);
			e.ang.z=rnd()*M_PI*2.f;
		}
	}
	
}

void E_explodeBig(vec3_t vt){
	
	effect_type_t type;
	int n;
	
	for(type=ET_burn1;type<=ET_burn4;type=(effect_type_t)((int)type+1)){
		for(n=0;n<2;n++){
			effect_t& e=eff[allocEffect(type)];
			e.pos=vt; e.vel=vec3_t((rnd()-rnd())*1.5f,(rnd()-rnd())*1.0f,(rnd()-rnd())*1.5f)*4.f;
			e.ang.z=rnd()*M_PI*2.f;
			e.v1=vt;
			e.r1=1.f;
		}
	}
	
	for(type=ET_smoke1f;type<=ET_smoke4f;type=(effect_type_t)((int)type+1)){
		for(n=0;n<2;n++){
			effect_t& e=eff[allocEffect(type)];
			e.pos=vt;
			e.ang=vec3_t(rnd()*M_PI*.4f, rnd()*M_PI*2.f, (rnd()-.5f)*M_PI*.3f+(M_PI*.5f));
			e.vel=rotate(vec3_t(0.f, 0.f, .5f), e.ang);
			e.vel.y+=.8f;
		}
	}
	
	effect_type_t types[]={ET_burn1, ET_burn2, ET_burn3, ET_burn4};
	
	for(int n=0;n<12;n++){
		effect_t& e=eff[allocEffect(types[rand()%4])];
		e.pos=vt; e.vel=vec3_t(0.f, 0.f, 0.f);
		e.pos+=vec3_t(rnd()-rnd(), rnd()-rnd(), rnd()-rnd())*.8f;
		e.ang.z=rnd()*M_PI*2.f;
	}
	
	{
		effect_t& e=eff[allocEffect(ET_explode)];
		e.pos=vt; e.vel=vec3_t(0.f, 1.5f, 0.f);
	
		
	}
	
}

void E_explode(vec3_t vt){
	
	effect_type_t type;
	int n;
	
	for(type=ET_burn1;type<=ET_burn4;type=(effect_type_t)((int)type+1)){
		for(n=0;n<2;n++){
			effect_t& e=eff[allocEffect(type)];
			e.pos=vt; e.vel=vec3_t((rnd()-rnd())*1.5f,(rnd()-rnd())*1.0f,(rnd()-rnd())*1.5f)*4.f;
			e.ang.z=rnd()*M_PI*2.f;
			e.v1=vt;
			e.r1=1.f;
		}
	}
	
	for(type=ET_smoke1f;type<=ET_smoke4f;type=(effect_type_t)((int)type+1)){
		for(n=0;n<2;n++){
			effect_t& e=eff[allocEffect(type)];
			e.pos=vt;
			e.ang=vec3_t(rnd()*M_PI*.4f, rnd()*M_PI*2.f, (rnd()-.5f)*M_PI*.3f+(M_PI*.5f));
			e.vel=rotate(vec3_t(0.f, 0.f, .5f), e.ang);
			e.vel.y+=.8f;
		}
	}
	
	for(type=ET_smoke1;type<=ET_smoke4;type=(effect_type_t)((int)type+1)){
		for(n=0;n<1;n++){
			effect_t& e=eff[allocEffect(type)];
			e.pos=vt; e.vel=vec3_t((rnd()-rnd())*1.5f,(rnd()-rnd())*1.0f,(rnd()-rnd())*1.5f)*4.f;
			e.ang.z=rnd()*M_PI*2.f;
			e.r1=1.f;
		}
	}
	
	effect_type_t types[]={ET_burst1, ET_burst2, ET_burst3, ET_burst4};
	
	for(int n=0;n<5;n++){
		effect_t& e=eff[allocEffect(types[rand()%4])];
		e.pos=vt; e.vel=vec3_t(0.f, 0.f, 0.f);
		e.ang.z=rnd()*M_PI*2.f;
	}
	
}

void E_blood(vec3_t vt){
	effect_type_t type;
	int n;
	
	for(type=ET_blood1;type<=ET_blood4;type=(effect_type_t)((int)type+1)){
		for(n=0;n<4;n++){
			effect_t& e=eff[allocEffect(type)];
			e.pos=vt; //e.vel=vec3_t(0,rnd()*.3f,0);
			e.ang=vec3_t(rnd()*M_PI*.4f, rnd()*M_PI*2.f, (rnd()-.5f)*M_PI*.3f+(M_PI*.5f));
			e.vel=rotate(vec3_t(0.f, 0.f, .5f), e.ang);
			e.vel.y+=.8f;
			e.r3=1.f;
		}
	}
	for(n=0;n<6;n++){
		effect_t& e=eff[allocEffect(ET_blood5)];
		e.pos=vt; e.vel=vec3_t((rnd()-rnd())*.3f,-.2f,(rnd()-rnd())*.3f);
		e.ang.z=rnd()*M_PI*2.f;
	}
	for(n=0;n<3;n++){
		effect_t& e=eff[allocEffect(ET_blood6)];
		e.pos=vt; e.vel=vec3_t((rnd()-rnd())*.3f,(rnd()-rnd())*.4f-.3f,(rnd()-rnd())*.3f);
		e.ang.z=rnd()*M_PI*2.f;
	}
}

void E_bloodBig(vec3_t vt){
	effect_type_t type;
	int n;
	
	for(type=ET_blood1;type<=ET_blood4;type=(effect_type_t)((int)type+1)){
		for(n=0;n<6;n++){
			effect_t& e=eff[allocEffect(type)];
			e.pos=vt; //e.vel=vec3_t(0,rnd()*.3f,0);
			e.ang=vec3_t(rnd()*M_PI*.4f, rnd()*M_PI*2.f, (rnd()-.5f)*M_PI*.3f+(M_PI*.5f));
			e.vel=rotate(vec3_t(0.f, 0.f, .5f), e.ang);
			e.vel.y+=.8f;
			//e.r1*=1.16f;
			e.r3=3.f;
		}
	}
	for(n=0;n<10;n++){
		effect_t& e=eff[allocEffect(ET_blood5)];
		e.pos=vt; e.vel=vec3_t((rnd()-rnd())*.3f,-.2f,(rnd()-rnd())*.3f);
		e.ang.z=rnd()*M_PI*2.f;
	}
	for(n=0;n<6;n++){
		effect_t& e=eff[allocEffect(ET_blood6)];
		e.pos=vt; e.vel=vec3_t((rnd()-rnd())*.3f,(rnd()-rnd())*.4f-.3f,(rnd()-rnd())*.3f);
		e.ang.z=rnd()*M_PI*2.f;
	}
}


void E_bulletMark(vec3_t pos, vec3_t proj){
	E_mark(pos, proj, vec3_t(1.f, 1.f, 1.f), 1.f, 
		   .1, rnd()*M_PI*2.f, tex_bulletMark,
		   0.f, 0.f, 1.f, 1.f);
}

void E_muzzleFlash(int cc, vec3_t shift){
	effect_t& e=eff[allocEffect(ET_muzzle_flash)];
	client_t& c=cli[cc];
	weapon_t *w=W_find(c.weapon);
	if(!w)
		return;
	e.pos=c.pos+rotate(vec3_t(.3f, 0.4f, .0f)+w->muzzle+shift, c.ang);
	e.v1=rotate(vec3_t(0.f, 0.f, 1.f), c.ang);
}

void E_addMapItems(){
	int n;
	E_removeMapItems(); // remove unneeded item effects
	for(n=0;n<mp->items;n++){
		// create effect
		effect_t& e=eff[allocEffect(ET_item)];
		e.critical=true; // mark as critical (unpurgable)
		e.pos=mp->item[n]+vec3_t(0.f, .5f, 0.f); // set position
									e.i1=n;
		//consoleLog("E_addMapItems: added item effect on (%f, %f, %f)\n", e.pos.x, e.pos.y, e.pos.z);
	}
}
void E_removeMapItems(){
	int n;
	for(n=0;n<MAX_EFFECTS;n++){
		if(eff[n].type==ET_item)
			eff[n].used=false; // remove
	}
}
