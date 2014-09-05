/*
 *  weapon.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/09/13.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "weapon.h"
#include "glpng.h"
#include "client.h"
#include "server.h"
#include "map.h"
#include "effect.h"

#include <sys/types.h>
#include <dirent.h>
//#include <fnmatch.h>

static vector<weapon_t *> weapons;
projectile_t projectile[MAX_PROJECTILES];

GLuint tex_bullet;

mesh *m_bullet;

#if GL_ARB_shader_objects
static GLhandleARB prg_bullet;

#endif

void W_init(){
	
	consoleLog("W_init: scanning res/weapons\n");
	
	DIR *d=opendir("res/weapons");
	
	dirent *e;
	
	while(e=readdir(d)){
		char nm[256];
		strcpy(nm, e->d_name);
		char *ptr=nm;
		if(strrchr(ptr, '/'))
			ptr=strrchr(ptr, '/')+1;
		if(ptr[4]=='.' && ptr[5]=='i' && ptr[6]=='n' && ptr[7]=='i'
		/*!fnmatch("????.ini", ptr, FNM_PATHNAME|FNM_CASEFOLD)*/){
			
			ptr[4]=0;
			try{
				weapons.push_back(new weapon_t(ptr));
				consoleLog("W_init: found weapon \"%s\"\n", ptr);
			}catch(const char *str){consoleLog("W_init: couldn't open weapon \"%s\": %s\n", ptr, str);
			}catch(...){consoleLog("W_init: couldn't open weapon \"%s\"\n", ptr);}
		}
	}
	if(weapons.size()==0){
		throw "W_init: No weapon found.";
	}
	
	int n;
	for(n=0;n<MAX_PROJECTILES;n++)
		projectile[n].used=false;
	
	consoleLog("W_init: loading res/sprites/bullet.png\n");
	
	glGenTextures(1, &tex_bullet);
	glBindTexture(GL_TEXTURE_2D, tex_bullet);
	glpngLoadTexture("res/sprites/bullet.png", true);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	consoleLog("W_init: loading res/models/bullet.x\n");
	m_bullet=new mesh("res/models/bullet.x");
	
#if GL_ARB_shader_objects
	
	if(cap_glsl){
		
		// load bullet shader
		
		prg_bullet=create_program("res/shaders/bullet.vs", "res/shaders/bullet.fs");
		if(prg_bullet)
			consoleLog("W_init: compiled program \"bullet\"\n");
		else
			consoleLog("W_init: couldn't compile program \"bullet\"\n");
		
		
	}else{
#endif
		
		consoleLog("W_init: no programs to compile\n");
		
#if GL_ARB_shader_objects
	}	
#endif
	
}

void W_clear(){
	int n;
	for(n=0;n<MAX_PROJECTILES;n++)
		projectile[n].used=false;
}

