/*
 *  clientgame.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/30.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "cmds.h"
#include "clientgame.h"
#include "map.h"
#include "client.h"
#include "font.h"
#include "sfx.h"
#include "weapon.h"
#include "effect.h"
#include "server.h"
#include "hurtfx.h"
#include "clienthud.h"
#include <set>

extern SDL_Surface *screen;
static UDPsocket sock=NULL;
static int sock_channel;
static IPaddress ip;
static int cmdIndex=0;
static set<int> history;

scmd_update_t sstate, sstate2;
float cg_steer=0.f;
int cg_accel=0;
bool cg_fire=false;
float cg_strafe=0.f;
Uint32 cg_oHitTime=0;
int cg_vertical=0.f;
float cg_pitch=0.f;
bool cg_running=false;

int yourClient;
static int yc;

static int ind_update=-1;

static int cg_sfps=0;
static Uint32 cg_osfps=0;
float cg_sfps2=0.f;

Uint32 ocommand;

static void drawProgress(float fade){
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_COLOR_MATERIAL);
	glAlphaFunc(GL_GREATER, 0.01);
	glViewport(0, 0, screen->w, screen->h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(2.f/screen->w, -2.f/screen->h, 1.f);
	glTranslatef(-screen->w*.5f, -screen->h*.5f, 0.f);
	
	char buf[256];
	glColor4f(1.f, 1.f, 1.f, fade);
	sprintf(buf, "Connecting...");
	font_roman36->draw(buf, 20, 20);
	
	SDL_GL_SwapBuffers();
}

static void cliInit(){
	int n;
	avgVel=vec3_t(0.f, 0.f, 0.f);
	screenKick=0.f;
	H_init();
	CH_init();
	for(n=0;n<MAX_CLIENTS;n++){
		cli[n].fired=false;
		scli[n].enable=false;
	}
	show_fraggedMsg(""); // clear fragged msg
}

void CG_connect(const char *addr, const char *name, const char *model){
	
	
	Uint32 ot;
	int tm;
	ot=SDL_GetTicks();
	while(SDL_GetTicks()<ot+250){
		tm=SDL_GetTicks()-ot;
		drawProgress((float)tm/250.f);
		SDL_PumpEvents();
	}
	yourClient=yc=-1;
	int n;
	IPaddress ip2;
	CG_disconnect();
	
	consoleLog("CG_connect: resolving host \"%s\"\n", addr);
	SDLNet_ResolveHost(&ip, addr, DEFAULT_PORT);
	
	SDLNet_Write16(DEFAULT_PORT, &(ip.port));
	sock=SDLNet_UDP_Open(0);
	if(!sock){
		throw "CG_connect: SDLNet error";
	}
	
	consoleLog("CG_connect: binding on UDP port\n");
	if((sock_channel=SDLNet_UDP_Bind(sock, -1, &ip))==-1){
		throw "CG_connect: SDLNet error";
	}
	
	cmd_t cmd;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.type=CCMD_connect;
	cmd.ccmd_connect.protocol=PROTOCOL_VERSION;
	strcpy(cmd.ccmd_connect.name, name);
	strcpy(cmd.ccmd_connect.model, model);
	SDL_Delay(250);
	consoleLog("CG_connect: sending command CCMD_connect\n");
	for(n=0;n<16;n++){
		cmd.index=(cmdIndex++);
		CMD_sendCommand(sock, cmd, ip);
	}
	
	consoleLog("CG_connect: waiting for SCMD_initial or SCMD_deny\n");
	ot=SDL_GetTicks();
	while(SDL_GetTicks()<ot+5000){

		if(CMD_recvCommand(sock, &cmd, &ip2)){
			if(cmd.type!=SCMD_initial && cmd.type!=SCMD_deny)
				continue;
			break;
		}
		drawProgress(1.f);
		SDL_PumpEvents();
	}
	ot=SDL_GetTicks();
	while(SDL_GetTicks()<ot+250){
		tm=SDL_GetTicks()-ot;
		drawProgress(1.f-(float)tm/250.f);
		SDL_PumpEvents();
	}
	
	cg_steer=0.f; cg_accel=0; cg_fire=false;
	
	if(cmd.type==SCMD_initial){
		// success!
		consoleLog("CG_connect: SCMD_initial sent\n");
		if(!mp)
		mp=new map_t(cmd.scmd_initial.mapname);
		E_clear();
		yc=cmd.scmd_initial.yourClient;
	}else if(cmd.type==SCMD_deny){
		// denied.
		consoleLog("CG_connect: SCMD_deny sent\n");
		throw "access denied.";
	}else{
		consoleLog("CG_connect: no command recieved\n");
		throw "failed to connect";
	}
	cg_osfps=SDL_GetTicks();
	ocommand=SDL_GetTicks();
	ind_update=-1;
	history.clear();
	
	char buf[256];
	
	sprintf(buf, "You are %s.", name);
	show_msg(buf);
	
	consoleLog("CG_connect: initializing environment\n");
	
	cliInit();
	
	cmd.type=CCMD_ready;
	cmd.index=(cmdIndex++);
	CMD_sendCommand(sock, cmd, ip);
	
	
	
	show_msg("You have connected");
	SDL_WM_GrabInput(SDL_GRAB_ON);
	SDL_ShowCursor(SDL_DISABLE);
		cg_running=true;
}

void CG_spectate(const char *addr, const char * name, const char *model){
	
	Uint32 ot;
	int tm;
	ot=SDL_GetTicks();
	while(SDL_GetTicks()<ot+250){
		tm=SDL_GetTicks()-ot;
		drawProgress((float)tm/250.f);
		SDL_PumpEvents();
	}
	yourClient=yc=-1;
	int n;
	IPaddress ip2;
	CG_disconnect();
	
	consoleLog("CG_connect: resolving host \"%s\"\n", addr);
	SDLNet_ResolveHost(&ip, addr, DEFAULT_PORT);
	
	SDLNet_Write16(DEFAULT_PORT, &(ip.port));
	sock=SDLNet_UDP_Open(0);
	if(!sock){
		throw "CG_connect: SDLNet error";
	}
	
	consoleLog("CG_connect: binding on UDP port\n");
	if((sock_channel=SDLNet_UDP_Bind(sock, -1, &ip))==-1){
		throw "CG_connect: SDLNet error";
	}
	
	cmd_t cmd;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.type=CCMD_connect_spectate;
	cmd.ccmd_connect.protocol=PROTOCOL_VERSION;
	strcpy(cmd.ccmd_connect.name, name);
	strcpy(cmd.ccmd_connect.model, model);
	SDL_Delay(250);
	consoleLog("CG_connect: sending command CCMD_connect_spectate\n");
	for(n=0;n<16;n++){
		cmd.index=(cmdIndex++);
		CMD_sendCommand(sock, cmd, ip);
	}
	
	consoleLog("CG_connect: waiting for SCMD_initial or SCMD_deny\n");
	ot=SDL_GetTicks();
	while(SDL_GetTicks()<ot+5000){
		
		if(CMD_recvCommand(sock, &cmd, &ip2)){
			if(cmd.type!=SCMD_initial && cmd.type!=SCMD_deny)
				continue;
			break;
		}
		drawProgress(1.f);
		SDL_PumpEvents();
	}
	ot=SDL_GetTicks();
	while(SDL_GetTicks()<ot+250){
		tm=SDL_GetTicks()-ot;
		drawProgress(1.f-(float)tm/250.f);
		SDL_PumpEvents();
	}
	
	cg_steer=0.f; cg_accel=0; cg_fire=false;
	
	if(cmd.type==SCMD_initial){
		// success!
		consoleLog("CG_connect: SCMD_initial sent\n");
		if(!sv_running)
			mp=new map_t(cmd.scmd_initial.mapname);
		E_clear();
		yc=cmd.scmd_initial.yourClient;
	}else if(cmd.type==SCMD_deny){
		// denied.
		consoleLog("CG_connect: SCMD_deny sent\n");
		throw "access denied.";
	}else{
		consoleLog("CG_connect: no command recieved\n");
		throw "failed to connect";
	}
	cg_osfps=SDL_GetTicks();
	ocommand=SDL_GetTicks();
	ind_update=-1;
	history.clear();
	
	char buf[256];
	
	sprintf(buf, "You are %s.", name);
	show_msg(buf);
	
	consoleLog("CG_connect: initializing environment\n");
	
	cliInit();
	
	cmd.type=CCMD_ready;
	cmd.index=(cmdIndex++);
	CMD_sendCommand(sock, cmd, ip);
	
	
	
	show_msg("You have connected");
	SDL_WM_GrabInput(SDL_GRAB_ON);
	SDL_ShowCursor(SDL_DISABLE);
}

void CG_enter(){
	cmd_t cmd;
	cmd.type=CCMD_enter;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.index=(cmdIndex++);
	CMD_sendCommand(sock, cmd, ip);

}

void CG_leave(){
	cmd_t cmd;
	cmd.type=CCMD_leave;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.index=(cmdIndex++);
	CMD_sendCommand(sock, cmd, ip);
}

void CG_disconnect(){
	if(sock){
		consoleLog("CG_disconnect: disconnecting\n");
		SDLNet_UDP_Unbind(sock, sock_channel);
		sock=NULL;
		mp=NULL;
	}
	cg_running=false;
}
static Uint32 ostate;
static void updateState(cmd_t& cmd){
	int n;
	if(cmd.index<=ind_update)
		return;
	ostate=SDL_GetTicks();
	ind_update=cmd.index;
	memcpy(&sstate, &cmd.scmd_update, sizeof(scmd_update_t));
	memcpy(&sstate2, &cmd.scmd_update, sizeof(scmd_update_t));
	for(n=0;n<min(64, MAX_CLIENTS);n++){
		scmd_update_client_t& cc=cmd.scmd_update.clients[n];
		client_t& c=cli[n];
		if(cc.enable==0)
			c.enable=false;
		
		if(strcmp(c.model, cc.model)){
			strcpy(c.model, cc.model);
			if(!cc.spectate)
			C_setup(n);
		}
		if(strcmp(c.weapon, cc.weapon)){
			strcpy(c.weapon, cc.weapon);
			c.weapFrame=0.f; // reset weapon animation
		}
		if(strcmp(c.name, cc.name)){
			strcpy(c.name, cc.name);
		}
		c.spectate=cc.spectate;
		c.pos=cc.pos;
		c.vel=cc.vel;
		c.ang=cc.ang;
		c.mang=cc.mang;
		c.slip=cc.slip;
		c.now_speed=unrotate(c.vel, c.ang).z;
		if(!sv_running){
			c.weapon_wait=cc.weapon_wait;
			c.health=cc.health;
			c.dead_time=cc.dead_time;
			c.ammo=cc.ammo;
			c.alive_time=cc.alive_time;
			c.score=cc.score;
			c.view_steer=cc.view_steer;
		}
		
		if(cc.enable)
			c.enable=true;
	}
	for(n=0;n<MAX_PROJECTILES;n++){
		scmd_update_projectile_t& pp=cmd.scmd_update.projectile[n];
		projectile_t& p=projectile[n];
		if(pp.used==0)
			p.used=false;
		if(strcmp(p.weapon, pp.weapon)){
			strcpy(p.weapon, pp.weapon);
		}
		p.pos=pp.pos;
		p.vel=pp.vel;
		if(pp.used)
			p.used=true;
	}
	cg_sfps++;
	if(SDL_GetTicks()>cg_osfps+500){
		cg_sfps2=(float)cg_sfps/(float)(int)(SDL_GetTicks()-cg_osfps)*1000.f;
		cg_sfps=0;
		cg_osfps=SDL_GetTicks();
	}	
	yourClient=yc;
	if(!sv_running)
		sv_speedFactor=cmd.scmd_update.speed;
	
}
static void interpolateState(){
	float scl;
	if(sv_running)
		return;
	scl=(float)(int)(SDL_GetTicks()-ostate)*.001f*sv_speedFactor;;
	for(int n=0;n<min(64, MAX_CLIENTS);n++){
		scmd_update_client_t& cc=sstate2.clients[n];
		scmd_update_client_t& cc2=sstate.clients[n];
		client_t& c=cli[n];
		c.pos=cc.pos;
		c.pos+=c.vel*scl;
		c.ang+=c.mang*scl;
		cc2.pos=cvec3(c.pos);
	}
}
static void pollCmds(){
	cmd_t cmd; IPaddress addr;
	while(CMD_recvCommand(sock, &cmd, &addr)){
		ocommand=SDL_GetTicks();
		if(history.find(cmd.index)==history.end()){
			history.insert(cmd.index);
			if(history.size()>1024)
				history.erase(history.begin());
			switch(cmd.type){
				case SCMD_update:
					updateState(cmd);
					break;
				case SCMD_msg:
					show_msg(cmd.scmd_msg.msg);
					break;
				case SCMD_sound:
					if(cmd.scmd_sound.is3d){
						S_sound(cmd.scmd_sound.name, cmd.scmd_sound.pos, cmd.scmd_sound.factor, cmd.scmd_sound.speed);
					}else{
						S_sound(cmd.scmd_sound.name, cmd.scmd_sound.factor, cmd.scmd_sound.speed);
					}
					/*TMix_Chunk *ch;
					// find the sound
					ch=S_find(cmd.scmd_sound.name);
					if(!ch) 
						break; // sound not found
					{
						TMix_ChannelEx ex;
						if(cmd.scmd_sound.is3d){
							ex=S_calc(cmd.scmd_sound.pos, cmd.scmd_sound.factor);
						}else{
							ex.speed=cmd.scmd_sound.speed;
							ex.vol_left=ex.vol_right=(int)(cmd.scmd_sound.factor*256.f);
						}
						TMix_PlayChannelEx(-1, ch, ex);
					}*/
					break;	
				case SCMD_blood:
					E_blood(cmd.scmd_blood.pos);
					break;
				case SCMD_blood_big:
					E_bloodBig(cmd.scmd_blood_big.pos);
					break;
				case SCMD_explode:
					E_explode(cmd.scmd_blood.pos);
					break;
				case SCMD_explode_big:
					E_explodeBig(cmd.scmd_blood.pos);
					break;
				case SCMD_bullet_impact:
					E_bulletMark(cmd.scmd_bullet_impact.pos, cmd.scmd_bullet_impact.proj);
					break;
				case SCMD_screen_kick:
					CG_screenKick(cmd.scmd_screen_kick.amount);
					break;
				case SCMD_hurt:
					H_hurt(cmd.scmd_hurt.amount, cmd.scmd_hurt.from);
					break;
				case SCMD_fired:
					cli[cmd.scmd_fired.cli].fired=true;
					weapon_t *w;
					w=W_find(cli[cmd.scmd_fired.cli].weapon);
					if(w)
						cli[cmd.scmd_fired.cli].weapFrame=(float)(w->getFrames+w->idleFrames)+.01f;
					break;
				case SCMD_fragged_msg:
					show_fraggedMsg(cmd.scmd_msg.msg);
					break;
				case SCMD_hit:
					cg_oHitTime=SDL_GetTicks();
					S_sound("hitnotify", 5.f);
					break;
			}
		}
		cmd_t cmd2;
		cmd2.magic=PROTOCOL_MAGIC;
		cmd2.type=CCMD_arrive;
		cmd2.ccmd_arrive.cmdindex=cmd.index;
		cmd2.index=(cmdIndex++);
		CMD_sendCommand(sock, cmd2, ip);
	}
	
}

