/*
 *  client.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/23.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "client.h"
#include "map.h"
#include "sfx.h"
#include "server.h"
#include "weapon.h"
#include "clientgame.h"

client_t cli[MAX_CLIENTS];

static Uint32 ohitsnd;
float c_maxHealth=100.f;
float c_regenTime=5.f;
float c_availTime=1.5f;
float c_respawnTime=4.f;
float c_invincibleTime=2.5f;
float c_spawnTime=.5f;
float c_slotTime=1.f;
float c_speedScaleInitial=1.3f;
float c_speedScaleMin=.8f;
float c_speedScaleMax=1.4f;
float c_speedScaleChange=0.3f/30.f; // per second
float c_repeatSuicideTime=10.f;

void C_init(){
	int n;
	for(n=0;n<MAX_CLIENTS;n++)
		cli[n].enable=cli[n].spectate=false;

	ohitsnd=SDL_GetTicks();
	consoleLog("C_init: all clients removed\n");
}
int C_count(){
	int n, i=0;
	for(n=0;n<MAX_CLIENTS;n++)
		if(!cli[n].spectate)
			if(cli[n].enable)
				i++;
	return i;
}
void C_framenext(float dt){
	int n, i;
	for(n=0;n<MAX_CLIENTS;n++){
		client_t& c=cli[n];
		if(c.spectate){
			
			float ac;
			float steer;
			float speed;
			
			if(spectate_controller_t *sctrl=dynamic_cast<spectate_controller_t *>(c.ctrl)){
				
				sctrl->framenext(dt);
				
				c.mang=vec3_t(0,0,0);
				
				c.mang.y=sctrl->get_steering()*4.f;
				c.mang.x=sctrl->get_pitch()*2.f;
				if(c.ang.x+c.mang.x>M_PI*.5f)
					c.mang.x=M_PI*.5f-c.ang.x;
				if(c.ang.x+c.mang.x<-M_PI*.5f)
					c.mang.x=-M_PI*.5f-c.ang.x;
				c.ang+=c.mang*dt;
				
				
				c.vel=rotate(vec3_t(0.f, 0.f, 1.f),c.ang)*15.f*(float)sctrl->get_accel();
				c.vel+=rotate(vec3_t(1.f, 0.f, 0.f),c.ang)*15.f*(float)sctrl->get_strafe();
				c.vel.y+=sctrl->get_vertical()*15.f;
				
				c.vel*=.5f;
				c.mang*=.5f;
				
				c.pos+=c.vel*dt;
				
			}else{
			
				c.ctrl->framenext(dt);
				ac=c.ctrl->get_accel();
				steer=c.ctrl->get_steering();
				
				c.ang.y+=steer*dt;
				if(c.ctrl->fire())
					c.pos+=rotate(vec3_t(0.f, 0.f, 1.f),c.ang)*dt*5.f;
				c.pos.y+=ac*3.f*dt;
			
			}
			
			c.alive_time+=dt;
			for(int step=0;step<2;step++){
				// map hit tset
				
				vector<isect_t> *hits=mp->m->isect(c.pos+vec3_t(0.f, .8f, 0.f), 1.f);
				
				float mind; int mini;
				mini=-1;
				
				if(hits->size()){
					
					mini=-1;
					mind=0.f;
					
					vector<isect_t>::iterator it=hits->begin();
					for(;it!=hits->end();it++){
						if(it->dist<mind || mini==-1){
							plane_t plane(it->v1, it->v2, it->v3);
							it->dist=plane.distance(c.pos+vec3_t(0.f, .8f, 0.f));
							if(it->dist<-1.f || it->dist>1.f)
								continue;
							
							if(it->dist<0.f)
								continue;
							mini=it-hits->begin();
							mind=it->dist;
							
						}
					}
					
					if(mini!=-1){
						
						isect_t hit=(*hits)[mini];
						plane_t plane(hit.v1, hit.v2, hit.v3);
						
						float d=plane.distance(c.pos+vec3_t(0.f, .8f, 0.f));
						
						d=1.f-d;
						vec3_t nrm, tmp;
						float impact;
						c.ctrl->hit(plane.n);
						
						nrm=plane.n*(d+.1f);
						c.pos+=nrm;
					}
					
					
				}
			}	
			
		}else if(c.enable){
			vec3_t lvel;
			int ac;
			float steer;
			float speed;
		
			c.ctrl->framenext(dt);
			ac=c.ctrl->get_accel();
			steer=c.ctrl->get_steering();
			
			c.mang=vec3_t(0,0,0);
			
			// hittests
			
			if(c.is_avail()){
			
				if(c.is_dead()){
					// lost control...
					steer=4.f;
					ac=1.f;
				}
				
				float dt2;
				int steps=1;
				int step;
				dt2=dt;
				speed=c.vel.distance();
				while(dt2*speed>.5f){
					dt2*=.5f;
					steps<<=1;
				}
				
				float gnd=0.f;
				bool onGround=false;
				
				for(step=0;step<steps;step++){
				
					c.pos+=c.vel*vec3_t(dt2);
					c.vel.y-=12.f*dt2;
					
					// ground hit test
					
					vec3_t hit;
					float dist;
					
					if(c.pos.y<mp->crashLevel)
						c.instantKill(BY_MAP);
					
					hit=mp->findfloor(vec3_t(c.pos.x, c.pos.y+.6f, c.pos.z), i);
					if(c.pos.y+c.vel.y*dt2-.15f<hit.y){
						if(mp->isCritical[mp->m->face[(i<<2)+3]])
							c.instantKill(BY_MAP);
						gnd+=1.f;
						vec3_t opos, ovel;
						opos=c.pos;
						ovel=c.vel;
						onGround=true;
						
						c.pos.y=hit.y;
						/*if(c.vel.y<0.f)
							c.vel.y=0.f;*/
						
						
						vec3_t hit1, hit2, hit3;
						float cx, cy;
						
						dist=.5f;
						do{
							if(dist<.01f){
								c.pos=opos;
								c.vel=ovel;
								goto ground_fall;
							}
							hit1=mp->findfloor(vec3_t(c.pos.x, c.pos.y+.6f, c.pos.z)+
											   rotate(vec3_t(0.f, 0.f, -dist), c.ang));
							hit2=mp->findfloor(vec3_t(c.pos.x, c.pos.y+.6f, c.pos.z)+
											   rotate(vec3_t(0.f, 0.f, dist), c.ang));
							dist*=.5f;
						}while(hit1.y<hit.y-.5f || hit2.y<hit.y-.5f);
						
						hit3=hit2;
						cx=hit2.x-hit1.x; cy=hit2.z-hit1.z;
						cx=sqrtf(cx*cx+cy*cy);
						cy=hit2.y-hit1.y;
						
						c.ang.x=atan2(cy, cx);
						if(c.ang.x>M_PI)
							c.ang.x=M_PI*2.f-c.ang.x;
						
						do{
							if(dist<.01f){
								c.pos=opos;
								c.vel=ovel;
								goto ground_fall;
							}
							hit1=mp->findfloor(vec3_t(c.pos.x, c.pos.y+.6f, c.pos.z)+
											   rotate(vec3_t(-dist, 0.f, 0.f), c.ang));
							hit2=mp->findfloor(vec3_t(c.pos.x, c.pos.y+.6f, c.pos.z)+
											   rotate(vec3_t(dist, 0.f, 0.f), c.ang));
							dist*=.5f;
						}while(hit1.y<hit.y-.5f || hit2.y<hit.y-.5f);
						
						plane_t plane(hit1, hit2, hit3);
						c.vel-=plane.n*vec3_t::dot(plane.n, c.vel);
						
						cx=hit2.x-hit1.x; cy=hit2.z-hit1.z;
						cx=sqrtf(cx*cx+cy*cy);
						cy=hit2.y-hit1.y;
						
						c.ang.z=-atan2(cy, cx);
						if(c.ang.x>M_PI)
							c.ang.x=M_PI*2.f-c.ang.x;
						
					}
					
					
				ground_fall:;
					
					// map hit tset
					
					vector<isect_t> *hits=mp->m->isect(c.pos+vec3_t(0.f, .8f, 0.f), .5f);
					
					float mind; int mini;
					mini=-1;
					
					if(hits->size()){
					
						mini=-1;
						mind=0.f;
						
						vector<isect_t>::iterator it=hits->begin();
						for(;it!=hits->end();it++){
							if(it->dist<mind || mini==-1){
								plane_t plane(it->v1, it->v2, it->v3);
								it->dist=plane.distance(c.pos);
								if(it->dist<-.5f || it->dist>.5f)
									continue;
								if(plane.n.y>.4f)
									continue;
								if(it->dist<0.f)
									continue;
								if(mp->isCritical[mp->m->face[(it->face<<2)+3]])
									c.instantKill(BY_MAP);
								mini=it-hits->begin();
								mind=it->dist;
								
							}
						}
						/*
						for(it=hits->begin();it!=hits->end();it++){
							mp->m->face[(it->face<<2)+3]=rand()%(mp->m->count_material);
						}
						for(it=hits->begin();it!=hits->end();it++){
							printf("%d: {%f, %f, %f} %f\n", it-hits->begin(),
								   it->v1.x, it->v1.y, it->v1.z, it->dist);
						}*/
						
						if(mini!=-1){
						
							isect_t hit=(*hits)[mini];
							plane_t plane(hit.v1, hit.v2, hit.v3);
							
							float d=plane.distance(c.pos);
							//assert(d<.5f);
							//assert(d>-.5f);
							/*printf("%d: {%f, %f, %f} {%f, %f, %f} %f\n",
								   mini, hit.v1.x, hit.v1.y, hit.v1.z, 
								   plane.n.x, plane.n.y, plane.n.z,
								   hit.dist);
							mp->m->face[(hit.face<<2)+3]=rand()%(mp->m->count_material);*/
							
							d=.5f-d;
							vec3_t nrm, tmp;
							float impact;
							c.ctrl->hit(plane.n);
							
							nrm=plane.n*(d+.1f);
							c.pos+=nrm;
							impact=fabs(vec3_t::dot(plane.n, c.vel));
							c.vel-=plane.n*vec3_t::dot(plane.n, c.vel)*1.5f;
							
							float dmgScale;
							// c
							if(fabs(vec3_t::dot(rotate(vec3_t(0,0,1), c.ang), plane.n))>.8f){
								// front/rear
								dmgScale=3.f;
							}else{
								// side
								dmgScale=6.f;
							}
							if(impact>6.f){
								c.damage((impact-6.f)*dmgScale, c.pos-nrm, -1);
							}
							
							if(impact>.8f){
								if(rand()%2)
									SV_sound("hit2", c.pos, impact);
								else
									SV_sound("hit4", c.pos, impact);
								ohitsnd=SDL_GetTicks();
							}else if(impact>.5f){
								if(rand()%2)
									SV_sound("hit2", c.pos, impact);
								else
									SV_sound("hit1", c.pos, impact);
								ohitsnd=SDL_GetTicks();
							}
						}
						
						
					}
					
					delete hits;
				
				
					//client hit tset
					
					for(i=0;i<n;i++){
						client_t& c2=cli[i];
						if(!c2.enable)
							continue;
						if(!c2.is_avail())
							continue;
						vec3_t dif=c.pos-c2.pos;
						if(dif.length_2()<.64f){
							vec3_t vdif=c.vel-c2.vel;
							vec3_t dir=dif.normalize();
							float impact;
							impact=fabs(vec3_t::dot(vdif, dir));
							c.ctrl->clientHit(i, c2.pos);
							c2.ctrl->clientHit(n, c.pos);
							c.vel-=dir*vec3_t::dot(vdif, dir)*1.f;
							c2.vel+=dir*vec3_t::dot(vdif, dir)*1.f;
							vec3_t cen=c.pos+(c2.pos-c.pos)*.5f;
							c.pos=cen+dir*.4f;
							c2.pos=cen-dir*.4f;
							float dmgScale;
							// c
							if(fabs(vec3_t::dot(rotate(vec3_t(0,0,1), c.ang), -dir))>.8f){
								// front/rear
								dmgScale=3.f;
							}else{
								// side
								dmgScale=6.f;
							}
							if(impact>6.f){
								c.damage((impact-6.f)*dmgScale, c2.pos, c2.getID()|BYF_COLISION);
							}
							
							if(fabs(vec3_t::dot(rotate(vec3_t(0,0,1), c2.ang), dir))>.8f){
								// front/rear
								dmgScale=3.f;
							}else{
								// side
								dmgScale=6.f;
							}
							if(impact>6.f){
								c2.damage((impact-6.f)*dmgScale, c.pos, c.getID()|BYF_COLISION);
							}
							
							if(impact>.8f){
								if(rand()%2)
									SV_sound("hit3", c.pos, impact*1.5f);
								else
									SV_sound("hit5", c.pos, impact*1.5f);
								ohitsnd=SDL_GetTicks();
							}else if(impact>.5f){
								if(rand()%2)
									SV_sound("hit3", c.pos, impact*1.5f);
								else
									SV_sound("hit1", c.pos, impact*1.5f);
								ohitsnd=SDL_GetTicks();
							}
						}
					}
						
					// item hit test
					
					if(c.weapon[0]==0){
						for(i=0;i<mp->items;i++){
							if(!mp->isItemAvailable(i))
								continue; // not avilable
							vec3_t dif=mp->item[i]-c.pos;
							if(dif.length_2()<1.8f){
								
								// gotcha!
								c.weapon_wait=c_slotTime;
								// now choose a weapon...
								weapon_t *w=W_choose(C_getRank(n));
								// setup weapon
								c.ammo=w->defaultAmmo;
								strcpy(c.weapon, w->name);
								// remove item
								mp->itemspawn[i]=-m_itemSpawnTime;
								SV_explode(mp->item[i]);
								// play sound
								SV_sound("item", c.pos, 5.f);
								
								break;
							}
						}
					}
					
					
				}
				gnd/=(float)steps;
				
				// use local coordinate system
				
				lvel=unrotate(c.vel, c.ang);
				
				// dynamics of accelling
				
				float curMaxSpeed=c.max_speed*c.speedScale;
				
				if(c.slip>.8f)
				lvel.x*=powf(c.grip, dt*gnd*.2f);
				else
				lvel.x*=powf(c.grip, dt*gnd);
				if(ac==-1 && lvel.z>curMaxSpeed*.2f && false)
					lvel.z*=powf(c.brake_damp, dt*gnd);
				else if(ac==-1 && lvel.z>-curMaxSpeed*.5f)
					lvel.z-=c.power*gnd*dt;
				else if(ac==1 && lvel.z<curMaxSpeed*.5f)
					lvel.z+=c.power*gnd*dt;
				else if(ac==1 && lvel.z<curMaxSpeed*.75f)
					lvel.z+=c.power*gnd*dt*.6f;
				else if(ac==1 && lvel.z<curMaxSpeed*.9f)
					lvel.z+=c.power*gnd*dt*.4f;
				else if(ac==1 && lvel.z<curMaxSpeed)
					lvel.z+=c.power*gnd*dt*.2f;
				if(ac==1)
					lvel.x*=powf(0.1f, dt*gnd);
				lvel.z*=powf(c.damp, dt*gnd);
				speed=lvel.z;
				float steerscale;
				if(ac==1)
					steerscale=.5f;
				else if(ac==0)
					steerscale=1.f;
				else if(ac==-1)
					steerscale=2.f;
				vec3_t rang=c.ang;
				c.view_steer+=(steer-c.view_steer)*(1.f-powf(.04f, dt));
				if(onGround){
					c.mang.y=speed*c.view_steer*c.steer*steerscale;
					c.ang.y+=speed*c.view_steer*c.steer*dt*steerscale;
				
					if(c.slip>.8f)
						rang.y+=speed*c.view_steer*c.steer*dt*steerscale*.1f;
					else
						rang.y+=speed*c.view_steer*c.steer*dt*steerscale;
				}
				c.now_speed=lvel.z;
				
				// back to world coordinate system, when on ground
				
				if(onGround)
					c.vel=rotate(lvel, rang);
				
				// slip
				
				if(c.slip>0.f)
					c.slip=max(0.f, c.slip-dt*1.4f);
				else
					c.slip=min(0.f, c.slip+dt*1.4f);
				c.slip+=(steer*speed/curMaxSpeed*dt*3.f*c.slipfactor);
				if(c.slip<-1.f)
					c.slip=-1.f;
				if(c.slip>1.f)
					c.slip=1.f;
				
				if(c.is_dead())
					c.slip=1.f;
				
				// recording location history for respawning
				
				c.hist_delay-=dt;
				if(c.hist_delay<0.f){
					c.hist_delay=.1f;
					if(mp->getProgress(c.pos+vec3_t(0.f, .3f, 0.f))>-.5f && !c.is_dead()){
						// enroute, record history
						c.hist[c.hist_pos]=c.pos;
						c.hist_ang[c.hist_pos]=c.ang;
						c.hist_pos=(c.hist_pos+1)%MAX_HIST;
					}
				}
				
			}else{
					
				// no body, no phystics.
				
				c.slip=0.f;
				c.vel=vec3_t(0.f, 0.f, 0.f);
				
			}
			
			// changing of speed scale
			
			float newScale;
			// calculate new scale
			newScale=c_speedScaleMin;
			newScale+=(float)(C_getRank(n)+1)/(float)(C_count()+1)
				*(c_speedScaleMax-c_speedScaleMin);
			if(newScale>c.speedScale){
				c.speedScale+=c_speedScaleChange*dt;
				if(c.speedScale>newScale)
					c.speedScale=newScale;
			}else{
				c.speedScale-=c_speedScaleChange*dt;
				if(c.speedScale<newScale)
					c.speedScale=newScale;
			}
			
			// weapon
			
			bool firing, fireReady;
			c.weapon_delay-=dt;
			fireReady=(c.weapon_wait<=0.f);
			c.weapon_wait-=dt;
			firing=c.ctrl->fire();
			if(c.is_dead() && c.is_avail())
				firing=true;  // forcely fire!
			if(c.has_weapon()){
				if(c.weapon_wait<=0.f && (!fireReady)){
					// ready sound
					SV_sound("fireready", c.pos, 6.f);
				}
			}
			if(firing){
				if(c.has_weapon() && c.is_avail()){
					weapon_t *wp=W_find(c.weapon);
					if(c.ammo){
						float fireDelay=0.f;
						while(c.weapon_delay<0.f && (wp->interval>.000001f || !c.ofire)){
							// can fire!
							
							// TODO: fire
							if(wp->issound[0] || wp->issound[1] || wp->issound[2] || wp->issound[3]){
								int i;
								do{
									i=(rand()>>8)&3;
								}while(!wp->issound[i]);
								SV_sound(wp->sound[i], c.pos, wp->sndscale[i]);
							}
							W_fire(n, fireDelay);
							c.weapon_delay=max(c.weapon_delay, -dt);
							c.weapon_delay+=wp->interval;
							fireDelay+=wp->interval;
							c.ammo--;
							c.ofire=firing;
						}
					}else if(!c.ofire){
						// throw away
						
						// TODO: throw the weapon
						
						c.weapon[0]=0;
					}
				}else if(!c.ofire){
					// melee ?
				}
			}
			c.ofire=firing;
			
			if(c.is_dead()){
				// dead player
				
				bool oavail=c.is_avail();
				
				c.dead_time+=dt;
				c.alive_time=0.f;
				
				if(oavail && !c.is_avail()){
					// blow away! body disappears.
					// maybe some gibs are given...
					c.explode();
				}
				
				
				
				if(c.dead_time>c_respawnTime){
					// respawn.
					c.respawn();
				}
			}else{
				// alive
				c.dead_time=0.f;
				c.alive_time+=dt;
			}
			c.suicideTime-=dt;
			/*
			TMix_ChannelEx ex=S_calc(c.pos, .7f*fabs(c.slip));
			ex.looped=true;
			if(fabs(c.slip)>.8f && c.slipsnd==-1){
				c.slipsnd=TMix_PlayChannelEx(-1, snd_slip, ex);
			}else if(fabs(c.slip)<.1f && c.slipsnd!=-1){
				TMix_StopChannel(c.slipsnd);
				c.slipsnd=-1;
			}else if(c.slipsnd!=-1){
				TMix_UpdateChannelEx(c.slipsnd, ex);
			}*/
		}
	}
}