weapon_t::weapon_t(const char *nam){
	strcpy(this->name, nam);
	char fn[256];
	sprintf(fn, "res/weapons/%s.x", nam);
	m=new scene_t(fn);
	sprintf(fn, "res/icons/weapons/%s.png", nam);
	glGenTextures(1, &icon);
	glBindTexture(GL_TEXTURE_2D, icon);
	glpngLoadTexture(fn, false);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	sprintf(fn, "res/weapons/%s.ini", nam);
	FILE *f=fopen(fn, "r");
	if(f==NULL){
		throw "weapon_t: couldn't open weapon";
	}
	memset(issound, 0, sizeof(issound));
	memset(sound, 0, sizeof(sound));
	splashRange=0.f;
	splashDamage=0.f;
	sndscale[0]=sndscale[1]=sndscale[2]=sndscale[3]=1.f;
	getFrames=1;
	idleFrames=1;
	fireFrames=1;
	spread=0.f;
	gravityScale=1.f;
	lifeTime=5.f;
	halfSpeedTime=1000.f;
	fpPos=vec3_t(0.f);
	strcpy(fragMsg, "%s was killed by %s");
	strcpy(suicideMsg, "%s became bored with his/her life");
	strcpy(fullName, name);
	char str[256];
	char *ptr;
	char *name;
	char group[256];
	char *arg;
	while(fgets(str, 256, f)){
		if(str[strlen(str)-1]=='\n')
			str[strlen(str)-1]=0;
		ptr=str;
		while(*ptr==' ')
			ptr++;
		if(ptr[0]=='*')
			strcpy(group, ptr+1);
		else if(ptr[0]=='#')
			continue;
		name=ptr;
		while(*name && *name!=' '){
			name++;
		}
		if(*name){
			*name=0;
			arg=name+1;
		}else{
			arg=name;
		}
		name=ptr;
		if(!strcasecmp(name, "skip")){
			throw "weapon_t: couldn't accept weapon";
		}
		if(!strcasecmp(group, "behavior")){
			if(!strcasecmp(name, "mode")){
				if(!strcasecmp(arg, "hitscan"))
					mode=WM_hitscan;
				else if(!strcasecmp(arg, "projectile"))
					mode=WM_projectile;
			}else if(!strcasecmp(name, "apperance")){
				if(!strcasecmp(arg, "bullet"))
					apperance=WAM_bullet;
				else if(!strcasecmp(arg, "rocket"))
					apperance=WAM_rocket;
			}else if(!strcasecmp(name, "interval")){
				interval=atof(arg);
			}else if(!strcasecmp(name, "speed")){
				speed=atof(arg);
			}else if(!strcasecmp(name, "damage")){
				damage=atof(arg);
			}else if(!strcasecmp(name, "ammo")){
				defaultAmmo=atoi(arg);
			}else if(!strcasecmp(name, "splashdamage")){
				splashDamage=atoi(arg);
			}else if(!strcasecmp(name, "splashrange")){
				splashRange=atoi(arg);
			}else if(!strcasecmp(name, "fragmsg")){
				strcpy(fragMsg, arg);
			}else if(!strcasecmp(name, "splashfragmsg")){
				strcpy(splashFragMsg, arg);
			}else if(!strcasecmp(name, "suicidemsg")){
				strcpy(suicideMsg, arg);
			}else if(!strcasecmp(name, "spread")){
				spread=atof(arg);
			}else if(!strcasecmp(name, "level")){
				level=atof(arg);
			}
		}else if(!strcasecmp(group, "sound")){
			if(!strcasecmp(name, "fire0")){
				strcpy(sound[0], arg); issound[0]=true;
			}else if(!strcasecmp(name, "fire1")){
				strcpy(sound[1], arg); issound[1]=true;
			}else if(!strcasecmp(name, "fire2")){
				strcpy(sound[2], arg); issound[2]=true;
			}else if(!strcasecmp(name, "fire3")){
				strcpy(sound[3], arg); issound[3]=true;
			}else if(!strcasecmp(name, "scale0")){
				sndscale[0]=atof(arg);
			}else if(!strcasecmp(name, "scale1")){
				sndscale[1]=atof(arg);
			}else if(!strcasecmp(name, "scale2")){
				sndscale[2]=atof(arg);
			}else if(!strcasecmp(name, "scale3")){
				sndscale[3]=atof(arg);
			}
		}else if(!strcasecmp(group, "appearance")){
			if(!strcasecmp(name, "muzzle")){
				muzzle=parse_vec3(arg);
			}else if(!strcasecmp(name, "name")){
				strcpy(fullName, arg);
			}else if(!strcasecmp(name, "firstpersonpos")){
				fpPos=parse_vec3(arg);
			}
		}else if(!strcasecmp(group, "animation")){
			if(!strcasecmp(name, "getframes")){
				getFrames=atoi(arg);
			}else if(!strcasecmp(name, "idleframes")){
				idleFrames=atoi(arg);
			}else if(!strcasecmp(name, "fireframes")){
				fireFrames=atoi(arg);
			}
		}
	}
	fclose(f);
}

void weapon_t::render(int c, vec3_t s){
	static bool b=true;
	m->render(cli[c].weapFrame);
	if(cli[c].fired){
		E_muzzleFlash(c, s);
		cli[c].fired=false;
	}
}

vec3_t weapon_t::getFirePos(){
	return muzzle;
}

float weapon_t::calcDamage(float time){
	return damage*powf(.5f, 2.f*time/halfSpeedTime);
}

weapon_t *W_find(const char *name){
	vector<weapon_t *>::iterator it;
	for(it=weapons.begin();it!=weapons.end();it++){
		if(!strcasecmp((*it)->name, name))
			return *it;
	}
	return NULL;
}

weapon_t *W_find(int i){
	return weapons[i];
}



