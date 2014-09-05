/*
 *  sfx.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/28.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "sfx.h"
#include "map.h"
#include "client.h"
#include "clientgame.h"
#include "server.h"

extern vec3_t camera_from, camera_at;

TMix_Chunk *snd_engine;
static TMix_Chunk *snd[MAX_SOUNDS];
static char *sndname[MAX_SOUNDS];
TMix_Chunk *snd_slip;
int snds=0;

void S_init(){
	TMix_OpenAudio(44100, 1024);
	TMix_ReserveChannels(MAX_CLIENTS);
	
	consoleLog("S_init: loading res/sounds/slip.wav\n");
	
	snd_slip=TMix_LoadWAV("res/sounds/slip.wav");
	
	consoleLog("S_init: loading res/sounds/engine.wav\n");
	
	snd_engine=TMix_LoadWAV("res/sounds/engine.wav");
	
	FILE *f;
	char buf[256];
	char buf2[256];
	f=fopen("res/sounds/sounds.ini", "r");
	while(fgets(buf, 256, f)){
		char *ptr=buf;
		char *ptr2;
		while(*ptr==' ')
			ptr++;
		if(strchr(ptr, '#'))
			*strchr(ptr, '#')=0;
		if(*ptr==0)
			continue;
		ptr[strlen(ptr)-1]=0;
		ptr2=strchr(buf, '=');
		if(ptr2==NULL)
			continue;
		*ptr2=0;
		ptr2++;
		
		sprintf(buf2, "res/%s", ptr2);
		
		consoleLog("S_init: loading %s as %s\n", buf2, buf);
		
		snd[snds]=TMix_LoadWAV(buf2);
		sndname[snds]=new char[strlen(buf)+1];
		strcpy(sndname[snds], buf);
		snds++;
	}	
	fclose(f);
	
	consoleLog("S_init: setting reverb\n");
	
	TMix_ReverbLevel(5);
	TMix_ReverbTime(4.8f);
	TMix_ReverbDelay(.12f);
	TMix_ReverbLPF(.3f);
	TMix_ReverbHPF(.5f);
	TMix_ReverbAirLPF(.6f);
	
}
static bool delayed=true;
static bool odead=false;
static float ofactor=1.f;

void S_update(){
	int n;
	bool od=delayed;
	vec3_t mypos;
	bool cdead;
	if(yourClient==-1){
		mypos=vec3_t(0,0,0);
		cdead=false;
	}else{
		mypos=cli[yourClient].pos;
		cdead=cli[yourClient].is_dead() && (!cli[yourClient].spectate);
	}
	
	mypos=camera_from;
	if(mp->findfloor(mypos+vec3_t(0.f, 100.f, 0.f)).y>mypos.y+.5f){
		delayed=false; // indoor
	}else{
		delayed=true;	// outdoor
	}	 
	TMix_GlobalSpeed(sv_speedFactor);
	if(delayed!=od || cdead!=odead|| sv_speedFactor!=ofactor){
		TMix_ReverbLevel(cdead?12:(delayed?7:12));
		TMix_ReverbDelay(cdead?0.01f:(delayed?.12f:.03f));
		TMix_ReverbTime((cdead?12.f:(delayed?4.8f:2.0f))/sv_speedFactor);
		TMix_ReverbLPF(cdead?.1f:(delayed?.3f:.4f));
		TMix_ReverbHPF(cdead?1.f:(delayed?0.8f:.5f));
		TMix_ReverbAirLPF(cdead?.8f:(delayed?.6f:.5f));
		
	}
	
	// calc ER
	TMix_ERDistance(mp->findfloor(mypos).y-mypos.y);
	
	if((!cdead) && odead)
		TMix_MuteOneShot();
	if(yourClient==-1)
		TMix_DryLevel(cdead?.01f:1.f);
	else{
		TMix_DryLevel(cdead?.01f:min(1.f, cli[yourClient].alive_time));
	}
	odead=cdead; // save current state of whether dead
	ofactor=sv_speedFactor;
	for(n=0;n<MAX_CLIENTS;n++){
		client_t& c=cli[n];
		TMix_ChannelEx ex=S_calc(c.pos, 3.f);
		ex.looped=true;
		float vf;
		vf=(float)SDL_GetTicks()/1000.f*40.f+(float)n;
		float scl;
		if(fabs(c.now_speed)<c.max_speed*.5f){
			scl=2.f;
		}else if(fabs(c.now_speed)<c.max_speed*.75f){
			scl=1.3f;
		}else if(fabs(c.now_speed)<c.max_speed*.9f){
			scl=.9f;
		}else{
			scl=.8f;
		}
		ex.speed=(float)(65536.f*(1.f+fabs(c.now_speed)/c.max_speed*scl*2.f+sinf(vf)*.03f));
		if((c.enable && c.is_avail())!=(bool)TMix_Playing(n)){
			if(c.enable && c.is_avail()){
				TMix_PlayChannelEx(n, snd_engine, ex);
			}else{
				TMix_StopChannel(n);
			}
		}
		TMix_UpdateChannelEx(n, ex);
	}
}

TMix_ChannelEx S_calc(vec3_t origin, float factor){
	TMix_ChannelEx ex;
	float vol;
	factor*=.5f;
	vol=(origin-camera_from).rlength()*factor*256.f;
	
	vec3_t at=camera_at;
	vec3_t up(0.f, 1.f, 0.f);
	vec3_t dir=(camera_at-camera_from).normalize();
	vec3_t right=vec3_t::cross(dir, up);
	float horz;
	horz=vec3_t::dot(right, (origin-camera_from).normalize());
	if(horz<-1.f)
		horz=-1.f;
	if(horz>1.f)
		horz=1.f;
	
	horz=.5f+horz*.5f;
	
	ex.vol_left=(int)(vol*(1.f-horz));
	ex.vol_right=(int)(vol*     horz);
	//ex.vol_left=ex.vol_right=0;
	
	// calc reverbration
	
	vol=(origin-camera_from).rlength()*256.f;
	vol=powf(vol, .5f)*factor;

	
	ex.reverb=(int)vol;
	
	//ex.reverb=(ex.reverb*3)>>2;
	if(mp->findfloor(origin+vec3_t(0.f, 100.f, 0.f)).y>origin.y+.5f){
		ex.reverb>>=1;
	}else{
		ex.reverb>>=2;
	}
	if(ex.reverb<0)
		ex.reverb=0;
	ex.er=255;
	return ex;
}

void S_start(){
	Mix_Music *mus;
	//mus=Mix_LoadMUS("res/musics/m5.ogg");
	mus=Mix_LoadMUS("res/musics/rainbowRoad.ogg");
	Mix_VolumeMusic(100);
	Mix_FadeInMusic(mus, -1, 2000);
}
void S_stop(){
	int n;
	for(n=0;n<MAX_CLIENTS;n++)
		TMix_StopChannel(n);
}

TMix_Chunk *S_find(const char *name){
	int n;
	for(n=snds-1;n>=0;n--){
		if(!strcasecmp(sndname[n], name)){
			return snd[n];
		}
	}
	printf("S_find: unknown sound \"%s\"\n", name);
	return NULL;
}
void S_sound(const char *name, vec3_t pos, float factor, int speed){
	TMix_Chunk *ch=S_find(name);
	if(!ch)
		return;
	TMix_ChannelEx ex;
	ex=S_calc(pos, factor);
	ex.speed=speed;
	TMix_PlayChannelEx(-1, ch, ex);
}
void S_sound(const char *name, float factor, int speed){
	TMix_Chunk *ch=S_find(name);
	if(!ch)
		return;
	TMix_ChannelEx ex;
	ex.vol_left=ex.vol_right=(int)(factor*256.f);
	ex.speed=speed;
	TMix_PlayChannelEx(-1, ch, ex);
}