void C_cframenext(float dt){
	int n;
	for(n=0;n<MAX_CLIENTS;n++){
		client_t& c=cli[n];
		if(!c.enable)
			continue;
		TMix_ChannelEx ex=S_calc(c.pos, .7f*fabs(c.slip));
		ex.looped=true;
		if(fabs(c.slip)>.8f && c.slipsnd==-1){
			c.slipsnd=TMix_PlayChannelEx(-1, snd_slip, ex);
		}else if(fabs(c.slip)<.1f && c.slipsnd!=-1){
			TMix_StopChannel(c.slipsnd);
			c.slipsnd=-1;
		}else if(c.slipsnd!=-1){
			TMix_UpdateChannelEx(c.slipsnd, ex);
		}
		
		// weapon anim
		
		weapon_t *w=W_find(c.weapon);
		if(w && c.has_weapon()){
		
			int opos=(int)c.weapFrame;
			float aa, bb;
			c.weapFrame+=dt*60.f;
			
			if(opos<w->getFrames+w->idleFrames){
				if(n==yourClient){
					//consoleLog("idle %f (%d<=%d)\n", c.weapFrame, opos, w->getFrames+w->idleFrames);
				}
				// idle/get
				aa=(float)w->getFrames;
				bb=(float)(w->getFrames+w->idleFrames);
				if(c.weapFrame>=bb-1.f){
					if(n==yourClient){
						//consoleLog("fireend %f>%f\n", c.weapFrame, bb);
					}
					c.weapFrame-=bb-1.f-aa; // repeat
				}
			}else{
				if(n==yourClient){
					//consoleLog("fire %f\n", c.weapFrame);
				}
				// firing
				bb=(float)(w->getFrames+w->idleFrames+w->fireFrames);
				if(c.weapFrame>=bb){
					if(n==yourClient){
						//consoleLog("fireend %f>%f\n", c.weapFrame, bb);
					}
					c.weapFrame=(float)w->getFrames; // back to idle
				}
			}
			
			
		}
	}
}
int C_spawn(const char *mn, controller_t *ctrl, bool spectate){
	int n;
	if(spectate){
		for(n=MAX_CLIENTS-1;n>=0;n--){
			client_t& c=cli[n];
			if(c.enable || c.spectate)
				continue;
			c.pos=mp->spawns[0];
			c.pos=mp->findfloor(c.pos);
			c.vel=vec3_t();
			c.ang=vec3_t(0.f, mp->angles[0]*M_PI/180.f, 0.f);
			c.ctrl=ctrl;
			c.ctrl->bind(&c);
			c.enable=false;
			c.spectate=true;
			c.alive_time=0.f;
			for(int i=0;i<MAX_HIST;i++)
				c.hist[n]=c.pos;
			c.hist_delay=0.f;
			strcpy(c.model, "spectate");
			
			//c.ammo=10000;
			//strcpy(c.weapon, "mish");
			//c.weapon_delay=0.f;
			return n;
		}
	}else{
		for(n=0;n<MAX_CLIENTS;n++){
			client_t& c=cli[n];
			if(c.enable || c.spectate)
				continue;
			if(!mp->spawnable[n]){
				printf("can't spawn as #%d\n", n);
				return -1;
			}
			c.pos=mp->spawns[n];
			c.pos=mp->findfloor(c.pos);
			c.vel=vec3_t();
			c.ang=vec3_t(0.f, mp->angles[n]*M_PI/180.f, 0.f);
			c.ctrl=ctrl;
			c.ctrl->bind(&c);
			c.enable=true;
			C_setup(n);
			c.slipsnd=-1;
			c.slip=0.f;
			c.weapon[0]=0;
			c.ammo=0;
			c.health=c_maxHealth;
			c.regen_delay=0.f;
			c.dead_time=0.f;
			c.weapon_wait=0.f;
			c.ofire=false;
			c.hist_pos=0;
			c.score=0;
			c.view_steer=0.f;
			c.alive_time=0.f;
			c.speedScale=c_speedScaleInitial;
			c.suicideTime=c_repeatSuicideTime;
			for(int i=0;i<MAX_HIST;i++)
				c.hist[n]=c.pos;
			c.hist_delay=0.f;
			strcpy(c.model, mn);
			
			c.ammo=1000000;
			strcpy(c.weapon, "");
			c.weapon_delay=0.f;
			return n;
		}
	}
	return -1;
}
void C_setup(int n){
	
	
	
	client_t& c=cli[n];
	c.slipfactor=1.f;
	c.grip=.03f;
	c.max_speed=10.f;
	c.brake_damp=.5f;
	c.power=5.f;
	c.damp=.95f;
	c.steer=.25f;
	
	consoleLog("C_setup: initialized parameter of %s\n", c.model);
	
}