static void W_explode(vec3_t pos, const char *name, int by){ // when hit (both hitscan and projectile, client and map)
	weapon_t *w=W_find(name);
	if(w==NULL)
		return;
	// TODO: explode or bullethit
	if(w->splashRange>=.1f && w->splashDamage>=.1f){
		splashDamage(pos, w->splashRange, w->splashDamage, by);
	}
	if(w->apperance==WAM_rocket){
		SV_explode(pos);
		//SV_sound("exp4", pos, 64.f);
		//SV_sound("exp4", pos, 64.f);
		SV_sound("exp4", pos, 256.f);
		//SV_sound("exp1", pos, 48.f);
		SV_screenKick(.2f);
	
	}else if(w->apperance==WAM_bullet){
		
	}
}
static void W_bulletImpact(vec3_t pos, vec3_t norm){
	
	SV_bulletImpact(pos, norm);
	char bf[5];
	sprintf(bf, "ric%d", (rand()%3)+1);
	SV_sound(bf, pos, 10.f);

}

void W_framenext(float dt){
	int n;
	for(n=0;n<MAX_PROJECTILES;n++){
		projectile_t& p=projectile[n];
		if(!p.used)
			continue;
		weapon_t *w=W_find(p.weapon);
		if(w==NULL)
			return;
		vec3_t v1, v2;
		v1=p.pos;
		v2=p.pos+p.vel*dt;
		p.vel.y-=1.f*w->gravityScale*dt;
		p.vel*=powf(.5f, dt/w->halfSpeedTime);
		
		vector<raycast_t> res;
		bool hit=false;
		raycast(v1, v2, res);
		sort(res.begin(), res.end());
		for(vector<raycast_t>::iterator it=res.begin();it!=res.end();it++){
			if(it->dest==HD_client && it->cli!=p.by){
				W_explode(it->pos, w->name, p.by);
				cli[it->cli].damage(w->calcDamage(p.frm), v1, p.by);
				p.used=false;
				break;
			}
			if(it->dest==HD_map){
				vec3_t vv1, vv2, vv3;
				W_explode(it->pos, w->name, p.by);
				vv1=mp->m->vertex[mp->m->face[(it->poly<<2)  ]];
				vv2=mp->m->vertex[mp->m->face[(it->poly<<2)+1]];
				vv3=mp->m->vertex[mp->m->face[(it->poly<<2)+2]];
				W_bulletImpact(it->pos, -(plane_t(vv1, vv2, vv3).toward(v1).n)*.1f);
				p.used=false;
				break;
			}
		}
		
		p.pos=v2;
		p.frm+=dt;
		
		if(p.frm>w->lifeTime)
			p.used=false;
		
	}
}
void W_cframenext(float dt){
	
}

static int allocProjectile(){
	int n;
	for(n=0;n<MAX_PROJECTILES;n++)
		if(!projectile[n].used){
			projectile[n].used=true;
			return n;
		}
	return max_element(projectile, projectile+MAX_PROJECTILES)-projectile;
}



void W_fire(int cl, float delay){
	client_t& c=cli[cl];
	if(!c.has_weapon())
		return;
	weapon_t *w=W_find(c.weapon);
	if(w==NULL)
		return;
	
	vec3_t v1, v2, dir;
	float spr;
	spr=tanf(w->spread*M_PI/180.f)*2.f;
	v1=c.pos+rotate(c.getWeaponPos()+w->getFirePos(), c.ang); 
	dir=rotate(vec3_t(spr*(rnd()-.5f), spr*(rnd()-.35f), 1.f), c.ang).normalize();
	//v2=c.pos+rotate(vec3_t(.3f+spr*1000.f, 0.4f+spr*1000.f, 1000.f), c.ang);
	v2=v1+dir*1000.f;
	
	SV_fired(cl);
	
	if(w->mode==WM_hitscan){
		vector<raycast_t> res;
		raycast(v1, v2, res);
		sort(res.begin(), res.end());
		
		for(vector<raycast_t>::iterator it=res.begin();it!=res.end();it++){
			if(it->dest==HD_client && it->cli!=cl){
				W_explode(it->pos, w->name, cl);
				cli[it->cli].damage(w->damage, v1, cl);
				//SV_blood(it->pos);
				//SV_sound("impact", it->pos, 2.f);
				break;
			}
			if(it->dest==HD_map){
				vec3_t vv1, vv2, vv3;
				W_explode(it->pos, w->name, cl);
				vv1=mp->m->vertex[mp->m->face[(it->poly<<2)  ]];
				vv2=mp->m->vertex[mp->m->face[(it->poly<<2)+1]];
				vv3=mp->m->vertex[mp->m->face[(it->poly<<2)+2]];
				W_bulletImpact(it->pos, -(plane_t(vv1, vv2, vv3).toward(v1).n)*.1f);
				break;
			}
		}
	}else if(w->mode==WM_projectile){
		// prescan - hit to too near target
		vector<raycast_t> res;
		bool hit=false;
		raycast(v1, v2, res);
		sort(res.begin(), res.end());
		for(vector<raycast_t>::iterator it=res.begin();it!=res.end();it++){
			if(it->dist>w->speed*(.01f+delay)){
				break;
			}	
			if(it->dest==HD_client && it->cli!=cl){
				W_explode(it->pos, w->name, cl);
				cli[it->cli].damage(w->calcDamage(it->dist/w->speed), v1, cl);
				hit=true;
				break;
			}
			if(it->dest==HD_map){
				vec3_t vv1, vv2, vv3;
				W_explode(it->pos, w->name, cl);
				vv1=mp->m->vertex[mp->m->face[(it->poly<<2)  ]];
				vv2=mp->m->vertex[mp->m->face[(it->poly<<2)+1]];
				vv3=mp->m->vertex[mp->m->face[(it->poly<<2)+2]];
				W_bulletImpact(it->pos, -(plane_t(vv1, vv2, vv3).toward(v1).n)*.1f);
				hit=true;
				break;
			}
		}
		
		if(hit)
			return; // already hit
		
		// launch a projectile
		
		projectile_t& p=projectile[allocProjectile()];
		p.pos=v1;
		p.frm=0.f;
		p.vel=dir*w->speed+c.vel;
		p.pos+=p.vel*delay;
		strcpy(p.weapon, w->name);
		p.by=cl;
		
	}
}

