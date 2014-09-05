/*
 *  server.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/30.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "server.h"
#include "client.h"
#include "map.h"
#include "ai.h"
#include "weapon.h"
#include <set>
#include "clientgame.h"

static char mapname[256];
static volatile UDPsocket sock=NULL;
static SDL_Thread *th;
static int cmdIndex=0;

float sv_fps=0.f;
int sv_kicktime=3000; // time to kick the player not resonding
bool sv_running=false;;
float sv_speedFactor=1.0f;
float sv_speedBase=1.f;
bool sv_sdlTimer=true;
int sv_resendDelay=60;
int sv_resendTimes=10;

int sv_statCmdsSent;
int sv_statCmdsRecv;
int sv_statLostCmds;
int sv_statBytesSent;
int sv_statBytesRecv;


void SV_init(){

}

class ctrl_t : public spectate_controller_t{
public:
	float steer; int accel;
	bool firing;
	float str, vert;
	float pitch;
	virtual void bind(client_t * c){
		steer=0.f; accel=0.f; firing=false;
		str=0.f; vert=0.f;
		this->controller_t::bind(c);
	}
	virtual void framenext(float dt){}
	virtual float get_steering(){return steer;}
	virtual int get_accel(){return accel;}
	virtual void hit(vec3_t){}
	virtual bool fire(){return firing;}
	virtual float get_strafe(){return str;}
	virtual float get_vertical(){return vert;}
	virtual float get_pitch(){return pitch;}
};

static void framenext(float dt){
	dt*=sv_speedFactor;
	M_framenext(dt);
	C_framenext(dt);
	W_framenext(dt);
}

static int findClient(IPaddress addr){
	int n;
	for(n=0;n<MAX_CLIENTS;n++)
		if(cli[n].addr==addr && (cli[n].enable || cli[n].spectate))
			return n;
	return -1;
}

struct cmd_history_t{
	cmd_t cmd;
	IPaddress addr;
	Uint32 sendTime;
	int times;
	cmd_history_t(){}
	cmd_history_t(cmd_t& c, IPaddress a){
		cmd=c; addr=a;
		sendTime=SDL_GetTicks();
		times=1;
	}
	bool operator <(const cmd_history_t& hs)const{
		return cmd.index<hs.cmd.index;
	}
};
static set<cmd_history_t> history;

static void broadcastCmd(cmd_t& cmd, int c=-1){
	for(int n=0;n<MAX_CLIENTS;n++){
		if((cli[n].enable || cli[n].spectate) && dynamic_cast<ctrl_t *>(cli[n].ctrl) && (n==c || c==-1)){
			cmd.index=(cmdIndex++);
			if(cmd.type!=SCMD_update) // SCMD_update requires band width, so disable resending
				history.insert(cmd_history_t(cmd, cli[n].addr));
			sv_statCmdsSent++;
			sv_statBytesSent+=CMD_getCmdSize(cmd);
			CMD_sendCommand(sock, cmd, cli[n].addr);
			
		}
	}
}

static void manageHistory(){
	// command remains unarrived, resend.
	set<cmd_history_t>::iterator it=history.begin(), it2;
	while(it!=history.end()){
		if(SDL_GetTicks()>it->sendTime+sv_resendDelay){
			// unarrived, resend
			
			cmd_history_t& hs=const_cast<cmd_history_t&>(*it);
			CMD_sendCommand(sock, it->cmd, it->addr);
			sv_statCmdsSent++;
			sv_statBytesSent+=CMD_getCmdSize(it->cmd);
			hs.times++;
			if(it->times>=sv_resendTimes){
				//char bf[256];
				//sprintf(bf, "lost msg: %d", it->cmd.type);
				//SV_msg(bf);
				it2=it;it++;
				history.erase(it2);
				sv_statLostCmds++; // too many lost
			}else{
				hs.sendTime=SDL_GetTicks();
			}
		}else{
			it++;
		}
	}
}

static void sendState(){
	cmd_t cmd;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.type=SCMD_update;
	
	cmd.scmd_update.speed=sv_speedFactor;
	
	int n;
	
	for(n=0;n<MAX_CLIENTS;n++){
		scmd_update_client_t& cc=cmd.scmd_update.clients[n];
		
		client_t& c=cli[n];
		cc.enable=c.enable?0xdeadbeef:0;
		cc.spectate=c.spectate?0xbeefdead:0;
		strcpy(cc.model, c.model);
		strcpy(cc.name, c.name);
		cc.pos=cvec3(c.pos);
		cc.vel=cvec3(c.vel);
		cc.ang=cvec3(c.ang);
		cc.mang=cvec3(c.mang);
		cc.slip=c.slip;
		strcpy(cc.weapon, c.weapon);
		cc.weapon_wait=c.weapon_wait;
		cc.ammo=c.ammo;
		cc.health=c.health;
		cc.dead_time=c.dead_time;
		cc.alive_time=c.alive_time;
		cc.score=c.score;
		cc.view_steer=c.view_steer;
		if((c.enable || c.spectate) && SDL_GetTicks()>c.oresp+sv_kicktime && dynamic_cast<ctrl_t *>(c.ctrl) && (c.ready)){
			c.spectate=false;
			SV_kick(n);
		}
		if((c.enable || c.spectate) && SDL_GetTicks()>c.oresp+sv_kicktime*10 && dynamic_cast<ctrl_t *>(c.ctrl) && (!c.ready)){
			c.spectate=false;
			SV_kick(n);
		}
	}
	
	for(n=0;n<MAX_PROJECTILES;n++){
		scmd_update_projectile_t& pp=cmd.scmd_update.projectile[n];
		projectile_t& p=projectile[n];
		pp.used=p.used;
		strcpy(pp.weapon, p.weapon);
		pp.pos=cvec3(p.pos);
		pp.vel=cvec3(p.vel);
	}
	
	broadcastCmd(cmd);
}

static void ServerFrame(float dt){
	try{
		cmd_t cmd;
		IPaddress addr;
		Uint8 *keys=SDL_GetKeyState(NULL);
		sv_speedFactor=(keys[('k')]?.05f:1.f)*sv_speedBase;
		int n;
		char buf[256];
		static int frm=0;
		framenext(dt);
		if(((frm++)&1)==0 || cg_running)
			sendState();
		while(CMD_recvCommand(sock, &cmd, &addr)){
			if(cmd.magic!=PROTOCOL_MAGIC)
				continue; // noise; ignore
			sv_statCmdsRecv++;
			sv_statBytesRecv+=CMD_getCmdSize(cmd);
			switch(cmd.type){
				case CCMD_connect_spectate:
					
					if(cmd.ccmd_connect.protocol!=PROTOCOL_VERSION){
						// invalid version; deny the client
						cmd.type=SCMD_deny;
						cmd.index=(cmdIndex++);
						CMD_sendCommand(sock, cmd, addr);
						break;
					}
					
					// find a client with the same addr.
					for(n=0;n<MAX_CLIENTS;n++){
						if(cli[n].addr==addr && cli[n].spectate)
							break;
					}
					if(n!=MAX_CLIENTS){
						// already connected
						cmd.type=SCMD_initial;
						strcpy(cmd.scmd_initial.mapname, mapname);
						cmd.scmd_initial.yourClient=n;
						cmd.index=(cmdIndex++);
						CMD_sendCommand(sock, cmd, addr);
						break;
					}
					for(n=0;n<MAX_CLIENTS;n++){
						if(cli[n].addr==addr)
							cli[n].enable=false;
					}
					
					// find a new slot.
					n=C_spawn(cmd.ccmd_connect.model, new ctrl_t(), true);
					
					if(n==-1){
						// no free slot; deny the client
						cmd.type=SCMD_deny;
						cmd.index=(cmdIndex++);
						CMD_sendCommand(sock, cmd, addr);
						break;
					}
					
					
					cli[n].addr=addr;
					strcpy(cli[n].name, cmd.ccmd_connect.name);
					cli[n].oresp=SDL_GetTicks();
					cli[n].ready=false;
					
					// allow the client and spawn
					
					cmd.type=SCMD_initial;
					strcpy(cmd.scmd_initial.mapname, mapname);
					cmd.scmd_initial.yourClient=n;
					cmd.index=(cmdIndex++);
					CMD_sendCommand(sock, cmd, addr);
					
					sprintf(buf, "Client %s connected", cli[n].name);
					SV_msg(buf);
					
					break;
				case CCMD_connect:
					
					if(cmd.ccmd_connect.protocol!=PROTOCOL_VERSION){
						// invalid version; deny the client
						cmd.type=SCMD_deny;
						cmd.index=(cmdIndex++);
						CMD_sendCommand(sock, cmd, addr);
						break;
					}
					
					// find a client with the same addr.
					for(n=0;n<MAX_CLIENTS;n++){
						if(cli[n].addr==addr && cli[n].enable)
							break;
					}
					if(n!=MAX_CLIENTS){
						// already connected
						cmd.type=SCMD_initial;
						strcpy(cmd.scmd_initial.mapname, mapname);
						cmd.scmd_initial.yourClient=n;
						cmd.index=(cmdIndex++);
						CMD_sendCommand(sock, cmd, addr);
						break;
					}
					// is spectating?
					for(n=0;n<MAX_CLIENTS;n++){
						if(cli[n].addr==addr && cli[n].spectate){
							cli[n].enable=false;
							n=MAX_CLIENTS-1;
						}
					}
					
					// find a new slot.
					n=C_spawn(cmd.ccmd_connect.model, new ctrl_t(), false);
					
					if(n==-1){
						// no free slot; deny the client
						cmd.type=SCMD_deny;
						cmd.index=(cmdIndex++);
						CMD_sendCommand(sock, cmd, addr);
						break;
					}
					
					
					cli[n].addr=addr;
					strcpy(cli[n].name, cmd.ccmd_connect.name);
					cli[n].oresp=SDL_GetTicks();
					cli[n].ready=false;
					
					// allow the client and spawn
					
					cmd.type=SCMD_initial;
					strcpy(cmd.scmd_initial.mapname, mapname);
					cmd.scmd_initial.yourClient=n;
					cmd.index=(cmdIndex++);
					CMD_sendCommand(sock, cmd, addr);
					
					sprintf(buf, "Client %s connected", cli[n].name);
					SV_msg(buf);
					
					
					break;
				case CCMD_ready:
					// find the client
					n=findClient(addr);
					if(n==-1){
						//invalid client;
						break;
					}
					cli[n].ready=true;
					cli[n].oresp=SDL_GetTicks();
					sprintf(buf, "Client %s entered the game", cli[n].name);
					SV_msg(buf);
					break;
				case CCMD_enter:
					// find the client
					n=findClient(addr);
					if(n==-1){
						//invalid client;
						break;
					}
					if(cli[n].spectate){
						sprintf(buf, "Client %s entered the game", cli[n].name);
						SV_msg(buf);
						cli[n].spectate=false;
					}
					break;
				case CCMD_leave:
					// find the client
					n=findClient(addr);
					if(n==-1){
						//invalid client;
						break;
					}
					if(!cli[n].spectate){
						sprintf(buf, "Client %s left the game", cli[n].name);
						SV_msg(buf);
						cli[n].spectate=true;
					}
					break;
				case CCMD_control:
					// find the client
					n=findClient(addr);
					if(n==-1){
						//invalid client;
						break;
					}
					cli[n].oresp=SDL_GetTicks();
					ctrl_t *ctrl;
					ctrl=static_cast<ctrl_t *>(cli[n].ctrl);
					ctrl->steer=cmd.ccmd_control.steer;
					ctrl->accel=cmd.ccmd_control.accel;
					ctrl->str=cmd.ccmd_control.strafe;
					ctrl->vert=0;
					ctrl->firing=(cmd.ccmd_control.sw&CCCSW_fire);
					ctrl->pitch=cmd.ccmd_control.pitch;
					if(cmd.ccmd_control.sw&CCCSW_raise)
						ctrl->vert=1;
					if(cmd.ccmd_control.sw&CCCSW_lower)
						ctrl->vert=-1;
					break;
				case CCMD_arrive:
					for(set<cmd_history_t>::iterator it=history.begin(), it2;it!=history.end();){
						if(it->cmd.index==cmd.ccmd_arrive.cmdindex){
							// arrived! remove from list.
							it2=it;
							it++;
							history.erase(it2);
						}else{
							it++;
						}
					}
					break;
			}
		}
		manageHistory();
	}catch(const char * str){
		consoleLog("ServerFrame: error occured: \"%s\"\n", str);
	}catch(...){
		consoleLog("ServerFrame: error occured\n");
	}
}

static Uint32 ofps;
static int fps=0;
static float fps2=100.f;

Uint32 ServerTimer(Uint32 interval, void *){
	fps++;
	if(SDL_GetTicks()>ofps+250){
		fps2=(float)fps/(float)(SDL_GetTicks()-ofps)*1000.f;
		fps=0;
		ofps=SDL_GetTicks();
		sv_fps=fps2;
	}
	
	ServerFrame(min(1.f/fps2,.1f));
	return interval;
}

static void resetStat(){
	sv_statCmdsSent=0;
	sv_statCmdsRecv=0;
	sv_statLostCmds=0;
	sv_statBytesSent=0;
	sv_statBytesRecv=0;
}

static int ServerThread(void *){
	int n;
	
	char buf[256];
	
	ofps=SDL_GetTicks();
	
	resetStat();
	
	consoleLog("ServerThread: adding AIs\n");
	const char *names[]={
		"member65", "cyborg",
		"kengo", "gotouKein",
		"lolicon", "hentai",
		"chu-2byou"
	};
	/*const char *names[]={
		"This picture is", "taken from my game",
		"BloodKart !", "Check it out at",
		"http://tcppjp.ddo.jp/", "--",
		"Copyright 2009 tcpp"
	};*/
	for(int n=0;n<7;n++){
		ai_t *ai=new ai_t();
		if((n&1)==0)
			ai->mindist*=2.f;
		client_t& c=cli[C_spawn("default",ai)];
		//c.max_speed*=(14.f-(float)(7-n))*.1f;
		c.power*=1.5f;
		//c.score=10000-n*1000;
		sprintf(c.name, "bkAI #%d", n+1);
		strcpy(c.name, names[n]);
	}
	fps2=100.f;
	if(!sv_sdlTimer){
		// busyloop implement
		consoleLog("ServerThread: starting server loop\n");
		while(sock){
			
			fps++;
			if(SDL_GetTicks()>ofps+250){
				fps2=(float)fps/(float)(SDL_GetTicks()-ofps)*1000.f;
				fps=0;
				ofps=SDL_GetTicks();
				//printf("serverFps: %f\n", fps2);
				sv_fps=fps2;
			}
			
			ServerFrame(min(1.f/fps2,.1f));
		}
	}else{
		consoleLog("ServerThread: starting server timer\n");
		SDL_TimerID timer=SDL_AddTimer(20, ServerTimer, NULL);
		while(sock){
			SDL_Delay(100);
		}
		SDL_RemoveTimer(timer);
		SDL_Delay(100); // wait for exit
	}
	return 0;
}