bool client_t::has_weapon(){

	if(weapon_wait>0.f)
		return false;
	return weapon[0]; // whether weapon name is exist
}

bool client_t::is_dead(){
	return health<=0.f;
}
bool client_t::is_avail(){
	return (!is_dead()) || (dead_time<c_availTime);
}

void client_t::respawn(){
	pos=hist[hist_pos];
	vel=vec3_t(0.f, 0.f, 0.f);
	ang=hist_ang[hist_pos];
	health=c_maxHealth;
	regen_delay=0.f;
	if(mp->getProgress(pos+vec3_t(0.f, .3f, 0.f))>mp->getProgress(pos+vec3_t(0.f, .3f, 0.f)+
	   rotate(vec3_t(0.f, 0.f, 0.01f), ang))){ // wrong way!
		ang.y+=M_PI; // direction is reversed
	}
	weapon[0]=0; // lost weapon

	weapon_delay=0.f;
}

void client_t::damage(float amount, vec3_t from, int by){
	if(!is_avail())
		return;
	SV_blood(pos);
	if(alive_time<c_invincibleTime)
		return;
	if(!enable)
		return;
	char buf[64];
	health-=amount;
	if(health+amount>0.f && health<=0.f){
		// died!
		killedBy(by);
	}
	if(by>=0){
		// injured by another client
		SV_hit(by, getID());
	}
	
	SV_sound("impact", pos, 2.f);
	SV_hurt(getID(), amount, from);
	if(health<-80.f && health+amount>-80.f){
		// gib!
		dead_time=c_availTime+.01f;
		assert(!is_avail());
		// blow away
		explode();
	}
	// is alive...
	if(!is_dead()){
		// hurt sound!
		sprintf(buf, "hurt%d", 1+(rand()%4));
		//SV_sound(buf, pos, 28.f);
	}
}

