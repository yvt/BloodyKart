/*
 *  ai.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/27.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "map.h"
#include "ai.h"
#include "weapon.h"

ai_t::ai_t(){
	steer=0.f;
	ohit=SDL_GetTicks();
	backstart=0;
	stopStart=0;
	mindist=0.4f+rnd()*.6f;
	fireTime=rnd();
	firing=false;
	likeFiring=true;
	dotFactor=1.f;
	itemFindTime=rnd()*2.f;
	goTarget=0;
}
void ai_t::bind(client_t * c){
	
	this->controller_t::bind(c);
}
void ai_t::framenext(float dt){
	//steer*=pow(0.2f, dt);
	client->slip=
	0.f;
	fireTime-=dt;
	if(firing)
		dotFactor*=powf(.04f+rnd()*.06f, dt);
	else
		dotFactor+=(1.f-dotFactor)*(1.f-powf(.01f+rnd()*.05f, dt));
	curPrg=mp->getProgress(client->pos+vec3_t(0.f, .5f, 0.f));
	// item finding
	itemFindTime-=dt;
	if(itemFindTime<=0.f){
		calcTarget();
		itemFindTime=.3f+rnd()*.3f;
	}
	targetTime+=dt;
}
void ai_t::calcTarget(){
	float prg;
	if(client->is_dead()){
		goTarget=0;
		return;
	}
	if(goTarget){
		// if passed it...
		prg=mp->getProgress(targetPos+vec3_t(0.f, .5f, 0.f));
		if(curPrg>.8f && prg<.2f)
			prg+=1.f;
		if(curPrg>prg){
			// ignore it.
			goTarget=0;
		}
		
		if(goTarget==1){
			// if already gotten...
			if(mp->itemspawn[targetItem]<0.f)
				goTarget=0;
		}else if(goTarget==2){
			// if already dead...
			if(cli[targetClient].is_dead())
				goTarget=0;
			// or don't have weapon...
			if(!client->has_weapon())
				goTarget=0;
			if(goTarget){
				vector<isect_t> *lst;
				vec3_t v1, v2;
				v1=client->pos+vec3_t(0.f, .5f, 0.f);
				v2=cli[targetClient].pos+vec3_t(0.f, .5f, 0.f);
				lst=mp->m->raycast(v1, v2);
				if(lst->size()>1){ // reachable...
					delete lst; // can't!
					if(targetTime>2.f){
						// ignore him
						goTarget=0;
					}	
				}else{
					delete lst; // able!
					targetPos=cli[targetClient].pos;
				}
			}
		}
	}
	if(!goTarget){
		// if no weapon is already gotten, or being gotten
		targetTime=0.f;
		if(client->weapon[0]==0){
			// find reachable gettable near items
			//  which are on way, and in the front of client
			vector<int> itms;
			const int shiftItem=0;
			const int shiftClient=65536;
			int n;
			vec3_t frontVec=rotate(vec3_t(0.f, 0.f, 1.f), client->ang);
			for(n=0;n<mp->items;n++){
				if(mp->itemspawn[n]<0.f) // gettable...
					continue;
				vec3_t dif=mp->item[n]-client->pos;
				if(dif.length()<15.f){ // near...
					vec3_t itemVec;
					itemVec=(mp->item[n]-client->pos).normalize();
					if(vec3_t::dot(itemVec, frontVec)<0.f) // front..
						continue;
					if(!mp->isWay(mp->item[n]+vec3_t(0.f, .5f, 0.f)))
						continue;
					if(!mp->isWay(mp->item[n]+vec3_t(0.f, .5f, 0.f)+itemVec))
						continue;
					prg=mp->getProgress(mp->item[n]+vec3_t(0.f, .5f, 0.f));
					if(curPrg>.8f && prg<.2f)
						prg+=1.f;
					if(prg>curPrg){ // on way...
						vector<isect_t> *lst;
						vec3_t v1, v2;
						v1=client->pos+vec3_t(0.f, .5f, 0.f);
						v2=mp->item[n]+vec3_t(0.f, .5f, 0.f);
						lst=mp->m->raycast(v1, v2);
						if(lst->size()){ // reachable...
							delete lst;
							continue;
						}else{
							delete lst;
						}
						itms.push_back(n+shiftItem);
					}
				}
			}
			if(client->has_weapon()){
				for(n=0;n<MAX_CLIENTS;n++){
					if(!cli[n].enable) // exist
						continue;
					if(cli[n].spectate) // not spectating
						continue;
					vec3_t dif=mp->item[n]-client->pos;
					if(dif.length()<15.f){ // near...
						vec3_t cliVec;
						cliVec=(cli[n].pos-client->pos).normalize();
						if(vec3_t::dot(cliVec, frontVec)<0.f) // front..
							continue;
						if(!mp->isWay(cli[n].pos+vec3_t(0.f, .5f, 0.f)))
							continue;
						if(!mp->isWay(cli[n].pos+vec3_t(0.f, .5f, 0.f)+cliVec))
							continue;
						prg=mp->getProgress(cli[n].pos+vec3_t(0.f, .5f, 0.f));
						if(curPrg>.8f && prg<.2f)
							prg+=1.f;
						if(prg>curPrg){ // on way...
							vector<isect_t> *lst;
							vec3_t v1, v2;
							v1=client->pos+vec3_t(0.f, .5f, 0.f);
							v2=cli[n].pos+vec3_t(0.f, .5f, 0.f);
							lst=mp->m->raycast(v1, v2);
							if(lst->size()>1){ // reachable...
								delete lst; // can't!
								continue;
							}else{
								delete lst;
							}
							itms.push_back(n+shiftClient);
						}
					}
					
				}
			}
			// select randomly
			if(!itms.size()){
				// no items found; aborting
				return;
			}
			
			n=itms[rand()%itms.size()];
			
			if(n>=shiftClient){
				targetPos=cli[n-shiftClient].pos+vec3_t(0.f, .3f, 0.f);
				targetClient=n;
				goTarget=2;
			}else if(n>=shiftItem){
				targetPos=mp->item[n-shiftItem]+vec3_t(0.f, .3f, 0.f);
				targetItem=n;
				goTarget=1;
			}
		}
	}
}
float ai_t::get_steering(){
	float ll, rr;
	float cpos=curPrg;
	/*if((!mp->isWay(client->pos+rotate(vec3_t(-mindist, 0.4f, 1.8f), client->ang))) && 
	   (!mp->isWay(client->pos+rotate(vec3_t(mindist, 0.4f, 1.8f), client->ang)))){
		return -4.;
	}else if(!mp->isWay(client->pos+rotate(vec3_t(-mindist, 0.8f, 5.3f), client->ang))){
		return -4.;
	}else if(!mp->isWay(client->pos+rotate(vec3_t(mindist, 0.8f, 5.3f), client->ang))){
		return 4.;
	}*//*else if(!mp->isWay(client->pos+rotate(vec3_t(-mindist, 0.8f, 10.3f), client->ang))){
		return -4.;
	}else if(!mp->isWay(client->pos+rotate(vec3_t(mindist, 0.8f, 10.3f), client->ang))){
		return 4.;
	}*/
	
	float dst;
	vec3_t newang;
	// predictate angle
	newang=client->ang;
	newang.y+=getFrontSpeed()*client->view_steer*client->steer*.0f*.5f;
	
	// go toward item
	if(goTarget){
		if(mp->itemspawn[targetItem]>0.f){
			vec3_t rightVec;
			vec3_t tgtVec;
			float strength;
			rightVec=rotate(vec3_t(1.f, 0.f, 0.f), newang);
			tgtVec=(targetPos-client->pos).normalize();
			strength=-vec3_t::dot(rightVec, tgtVec)*4.f;
			if(strength>1.f)
				strength=1.f;
			if(strength<-1.f)
				strength=-1.f;
			client->view_steer=strength; 
			return strength;
		}
	}
	
	
	
	for(dst=.8f;dst<5.f;dst*=1.3f){
		bool a, b;
		a=mp->isWay(client->pos+rotate(vec3_t(-mindist, 0.5f*dst+.5f, dst), newang));
		b=mp->isWay(client->pos+rotate(vec3_t(mindist, 0.5f*dst+.5f, dst), newang));
		if((!a) && (!b))
			return -4.f; // oh no! dead end?
		
		float strength=7.f/dst;
		strength=min(strength, 5.f);
		
		if(!a){
			client->view_steer=-strength; 
			return -strength;
		}
		if(!b){
			client->view_steer=strength; 
			return strength;
		}
	}
	
	// avoid collision by wall
	
	{
		vec3_t v1, v2;
		v1=client->pos+vec3_t(0.f, .3f, 0.f);
		v2=v1+rotate(vec3_t(0.f, 0.f, 1.f), newang);;
		
		vector<isect_t> *lst;
		lst=mp->m->raycast(v1, v2);
		if(lst->size()){
			isect_t sct; vec3_t norm;
			sct=*min_element(lst->begin(), lst->end());
			norm=vec3_t::normal(sct.v1, sct.v2, sct.v3);
			delete lst;
			
			vec3_t rightVec;
			rightVec=rotate(vec3_t(1.f, 0.f, 0.f), newang);
			
			if(vec3_t::dot(rightVec, norm)>0.f){
				return -1.f;
			}else{
				return 1.f;
			}
			
		}else{
			delete lst;
		}
	}
	
	ll=mp->getProgress(client->pos+rotate(vec3_t(-.5f, 1.4f, 1.7f), newang));
	rr=mp->getProgress(client->pos+rotate(vec3_t(.5f, 1.4f, 1.7f), newang));
	if(cpos>.8f){
		if(ll<.2f && ll>=-.01f)
			ll+=1.f;
		if(rr<.2f && rr>=-.01f)
			rr+=1.f;
	}
	int l, r;
	l=(int)(ll*10.f);
	r=(int)(rr*10.f);
	if(l>r)
		steer=4.f;
	else if(l<r)
		steer=-4.f;
	else
		steer=0.f;
	
	if(!mp->isWay(client->pos+rotate(vec3_t(-steer*2.f, 0.5f, 2.f), newang)))
		steer=0.f; // going to fall!
	
	if(getFrontSpeed()<0.f)
		steer=-steer;
	
	//steer=((int)(rr*30.f)<=(int)(ll*30.f))?1.f:-1.f;
	//steer-=.3f;
	client->view_steer=steer; // cheat for ai:)
	return steer;
}
float ai_t::getFrontSpeed(){
	return vec3_t::dot(client->vel, rotate(vec3_t(0.f, 0.f, 1.f), client->ang));
}	
int ai_t::get_accel(){
	//return 0;
	if(!mp->isWay(client->pos+rotate(vec3_t(0.0f, 0.4f, 0.8f), client->ang)))
		return -1;
	if(SDL_GetTicks()<stopStart+1500 && getFrontSpeed()>=0.f)
		return 0;
	return 1;
	return (mp->isWay(client->pos+rotate(vec3_t(0.5f, 0.4f, 1.2f), client->ang)) ||
	mp->isWay(client->pos+rotate(vec3_t(-0.5f, 0.4f, 1.2f), client->ang)))?1:-1;
}
void ai_t::hit(vec3_t vc){
	/*if(SDL_GetTicks()<ohit+200){
		if(SDL_GetTicks()>backstart+800)
		backstart=SDL_GetTicks();
		else
			backstart=0;
	}
	ohit=SDL_GetTicks();
	steer+=vec3_t::dot(rotate(vec3_t(-1.f, 0.f, 0.f), client->ang), vc)*2.6f;*/
	this->controller_t::hit(vc);
}
void ai_t::clientHit(int by, vec3_t pos){
	vec3_t dir=rotate(vec3_t(0.f, 0.f, 1.f), client->ang);
	vec3_t dir2=(pos-client->pos).normalize();
	if(vec3_t::dot(dir, dir2)>.3f){
		stopStart=SDL_GetTicks();
	}	
}
bool ai_t::fire(){
	//return false;
	client_t& c=*client;
	bool ofire=firing;
	weapon_t *w=W_find(c.weapon);
	
	if(fireTime<0.f){
		vec3_t v1, v2;
		vector<raycast_t> res;
		
		if(likeFiring){
			firing=false;
			int n;
			vec3_t dir2;
			dir2=rotate(vec3_t(0.f, 0.f, 1.f), c.ang);
			if(w){
				for(n=0;n<MAX_CLIENTS;n++){
					client_t& c2=cli[n];
					if(c.getID()==c2.getID())
						continue;
					if(!c2.enable)
						continue;
					if(!c2.is_avail())
						continue;
					vec3_t dir;
					float minDot;
					dir=c2.pos-c.pos;
					dir=dir.normalize();
					minDot=((w->splashRange>.1f)?.8f:.86f);
					minDot+=(1.f-minDot)*(1.f-dotFactor);
					if(vec3_t::dot(dir, dir2)>minDot){
						// may fire...
						
						
						float dmg=0.f;
						
					
						if(w->splashRange>.1f){
							dmg=(w->splashRange-(c2.pos-c.pos).length())/w->splashRange;
							dmg=max(0.f, dmg);
							dmg=w->splashDamage*dmg*dmg;
							dmg*=1.3f;
						}
						
						if(c.health-dmg>0.f){
							// can bullet arrive ?
							bool hits=true;
							v1=c.pos+rotate(c.getWeaponPos()+w->getFirePos(), c.ang);
							v2=c2.pos; v2.y+=.3f;
							res.clear();
							raycast(v1, v2, res);
							sort(res.begin(), res.end());
							for(vector<raycast_t>::iterator it=res.begin();it!=res.end();it++){
								if(it->dest==HD_map){
									hits=false;
									break;
								}
							}
							if(hits){
								firing=true; // fire!
								break;
							}
						}
					}
				}
			}
			
			// safety check 
			
			if(w && w->splashRange>.1f){
			
				v1=c.pos+rotate(c.getWeaponPos()+w->getFirePos(), c.ang); v1.y+=.3f;
				v2=v1+rotate(vec3_t(0.f, 0.f, 1000.f), c.ang);
				raycast(v1, v2, res);
				sort(res.begin(), res.end());
				
				
				for(vector<raycast_t>::iterator it=res.begin();it!=res.end();it++){
					if((it->dest==HD_map) || (it->dest==HD_client && (&cli[it->cli]!=client))){
						// estimate splash damage to himself
						float dmg=0.f;
					
						dmg=(w->splashRange-(it->pos-c.pos).length())/w->splashRange;
						dmg=max(0.f, dmg);
						dmg=w->splashDamage*dmg*dmg;
						dmg*=1.3f;
						
						if(c.health-dmg<0.f){
							stopStart=SDL_GetTicks();
							firing=false;
						}
						break;
					}
					
				}
				
			}
			
		}else{
		
			res.clear();
			float cx, cy;
			firing=false;
			cx=rnd()*100.f-50.f;
			cy=rnd()*100.f-50.f;
			v1=c.pos; v1.y+=.5f;
			v2=v1+rotate(vec3_t(cx, cy, 1000.f), c.ang);
			raycast(v1, v2, res);
			sort(res.begin(), res.end());
			
			
			for(vector<raycast_t>::iterator it=res.begin();it!=res.end();it++){
				if(it->dest==HD_map){
					break;
				}
				if(it->dest==HD_client && (&cli[it->cli]!=client)/* && dynamic_cast<ai_t *>(cli[it->cli].ctrl)==NULL*/){
					// estimate splash damage to himself
					float dmg=0.f;
					if(w){
						if(w->splashRange>.1f){
							dmg=(w->splashRange-(it->pos-c.pos).length())/w->splashRange;
							dmg=max(0.f, dmg);
							dmg=w->splashDamage*dmg*dmg;
							dmg*=1.3f;
						}
					}
					if(c.health-dmg>0.f)
						firing=true;
					else
						stopStart=SDL_GetTicks();
					break;
				}
				
			}
			
		}
			
		fireTime+=.1f;
		if(firing)
			fireTime+=.5f;
	}
	if(!c.has_weapon())
		return false;
	if(c.ammo==0){
		// to do is throw away
		firing=!ofire;
	}
	
	return firing;
	
}
bool ai_t::isFoward(vec3_t v){
	float prg=mp->getProgress(v+vec3_t(0.f, .5f, 1.f));
	if(curPrg>.8f){
		// wrap
		if(prg<.2f)
			prg+=1.f;
	}
	return prg>curPrg;
}