void SV_start(const char *mn, int port){
	strcpy(mapname, mn);
	
	consoleLog("SV_start: initializing environment\n");
	
	mp=new map_t(mn);
	W_clear();
	
	// bench
	/*
	Uint32 ot;int cnt;
	aabb_t bnd=aabb_t(mp->m->vertex, mp->m->count_vertex);
	
	ot=SDL_GetTicks(); cnt=0;
	while(SDL_GetTicks()<ot+1000){
		vec3_t v1, v2;
		v1=(bnd.minvec+bnd.maxvec)*.5f;
		v2=vec3_t(rnd(), rnd(), rnd()).normalize();
		v1+=v2*100000.f;
		mp->m->raycast(v1, v2);
		cnt++;
	}
	printf("raycast performance: %d op/s\n", cnt);
	*/
	
	consoleLog("SV_start: opening UDP port\n");
	sv_running=true;
	sock=SDLNet_UDP_Open(port);
	
	if(sock){
	
		consoleLog("SV_start: starting server thread\n");
		
		th=SDL_CreateThread(ServerThread, NULL);
	
	}else{
		
		consoleLog("SV_start: couldn't open UDP port\n");
		
		throw "SV_start: failed to open UDP port";
		
	}
	
}

void SV_stop(){
	UDPsocket s=sock;
	history.clear();
	if(s){
		sock=NULL;
		SDL_Delay(250);
		SDLNet_UDP_Close(s);
		consoleLog("SV_stop: closed UDP port\n");
		SV_logStat();
	}
	
}