void client_t::killedBy(int by){
	char buf[256];
	int cl=by&BYF_MASKCLIENT;
	if(by>=0 && cl!=getID()){
		// killed
		client_t& cc=cli[cl];
		int flg=by&BYF_MASKFLAG;
		if(flg==BYF_EXPLOSION)
			sprintf(buf, "%s was killed in %s's explosion", name, cc.name);
		else if(flg==BYF_COLISION)
			sprintf(buf, "%s was ran into by %s", name, cc.name);
		else{
			weapon_t *w=W_find(cc.weapon);
			if(w){
				if(flg==BYF_WEAPON)
					sprintf(buf, w->fragMsg, name, cc.name);
				else if(flg==BYF_WEAPONSPL)
					sprintf(buf, w->splashFragMsg, name, cc.name);
			}else{
				sprintf(buf, "%s was killed by %s", name, cc.name);
			}
		}
		SV_msg(buf);
		SV_fraggedMsg(cl, "You have fragged %s", name);
		cli[cl].score++;
	}else if(cl==getID()){
		// suicide
		weapon_t *w=W_find(weapon);
		if(w){
			sprintf(buf, w->suicideMsg, name);
		}else{
			sprintf(buf, "%s suicided", name);
		}
		SV_msg(buf);
		if(suicideTime<0.f){
			score--;
			suicideTime=c_repeatSuicideTime;	
		}
	}else if(by==BY_MAP){
		sprintf(buf, "%s was in the wrong place", name);
		SV_msg(buf);
		if(suicideTime<0.f){
			score--;
			suicideTime=c_repeatSuicideTime;	
		}
	}
}