static float osteer=10.f;
static int oaccel=-10;
static Uint32 osend=0;

static void sendCtrl(){

	if(SDL_GetTicks()>osend+1000 || osteer!=cg_steer || oaccel!=cg_accel){
		
		cmd_t cmd;
		cmd.type=CCMD_control;
		cmd.magic=PROTOCOL_MAGIC;
		cmd.ccmd_control.accel=cg_accel;
		cmd.ccmd_control.steer=cg_steer;
		cmd.ccmd_control.sw=CCCSW_none;
		if(cg_fire)
			cmd.ccmd_control.sw|=CCCSW_fire;
		if(cg_vertical>0)
			cmd.ccmd_control.sw|=CCCSW_raise;
		if(cg_vertical<0)
			cmd.ccmd_control.sw|=CCCSW_lower;
		cmd.ccmd_control.strafe=cg_strafe;
		cmd.ccmd_control.pitch=cg_pitch;
		
		cmd.index=(cmdIndex++);
		CMD_sendCommand(sock, cmd, ip);
		
	}
	
}

void CG_screenKick(float amount){
	screenKick=max(screenKick, amount);
}

void CG_framenext(float dt){
	dt*=sv_speedFactor;
	sendCtrl();
	pollCmds();
	interpolateState();
	M_cframenext(dt);
	C_cframenext(dt);
	W_cframenext(dt);
	E_cframenext(dt);
	H_cframenext(dt);
	CH_cframenext(dt);
	S_update();
}