void SV_logStat(){
	consoleLog("SV_logStat: **** server stat ****\n");
	consoleLog("SV_logStat: sv_statCmdsSent: %d\n", sv_statCmdsSent);
	consoleLog("SV_logStat: sv_statCmdsRecv: %d\n", sv_statCmdsRecv);
	consoleLog("SV_logStat: sv_statLostCmds: %d\n", sv_statLostCmds);
	consoleLog("SV_logStat: sv_statBytesSent: %d\n", sv_statBytesSent);
	consoleLog("SV_logStat: sv_statBytesRecv: %d\n", sv_statBytesRecv);
}
void SV_msgStat(){
	SV_msg("SV_logStat: sv_statBytesRecv: %d", sv_statBytesRecv);
	SV_msg("SV_logStat: sv_statBytesSent: %d", sv_statBytesSent);
	SV_msg("SV_logStat: sv_statLostCmds: %d", sv_statLostCmds);
	SV_msg("SV_logStat: sv_statCmdsRecv: %d", sv_statCmdsRecv);
	SV_msg("SV_logStat: sv_statCmdsSent: %d", sv_statCmdsSent);
	SV_msg("SV_logStat: **** server stat ****");
	
}

void SV_sound(const char *name, vec3_t pos, float factor, int speed){
	cmd_t cmd;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.type=SCMD_sound;
	strcpy(cmd.scmd_sound.name, name);
	cmd.scmd_sound.is3d=true;
	cmd.scmd_sound.pos=cvec3(pos);
	cmd.scmd_sound.factor=factor;
	cmd.scmd_sound.speed=speed;
	broadcastCmd(cmd);
}
void SV_sound(const char *name, float factor, int speed){
	cmd_t cmd;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.type=SCMD_sound;
	strcpy(cmd.scmd_sound.name, name);
	cmd.scmd_sound.is3d=false;
	cmd.scmd_sound.factor=factor;
	cmd.scmd_sound.speed=speed;
	broadcastCmd(cmd);
}
void SV_blood(vec3_t v){
	cmd_t cmd;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.type=SCMD_blood;
	cmd.scmd_blood.pos=cvec3(v);
	broadcastCmd(cmd);
}
void SV_bloodBig(vec3_t v){
	cmd_t cmd;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.type=SCMD_blood_big;
	cmd.scmd_blood_big.pos=cvec3(v);
	broadcastCmd(cmd);
}
void SV_explode(vec3_t v){
	cmd_t cmd;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.type=SCMD_explode;
	cmd.scmd_blood.pos=cvec3(v);
	broadcastCmd(cmd);
}
void SV_explodeBig(vec3_t v){
	cmd_t cmd;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.type=SCMD_explode_big;
	cmd.scmd_blood.pos=cvec3(v);
	broadcastCmd(cmd);
}
void SV_bulletImpact(vec3_t pos, vec3_t proj){
	cmd_t cmd;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.type=SCMD_bullet_impact;
	cmd.scmd_bullet_impact.pos=cvec3(pos);
	cmd.scmd_bullet_impact.proj=cvec3(proj);
	broadcastCmd(cmd);
}
void SV_screenKick(float v){
	cmd_t cmd;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.type=SCMD_screen_kick;
	cmd.scmd_screen_kick.amount=v;
	broadcastCmd(cmd);
}
void SV_msg(const char *m, ...){
	char buf[1024];
	va_list v;
	va_start(v, m);
	
	cmd_t cmd;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.type=SCMD_msg;
	vsprintf(cmd.scmd_msg.msg, m, v);
	broadcastCmd(cmd);
	va_end(v);
}
void SV_kick(int i){
	client_t& c=cli[i];
	cmd_t cmd;
	
	if(!c.enable)
		return;
	
	c.explode();
	c.enable=false;
	c.spectate=true;
	
	
	cmd.magic=PROTOCOL_MAGIC;
	cmd.type=SCMD_kick;
	cmd.index=(cmdIndex++);
	broadcastCmd(cmd, i);
	
	char buf[256];
	sprintf(buf, "Client %s was kicked", c.name);
	SV_msg(buf);
}
void SV_fired(int cl){
	cmd_t cmd;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.type=SCMD_fired;
	cmd.scmd_fired.cli=cl;
	broadcastCmd(cmd);
}
void SV_hit(int cli, int target){
	cmd_t cmd;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.type=SCMD_hit;
	cmd.scmd_fired.cli=target;
	broadcastCmd(cmd, cli);
}
void SV_hurt(int client, float amount, vec3_t from){
	cmd_t cmd;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.type=SCMD_hurt;
	cmd.scmd_hurt.from=cvec3(from);
	cmd.scmd_hurt.amount=amount;
	broadcastCmd(cmd, client);
}
void SV_fraggedMsg(int cli, const char *m, ...){
	char buf[1024];
	va_list v;
	va_start(v, m);
	
	cmd_t cmd;
	cmd.magic=PROTOCOL_MAGIC;
	cmd.type=SCMD_fragged_msg;
	vsprintf(cmd.scmd_msg.msg, m, v);
	broadcastCmd(cmd, cli);
	va_end(v);
}