void client_t::instantKill(int by){
	damage(health+100.f, pos+rotate(vec3_t(0.f, 0.f, 1.f), ang), by);
}

void client_t::explode(){
	SV_explodeBig(pos);
	SV_bloodBig(pos);
	SV_sound("exp1", pos, 192.f);
	SV_sound("exp2", pos, 24.f);
	//SV_sound("exp3", pos, 256.f);
	SV_screenKick(.3f);
	splashDamage(pos, 10.f, 30.f, getID()|BYF_EXPLOSION);
}

vec3_t client_t::getWeaponPos(){
	return vec3_t(.3f, 0.4f, .0f);
}

int client_t::getID(){
	int n;
	for(n=0;n<MAX_CLIENTS;n++)
		if(&(cli[n])==this)
			return n;
	return -1;
}

int C_getRank(int c){
	typedef struct{
		static bool compare(const client_t * c1, const client_t * c2){
			return c1->score>c2->score;
		}	
	} comparer_t;
	vector<const client_t *> cs;
	int n;
	for(n=0;n<MAX_CLIENTS;n++)
		if(cli[n].enable && (!cli[n].spectate))
			cs.push_back(&(cli[n]));
	stable_sort(cs.begin(), cs.end(), comparer_t::compare);
	return find(cs.begin(), cs.end(), (const client_t *)(&(cli[c])))-cs.begin();
}