weapon_t *W_choose(int rank){
	
	// calculate proper level for rank
	
	float lv;
	lv=(float)(rank+1)*100.f/(float)(C_count()+1);
	
	// calculate chance of weapons depending on rank
	vector<float> chances;
	int n;
	float ch, total=0.f;
	
	for(n=0;n<weapons.size();n++){
		weapon_t *w=weapons[n];
		ch=1.f/powf(fabs(w->level-lv)/40.f, 2.f);
		chances.push_back(ch);
		total+=ch;
	}
	
	// normalize chance
	
	total=1.f/total;
	for(n=0;n<weapons.size();n++)
		chances[n]*=total;
	
	// choose
	
	ch=rnd(); total=0.f;
	
	for(n=0;n<weapons.size();n++){
		total+=chances[n];
		if(total>=ch)
			return weapons[n];
	}
	
	consoleLog("W_choose: no weapon was chosen; this SHOULDN'T happen\n");
	
	return weapons[rand()%weapons.size()];
}

int W_indexOf(weapon_t *w){
	return find(weapons.begin(), weapons.end(), w)-weapons.begin();
}

int W_count(){
	return weapons.size();
}

void W_render(){
	int n;
	for(n=0;n<MAX_PROJECTILES;n++){
		projectile_t& p=projectile[n];
		if(!p.used)
			continue;
		weapon_t *w=W_find(p.weapon);
		if(!w)
			continue;
		
		if(w->apperance==WAM_bullet || w->apperance==WAM_rocket){
			//consoleLog("%f, %f, %f\n", p.pos.x, p.pos.y, p.pos.z);
			glBindTexture(GL_TEXTURE_2D, tex_bullet);
#if GL_ARB_shader_objects
			if(use_glsl && use_hdr){
				glUseProgramObjectARB(prg_bullet);
			}
#endif
			

			if(sv_speedFactor<.5f && (p.pos-camera_from).length()<8.f){

				glPushMatrix();
				glTranslatef(p.pos.x, p.pos.y, p.pos.z);
				vec3_t ang=unrotate(p.vel);
				glRotatef(-ang.y/M_PI*180.f, 0.f, 1.f, 0.f);
				glRotatef(-ang.x/M_PI*180.f, 1.f, 0.f, 0.f);
				glScalef(2.f, 2.f, 2.f);
				m_bullet->render();
				glPopMatrix();
			}else{
				glDisable(GL_LIGHTING);
				glDisable(GL_ALPHA_TEST);
				glDepthMask(GL_FALSE);
				glBegin(GL_QUADS);
				glColor4f(1.f, 1.f, 1.f, 1.f);
				drawRay(p.pos-p.vel*min(.07f*sv_speedFactor, p.frm), p.pos, .03f, 0.f, 0.f, 1.f, 1.f);
				glEnd();
				glEnable(GL_ALPHA_TEST);
				glEnable(GL_LIGHTING);
				glDepthMask(GL_TRUE);
			}
		}
		
#if GL_ARB_shader_objects
		if(use_glsl){
			glUseProgramObjectARB(0);
		}
#endif
		
	}
	
	glEnable(GL_TEXTURE_2D);
}
