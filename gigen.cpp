/*
 *  gigen.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/10/04.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "map.h"
#include "gigen.h"
#include "skybox.h"
#include <sys/stat.h>
#include "flare.h"

static mesh *m;
const int phases=1;
static aabb_t bound;
static volatile int cphase=0;
static volatile int cvertex0=0;
static volatile int cvertex1=0;
static volatile int remainTime=0;

static const int subDiv=8;
static const int hRes=24;
static const int vRes=12;
static const float gridSize=1.2f;
static const int gridTrace=156;

static vec3_t *dLights;
static vec3_t *ogi;
static vec3_t *rots;
static gg_mode_t gmode;

bool gg_multiThread=true;

struct ggvec_t{
	vec3_t pos;
	vec3_t color; float alpha;
	float u, v; vec3_t gI;
	float shadow;
	int face;
	vec3_t dL;
};

ggvec_t mixVec(ggvec_t v1, ggvec_t v2, float per){
	v1.pos+=(v2.pos-v1.pos)*per;
	v1.color+=(v2.color-v1.color)*per;
	v1.alpha+=(v2.alpha-v1.alpha)*per;
	v1.u+=(v2.u-v1.u)*per;
	v1.v+=(v2.v-v1.v)*per;
	v1.gI+=(v2.gI-v1.gI)*per;
	v1.shadow+=(v2.shadow-v1.shadow)*per;
	v1.dL+=(v2.dL-v1.dL)*per;
	return v1;
}
ggvec_t calcVec(int face, int ind){
	ggvec_t r;
	xmaterial mt;
	int j;
	mt=m->mat[m->face[(face<<2)+3]];
	r.color=vec3_t(mt.dr, mt.dg, mt.db);
	if(mt.texed)
	r.color*=vec3_t(mt.ar, mt.ag, mt.ab);
	
	if(mp->isOcean[m->face[(face<<2)+3]]){
		r.color*=vec3_t(0.2f, 0.6f, 1.f);
	}
	// TODO: interpolate oGI value
	
	r.shadow=((max(0.f, vec3_t::dot(m->normal[ind], mp->sunPos))));
	r.alpha=mt.aa*mt.da;
	if(m->uv){
	r.u=m->uv[ind].x;
	r.v=m->uv[ind].y;
	}
	if(m->face[(face<<2)]==ind){
		r.gI=vec3_t(0.f, 0.f, 0.f);
	}else if(m->face[(face<<2)+1]==ind){
		r.gI=vec3_t(0.f, m->giDiv-1, 0.f);
	}else if(m->face[(face<<2)+2]==ind){
		r.gI=vec3_t(m->giDiv-1, m->giDiv-1, 0.f);
	}
	//r.gI=ogi[ind];
	r.face=face;
	r.pos=m->vertex[ind];
	if(m->face[(face<<2)+3]!=1 && (r.color.x+r.color.y+r.color.z>.2f)){
		//ind=ind;
	}
	return r;
}
vec3_t ipFace(int face, vec3_t pos){
	ggvec_t v1, v2, v3;
	plane_t plane, px;
	v1=calcVec(face, m->face[(face<<2)]);
	v2=calcVec(face, m->face[(face<<2)+1]);
	v3=calcVec(face, m->face[(face<<2)+2]);
	
	plane=plane_t(v1.pos, v2.pos, v3.pos);
	px=plane_t(v2.pos, v3.pos, v2.pos+plane.n);
	
	float per;
	ggvec_t i1, i2;
	
	per=1.f-px.distance(pos)/(px.distance(v1.pos)+.0000001f);
	
	i1=mixVec(v1, v2, per);
	i2=mixVec(v1, v3, per);
	
	px=plane_t(i1.pos, i1.pos+plane.n, v2.pos+(v2.pos-v1.pos));
	
	per=px.distance(pos)/(px.distance(i2.pos)+.0000001f);
	
	ggvec_t i;
	
	i=mixVec(i1, i2, per);
	
	int ind;
	ind=face*m->giDiv*m->giDiv;
	
	vec3_t a, b, c, d;
	int ix, iy;
	float fx, fy;
	
	ix=(int)floorf(i.gI.x);
	iy=(int)floorf(i.gI.y);
	fx=i.gI.x-floorf(i.gI.x);
	fy=i.gI.y-floorf(i.gI.y);
	
#define readVec(x, y)	ogi[ind+((x)&(m->giDiv-1))+((y)&(m->giDiv-1))*m->giDiv]
#define readShd(x, y)	m->giShadow[ind+((x)&(m->giDiv-1))+((y)&(m->giDiv-1))*m->giDiv]
#define readGLt(x, y)	dLights[ind+((x)&(m->giDiv-1))+((y)&(m->giDiv-1))*m->giDiv]
	
	vec3_t dif;
	
	dif=1.f;//i.shadow;
	
	a=readShd(ix, iy); b=readShd(ix+1, iy);
	c=readShd(ix, iy+1); d=readShd(ix+1, iy+1);
	
	a+=(b-a)*fx;
	c+=(d-c)*fx;
	a+=(c-a)*fy;
	
	dif=mp->sunColor*(a);
	
	a=readGLt(ix, iy); b=readGLt(ix+1, iy);
	c=readGLt(ix, iy+1); d=readGLt(ix+1, iy+1);
	
	a+=(b-a)*fx;
	c+=(d-c)*fx;
	a+=(c-a)*fy;
	
	dif+=a;
	
	a=readVec(ix, iy); b=readVec(ix+1, iy);
	c=readVec(ix, iy+1); d=readVec(ix+1, iy+1);
	
	a+=(b-a)*fx;
	c+=(d-c)*fx;
	a+=(c-a)*fy;

	
	dif+=a;
	
	i.color*=dif;
	
	return i.color;
}
static float clamp(float v){
	if(v<0.f)
		v=0.f;
	if(v>1.f)
		v=1.f;
	return v;
}
static void ggPhase(bool th0, bool th1){
	int f,  i, k;
	int ind=0, ind2, ind3;
	if(th0 && th1)
		consoleLog("ggPhase: started phase %d\n", cphase);
	else if(th0)
		consoleLog("ggPhase: started phase %d on thread 1\n", cphase);
	else if(th1)
		consoleLog("ggPhase: started phase %d on thread 2\n", cphase);
	if(gmode==GG_grd){
		/* lightData mode */
		Uint32 ot=SDL_GetTicks();
		vec3_t v1, v2;
		
		lightData_t *data=mp->lightData;
		for(f=0;f<data->w*data->h*data->d;f++){
			// thread's own work
			if((!th0) && ((f&1)==0)){
				continue;
			}
			if((!th1) && ((f&1)==1)){
				continue;
			}
			 
			
			
			int xx, yy, zz;
			vec3_t v;
			xx=f%data->w;
			yy=(f/data->w)%data->h;
			zz=f/data->w/data->h;
			assert(xx>=0 && xx<data->w);
			assert(yy>=0 && yy<data->h);
			assert(zz>=0 && zz<data->d);
			v=data->calcCoord(xx, yy, zz);
			
			// coordinate
			lightPoint_t& p=data->nearest(xx, yy, zz);
			
			// reset
			p.v00=p.v10=p.v11=p.v12=vec3_t(.0f);
			
			// trace
			vec3_t dir;
			for(i=0;i<gridTrace;i++){
				// calc direction
				dir=vec3_t(rnd()-.5f, rnd()-.5f, rnd()-.5f);
				dir=dir.normalize();
				
				// trace!
				v1=v; v2=v1;
				v2+=dir*3000.f;
				
				vector<isect_t> *res=m->raycast(v1, v2, -1);
				vec3_t col;
				
				// if no object was hit, sky color is used.
				col=mp->skyColor;
				//sort(res->begin(),res->end());
		
				while(res->size()){
					isect_t mn=*min_element(res->begin(), res->end());
					if(mn.dist>0.f){
						plane_t plane(m->vertex[m->face[(mn.face<<2)]],
									  m->vertex[m->face[(mn.face<<2)+1]],
									  m->vertex[m->face[(mn.face<<2)+2]]);
						if(plane.distance(v1)>0.f){
							vec3_t pos;
							pos=v1+(v2-v1).normalize()*mn.dist;
							col=ipFace(mn.face, pos);
							//printf("%f, %f, %f - %f, %f, %f - %f, %f, %f\n", v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, col.x, col.y, col.z);
							break;
						}
					}
					res->erase(min_element(res->begin(), res->end()));
				}
				delete res;
				
				// apply
				
				col/=(float)gridTrace*2.f;
				p.v00+=col;
				p.v10+=col*dir.x;
				p.v11+=col*dir.y;
				p.v12+=col*dir.z;
				
			}
			/*
			// direct light
			for(k=0;k<mp->lights;k++){
				vec3_t dif, dir;
				float d, att;
				dif=v-mp->lightPos[k];
				dir=dif.normalize();
				
				vector<isect_t> *rc2=m->raycast(v, mp->lightPos[k]+dir*.01f, f);
				if(rc2->size()){
					// in shadow
					delete rc2;
					continue;
				}else{
					delete rc2;
				}
				
				d=vec3_t::dot(dir, mp->lightDir[k].normalize());
				att=dif.rlength();
				if(mp->lightPower[k]<.001f){
					// Point Light
					//att*=max(0.f, -vec3_t::dot(nrm, dir));
				}else if(d>=0.00001f){
					// Spot Light
					att*=powf(d, mp->lightPower[k]);
					//att*=max(0.f, -vec3_t::dot(nrm, dir));
				}else{
					att=0.f;
				}
				p.v00+=mp->lightColor[k]*.5f*att;
				p.v10-=mp->lightColor[k]*.5f*att*dir.x;
				p.v11-=mp->lightColor[k]*.5f*att*dir.y;
				p.v12-=mp->lightColor[k]*.5f*att*dir.z;
				
			}
			
			// sun
			do{
				vec3_t dif, dir;
				float d, att;
				dif=mp->sunPos;
				dir=dif.normalize();
				
				vector<isect_t> *rc2=m->raycast(v, v+dir*30000.f);
				if(rc2->size()){
					// in shadow
					delete rc2;
					continue;
				}else{
					delete rc2;
				}
				
		
				p.v00+=mp->sunColor*.5f;
				p.v10+=mp->sunColor*.5f*dir.x;
				p.v11+=mp->sunColor*.5f*dir.y;
				p.v12+=mp->sunColor*.5f*dir.z;
				
				//printf("%f, %f, %f ", p.v00.x, p.v00.y, p.v00.z);
				//printf("%f, %f, %f ", p.v10.x, p.v10.y, p.v10.z);
				//printf("%f, %f, %f\n", p.v11.x, p.v11.y, p.v11.z);
			}while(0);*/
			
			// predictate time
			{
				float spd=(float)f/(float)(SDL_GetTicks()-ot)*1000.f;
				remainTime=(int)((float)(data->w*data->h*data->d-f)/spd);
			}
			// report
			if(th0)
				cvertex0=f;
			if(th1)
				cvertex1=f;
		}
		
	}else{
		vec3_t norm, uu, vv, up;
		const float perscl=(subDiv==1)?(0.f):(1.f/float(subDiv-1));
		memcpy(ogi, m->gi, sizeof(vec3_t)*m->count_face*subDiv*subDiv);
		for(int n=0;n<m->count_vertex;n++)
			rots[n]=vec3_t(rnd(), rnd(), rnd());
		Uint32 ot=SDL_GetTicks();
		if(cphase==-1){
			// calc shadow
			ind=0;
			cphase=-1;
		
			for(f=0;f<m->count_face;f++){
				
				if((!th0) && ((f&1)==0)){
					ind+=subDiv*subDiv;
					continue;
				}
				if((!th1) && ((f&1)==1)){
					ind+=subDiv*subDiv;
					continue;
				}
				
				ind2=ind; ind3=ind;
				//rintf("%d %d %d\n", f, th0?1:0, th1?1:0);
				if(th0)
					cvertex0=f;
				if(th1)
					cvertex1=f;
				
				int find1, find2, find3;
				find1=m->face[(f<<2)];
				find2=m->face[(f<<2)+1];
				find3=m->face[(f<<2)+2];
				
				vec3_t fNormal; // normal from face
				fNormal=vec3_t::normal(m->vertex[find1], m->vertex[find2], m->vertex[find3]);

				for(int n=0;n<subDiv;n++)
					for(int j=0;j<subDiv;j++){
						if(j>n){
							m->giShadow[ind]=rnd();
							dLights[ind]=0.f;
							m->lit[ind]=0.f;
							if(m->lit1){
								m->lit1[ind]=0.f;
								m->lit2[ind]=0.f;
								m->lit3[ind]=0.f;
								m->lit4[ind]=0.f;
							}
							ind++;
							continue;
						}
						
						dLights[ind]=0.f;
						m->giShadow[ind]=0.f;
						m->lit[ind]=0.f;
						
						for(int a1=0;a1<2;a1++){
							for(int a2=0;a2<2;a2++){
								vec3_t vrt, v1, v2, uu1, uu2, uu;
								vec3_t nrm, n1, n2, vv1, vv2, vv;
								vec3_t rot;
								float per1, per2;
								per1=(float)n*perscl;
								if(n)
									per2=(float)j/(float)n;
								else
									per2=0.f;
								per1=clamp(per1);
								per2=clamp(per2);
								
								
								
								v1=m->vertex[find1]+(m->vertex[find2]-m->vertex[find1])*per1;
								v2=m->vertex[find1]+(m->vertex[find3]-m->vertex[find1])*per1;
								n1=m->normal[find1]+(m->normal[find2]-m->normal[find1])*per1;
								n2=m->normal[find1]+(m->normal[find3]-m->normal[find1])*per1;
								if(m->oUU){
									uu1=m->oUU[find1]+(m->oUU[find2]-m->oUU[find1])*per1;
									uu2=m->oUU[find1]+(m->oUU[find3]-m->oUU[find1])*per1;
									vv1=m->oVV[find1]+(m->oVV[find2]-m->oVV[find1])*per1;
									vv2=m->oVV[find1]+(m->oVV[find3]-m->oVV[find1])*per1;
								}
								
								vrt=v1+(v2-v1)*per2;
								nrm=n1+(n2-n1)*per2;
								if(m->oUU){
									uu=uu1+(uu2-uu1)*per2;
									vv=vv1+(vv2-vv1)*per2;
									uu=uu.normalize();
									vv=vv.normalize();
								}else{
									uu=vv=nrm;
								}
								
								v1=vrt+mp->sunPos*.0f;
								v2=vrt+mp->sunPos*10000.f;
								if(vec3_t::dot(fNormal, mp->sunPos)<0.f){
									// not toward sun
									m->giShadow[ind]+=0.f;
								}else{
									vector<isect_t> *rc=m->raycast(v1, v2, f);
									if(rc->size())
										m->giShadow[ind]+=0.f;
									else
										m->giShadow[ind]+=.25f;
									delete rc;
								}
								
								vec3_t dl(0.f, 0.f, 0.f);
								vec3_t dl1(0.f, 0.f, 0.f);
								vec3_t dl2(0.f, 0.f, 0.f);
								vec3_t dl3(0.f, 0.f, 0.f);
								vec3_t dl4(0.f, 0.f, 0.f);
								
								for(k=0;k<mp->lights;k++){
									vec3_t dif, dir;
									float d, att, att1, att2, att3, att4;
									dif=vrt-mp->lightPos[k];
									dir=dif.normalize();
									
									vector<isect_t> *rc2=m->raycast(vrt, mp->lightPos[k]+dir*.01f, f);
									if(rc2->size()){
										// in shadow
										delete rc2;
										continue;
									}else{
										delete rc2;
									}
									
									d=vec3_t::dot(dir, mp->lightDir[k].normalize());
									att=dif.rlength();
									if(mp->lightPower[k]<.001f){
										// Point Light
										 att1=att2=att3=att4=att;
										att*=max(0.f, -vec3_t::dot(nrm, dir));
										att1*=max(0.f, -vec3_t::dot(uu, dir));
										att2*=max(0.f, -vec3_t::dot(vv, dir));
										att3*=max(0.f, vec3_t::dot(uu, dir));
										att4*=max(0.f, vec3_t::dot(vv, dir));
									}else if(d>=0.00001f){
										// Spot Light
										att*=powf(d, mp->lightPower[k]);
										 att1=att2=att3=att4=att;
										att*=max(0.f, -vec3_t::dot(nrm, dir));
										att1*=max(0.f, -vec3_t::dot(uu, dir));
										att2*=max(0.f, -vec3_t::dot(vv, dir));
										att3*=max(0.f, vec3_t::dot(uu, dir));
										att4*=max(0.f, vec3_t::dot(vv, dir));
									}else{
										att=0.f;
									}
									dl+=mp->lightColor[k]*att;
									dl1+=mp->lightColor[k]*att1;
									dl2+=mp->lightColor[k]*att2;
									dl3+=mp->lightColor[k]*att3;
									dl4+=mp->lightColor[k]*att4;
									
								}
								
								dLights[ind]+=dl*.25f;
								m->lit[ind]+=dl*.25f;
								if(m->lit1){
									m->lit1[ind]+=dl1*.25f;
									m->lit2[ind]+=dl2*.25f;
									m->lit3[ind]+=dl3*.25f;
									m->lit4[ind]+=dl4*.25f;
								}
								
							}
						}
						
						ind++;
					}
				ind3+=subDiv;
				for(int n=0;n<subDiv-1;n++){
					
					ind2++;
					m->giShadow[ind2]=m->giShadow[ind3]+(m->giShadow[ind3-subDiv]-m->giShadow[ind3])+
					(m->giShadow[ind3+1]-m->giShadow[ind3]);
					dLights[ind2]=dLights[ind3]+(dLights[ind3-subDiv]-dLights[ind3])+
					(dLights[ind3+1]-dLights[ind3]);
					
					ind2+=subDiv;
					ind3+=subDiv+1;
				}
				
				float spd;
				if(SDL_GetTicks()-ot){
					spd=(float)f/(float)(SDL_GetTicks()-ot)*1000.f;
					remainTime=(int)((float)(m->count_face-f)/spd);
				}
				
			}
			return;
		}
		ind=0;
		for(f=0;f<m->count_face;f++){
			if((!th0) && ((f&1)==0)){
				ind+=subDiv*subDiv;
				continue;
			}
			if((!th1) && ((f&1)==1)){
				ind+=subDiv*subDiv;
				continue;
			}
			
			ind2=ind; ind3=ind;
			if(th0)
				cvertex0=f;
			if(th1)
				cvertex1=f;
			for(int n=0;n<subDiv;n++)
				for(int j=0;j<subDiv;j++){
					if(j>n){
						m->gi[ind]=vec3_t(rnd(),rnd(),rnd());
						ind++;
						continue;
					}
					vec3_t vrt, v1, v2;
					vec3_t rot;
					float per1, per2;
					per1=(float)n*perscl;
					if(n)
						per2=(float)j/(float)n;
					else
						per2=0.f;
					per1=clamp(per1);
					per2=clamp(per2);
					per1=.0001f+per1*.9998f;
					per2=.0001f+per2*.9998f;
					
					int find1, find2, find3;
					find1=m->face[(f<<2)];
					find2=m->face[(f<<2)+1];
					find3=m->face[(f<<2)+2];
					
					v1=m->vertex[find1]+(m->vertex[find2]-m->vertex[find1])*per1;
					v2=m->vertex[find1]+(m->vertex[find3]-m->vertex[find1])*per1;
					
					vrt=v1+(v2-v1)*per2;
					
					
					
					
					v1=m->normal[find1]+(m->normal[find2]-m->normal[find1])*per1;
					v2=m->normal[find1]+(m->normal[find3]-m->normal[find1])*per1;
					norm=v1+(v2-v1)*per2;
					//norm=vec3_t::normal(m->vertex[find1], m->vertex[find2], m->vertex[find3]);
					
					v1=rots[find1]+(rots[find2]-rots[find1])*per1;
					v2=rots[find1]+(rots[find3]-rots[find1])*per1;
					rot=v1+(v2-v1)*per2;
			
					if(fabs(norm.y)>.9f){
						up=vec3_t(1.f, 0.f, 0.f);
					}else{
						up=vec3_t(0.f, 1.f, 0.f);
					}
					
					
					uu=vec3_t::cross(norm, up).normalize();
					vv=vec3_t::cross(uu, norm).normalize();
					vec3_t cols;
					float total=0.f;
					cols=vec3_t(0, 0, 0);
					plane_t plane(m->vertex[find1], m->vertex[find2], m->vertex[find3]);
					
					
					if(n==j)
						total+=.0001f;

					for(int h=0;h<hRes;h++){
						for(int v=0;v<vRes;v++){
							vec3_t dir;
							vec3_t ans;
							float ang;
							ang=(float)(h+rot.x)/(float)hRes*M_PI*2.f;
							dir.x=sinf(ang); dir.z=cosf(ang);
							
							ang=((float)(v+rot.y)/(float)vRes*.9f+.1f)*M_PI*.5f;
							
							dir.x*=cosf(ang); dir.z*=cosf(ang);
							dir.y=sinf(ang);
							
							ans=norm*dir.y+uu*dir.x+vv*dir.z;
							ans=ans.normalize();
							
							//vec3_t v1, v2;
							v1=v2=vrt;
							v1-=ans*0.000f;//ans*.05f;
							v2+=ans*300.f;
							
							vector<isect_t> *res=m->raycast(v1, v2, f);
							float scl;
							scl=1.f;//cosf(ang);
							//sort(res->begin(),res->end());
							
							while(res->size()){
								isect_t mn=*min_element(res->begin(), res->end());
								if(mn.face!=f && mn.dist>0.f && plane.distance(vrt+ans*mn.dist)>0.00001f){
									plane_t plane(m->vertex[m->face[(mn.face<<2)]],
												  m->vertex[m->face[(mn.face<<2)+1]],
												  m->vertex[m->face[(mn.face<<2)+2]]);
									if(plane.distance(v1)>0.f){
										vec3_t pos;
										pos=v1+(v2-v1).normalize()*mn.dist;
										cols+=ipFace(mn.face, pos)*scl;
										vec3_t nnn=cols.normalize();
										/*if(f==0 && m->face[(mn.face<<2)+3]!=1){
											m->vertex[m->face[(mn.face<<2)+0]]+=vec3_t(10,10,10);
											m->vertex[m->face[(mn.face<<2)+1]]+=vec3_t(10,10,10);
											m->vertex[m->face[(mn.face<<2)+2]]+=vec3_t(10,10,10);
										}*/
										if(cols.y>1.f){
											//i=1/0;
										}
										break;
									}
								}
								res->erase(min_element(res->begin(), res->end()));
							}
							if(res->size()==0){
								cols+=mp->skyColor*scl;
							}
							delete res;
							total+=scl;
							
						}
					}
				
					cols*=(1.f/total);
				
					//cols=vec3_t(rnd(),rnd(),rnd());
					//cols=uu*.5+.5;
					/*if((n+j)&1)
						cols=vec3_t(1,1,1);
					else
						cols=vec3_t(0,0,0);*/
					m->gi[ind++]=cols;
					
				}
			ind3+=subDiv;
			for(int n=0;n<subDiv-1;n++){
				ind2++;	
				m->gi[ind2]=m->gi[ind3]+(m->gi[ind3-subDiv]-m->gi[ind3])+
				(m->gi[ind3+1]-m->gi[ind3]);
				ind2+=subDiv;
				ind3+=subDiv+1;
			}
			
			float spd;
			if(SDL_GetTicks()-ot){
				spd=(float)f/(float)(SDL_GetTicks()-ot)*1000.f;
				remainTime=(int)((float)(m->count_face-f)/spd);
			}
		}
		
	
	}
}

static int ggSubThread(void *){
	
	ggPhase(false, true);
	return 0;
}

static int ggThread(void *){
	int n;
	cphase=-1;

	
	for(n=-1;n<phases || true;n++){
		if(gg_multiThread){
			SDL_Thread *subTh;
			subTh=SDL_CreateThread(ggSubThread, NULL);
			ggPhase(true, false);
			SDL_WaitThread(subTh, NULL);
		}else{
			ggPhase(true, true);
		}
		cphase=n+1;
		if(gmode==GG_grd){
			break; // only a phase is needed
		}
		GG_save(true);
		
	}
}
static float cx=0.f, cy=0.5f;
static float cdist=.4f;
static void ggSetMatrix(){
	float dist, ang;
	
	vec3_t sz=bound.maxvec-bound.minvec;
	vec3_t cent=(bound.minvec+bound.maxvec)*.5f;
	cent=vec3_t(0,0,0);
	dist=max(sz.x, sz.z)*cdist;
	ang=(float)SDL_GetTicks()/3000.f*0.f+cx*M_PI*2.f;
	
	camera_from=vec3_t(sinf(ang)*dist,dist*1.2f*cy, cosf(ang)*dist)+cent;
	camera_at=cent;
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.f, -(float)screen->w/(float)screen->h, .1f, 100000.f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(camera_from.x, camera_from.y, camera_from.z, camera_at.x, camera_at.y, camera_at.z,
			 0.f, 1.f, 0.f);
}
static bool dirLight=true;

static void ggSetLight(){
	glEnable(GL_LIGHTING);
	vec3_t v;
	
	v=mp->ambColor;
	v=vec3_t(0,0,0);
	float ambientcolor[]={v.x, v.y, v.z, 1};
	
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientcolor);
	{
		v=mp->sunColor;
		if(!dirLight)
			v=vec3_t(0,0,0);
		float lightcolor[]={v.x, v.y, v.z, 1};
		
		
		v=mp->sunPos;
		float lightpos[]={v.x, v.y, v.z, 0};
		
		
		glEnable(GL_LIGHT0);
		glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, lightcolor);
		glLightfv(GL_LIGHT0, GL_SPECULAR, lightcolor);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientcolor);
	}
	{
		
		v=mp->sunColor;
		if(!dirLight)
			v=vec3_t(0,0,0);
		float lightcolor[]={v.x, v.y, v.z, 1};
		
		v=-mp->sunPos; v=-vec3_t(0.f, 1.f, 0.f);
		float lightpos[]={v.x, v.y, v.z, 0};
		
		
		glEnable(GL_LIGHT1);
		glLightfv(GL_LIGHT1, GL_POSITION, lightpos);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, lightcolor);
		glLightfv(GL_LIGHT1, GL_SPECULAR, lightcolor);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientcolor);
	}
}
static Uint32 oGI=0;
static bool odir=true;
static void ggRender(){
	SDL_Delay(30);
	if(SDL_GetTicks()>oGI+1000 || dirLight!=odir){
		if(gmode!=GG_grd){ // updating gi is not needed for grd
			mp->m->updateGi(dirLight);
			oGI=SDL_GetTicks();
			odir=dirLight;
		}
	}
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor(0.5f, 0.5f, 0.5f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_COLOR_MATERIAL);
	glAlphaFunc(GL_GREATER, 0.99);
	glViewport(0, 0, screen->w, screen->h);
	
	ggSetMatrix();
	ggSetLight();
	F_cframenext(.1f);
	
	int n;
	mp->setup(camera_from, dirLight, false);
	
	if(!dirLight){
		for(n=2;n<8;n++){
			glDisable(GL_LIGHT0+n);
			glLightfv(GL_LIGHT0+n, GL_DIFFUSE, color4f(0,0,0,0));
			glLightfv(GL_LIGHT0+n, GL_SPECULAR, color4f(0,0,0,0));
		}
		glLightfv(GL_LIGHT0, GL_DIFFUSE, color4f(0.f, 0.f, 0.f, 1.f));
		glLightfv(GL_LIGHT0, GL_SPECULAR, color4f(0.f, 0.f, 0.f, 1.f));
	}
	glDisable(GL_FOG);
	glFogi(GL_FOG_END, 100000000.f);
	
	glDepthMask(GL_FALSE);
	SB_render();
	glDepthMask(GL_TRUE);
	glDisable(GL_ALPHA_TEST);
	
	for(n=0;n<64;n++)
		mp->m->invalid[n]=true;
	mp->render(dirLight);
	
	if(gmode!=GG_grd){
	
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glEnable(GL_LINE_SMOOTH);
		glLineWidth(1.5f);
		glBegin(GL_LINE_LOOP);
		glColor4f(1.f, 0.f, 0.f, 1.f);
		vec3_t v;
		v=m->vertex[m->face[(cvertex0<<2)]];
		glVertex3f(v.x, v.y, v.z);
		v=m->vertex[m->face[(cvertex0<<2)+1]];
		glVertex3f(v.x, v.y, v.z);
		v=m->vertex[m->face[(cvertex0<<2)+2]];
		glVertex3f(v.x, v.y, v.z);
		glEnd();
		glPointSize(4.f);
		glBegin(GL_POINTS);
		v=m->vertex[m->face[(cvertex0<<2)+2]];
		glVertex3f(v.x, v.y, v.z);
		glEnd();
		glLineWidth(1.5f);
		glBegin(GL_LINE_LOOP);
		glColor4f(1.f, 0.f, 0.f, 1.f);

		v=m->vertex[m->face[(cvertex1<<2)]];
		glVertex3f(v.x, v.y, v.z);
		v=m->vertex[m->face[(cvertex1<<2)+1]];
		glVertex3f(v.x, v.y, v.z);
		v=m->vertex[m->face[(cvertex1<<2)+2]];
		glVertex3f(v.x, v.y, v.z);
		glEnd();
		glPointSize(4.f);
		glBegin(GL_POINTS);
		v=m->vertex[m->face[(cvertex1<<2)+2]];
		glVertex3f(v.x, v.y, v.z);
		glEnd();
		
		glBegin(GL_LINES);
		glVertex3f(mp->sunPos.x*100000.f, mp->sunPos.y*100000.f, mp->sunPos.z*100000.f);
		glVertex3f(0.f, 0.f,0.f);
		glEnd();
		
	}else{
		int xx, yy, zz;
		lightData_t *data=mp->lightData;
		lightPoint_t p;
		float scl;
		vec3_t v;
		
		// vertex0
		n=cvertex0;
		xx=n%data->w;
		yy=(n/data->w)%data->h;
		zz=n/data->w/data->h;
		v=data->calcCoord(xx, yy, zz);
		p=data->nearest(xx, yy, zz);
		scl=p.v00.length();
		scl=max(scl, p.v10.length());
		scl=max(scl, p.v11.length());
		scl=max(scl, p.v12.length());
		scl=1.f/scl;
		//p.v00*=scl; p.v10*=scl;
		//p.v11*=scl; p.v12*=scl;
		
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glEnable(GL_LINE_SMOOTH);
		glLineWidth(4.f);
		
		glBegin(GL_LINES);
		glColor4f(p.v00.x+p.v10.x, p.v00.y+p.v10.y, p.v00.z+p.v10.z, 1.f);
		glVertex3f(v.x, v.y, v.z);
		glVertex3f(v.x+5.f, v.y, v.z);
		glColor4f(p.v00.x+p.v11.x, p.v00.y+p.v11.y, p.v00.z+p.v11.z, 1.f);
		glVertex3f(v.x, v.y, v.z);
		glVertex3f(v.x, v.y+5.f, v.z);
		glColor4f(p.v00.x+p.v12.x, p.v00.y+p.v12.y, p.v00.z+p.v12.z, 1.f);
		glVertex3f(v.x, v.y, v.z);
		glVertex3f(v.x, v.y, v.z+5.f);
		glColor4f(p.v00.x-p.v10.x, p.v00.y-p.v10.y, p.v00.z-p.v10.z, 1.f);
		glVertex3f(v.x, v.y, v.z);
		glVertex3f(v.x-5.f, v.y, v.z);
		glColor4f(p.v00.x-p.v11.x, p.v00.y-p.v11.y, p.v00.z-p.v11.z, 1.f);
		glVertex3f(v.x, v.y, v.z);
		glVertex3f(v.x, v.y-5.f, v.z);
		glColor4f(p.v00.x-p.v12.x, p.v00.y-p.v12.y, p.v00.z-p.v12.z, 1.f);
		glVertex3f(v.x, v.y, v.z);
		glVertex3f(v.x, v.y, v.z-5.f);
		glEnd();
		glPointSize(8.f);
		glBegin(GL_POINTS);
		glColor4f(p.v00.x, p.v00.y, p.v00.z, 1.f);
		glVertex3f(v.x, v.y, v.z);
		glEnd();
		
		vec3_t dir;
		for(int i=0;i<0;i++){
			// calc direction
			dir=vec3_t(rnd()-.5f, rnd()-.5f, rnd()-.5f);
			dir=dir.normalize();
			
			// trace!
			vec3_t v1=v; vec3_t v2=v1;
			v2+=dir*300.f;
			
			vector<isect_t> *res=m->raycast(v1, v2, -1);
			vec3_t col;
			
			// if no object was hit, sky color is used.
			col=mp->skyColor;
			//sort(res->begin(),res->end());
			
			while(res->size()){
				isect_t mn=*min_element(res->begin(), res->end());
				if(mn.dist>0.f){
					plane_t plane(m->vertex[m->face[(mn.face<<2)]],
								  m->vertex[m->face[(mn.face<<2)+1]],
								  m->vertex[m->face[(mn.face<<2)+2]]);
					if(plane.distance(v1)>0.f){
						vec3_t pos, col;
						pos=v1+(v2-v1).normalize()*mn.dist;
						col=ipFace(mn.face, pos);
						glLineWidth(2.f);
						
						glBegin(GL_LINES);
						glColor4f(col.x, col.y, col.z, 1.f);
						v2=pos;
						v1=v2-dir*5.f;
						glVertex3f(v1.x, v1.y, v1.z);
						glVertex3f(v2.x, v2.y, v2.z);
						glEnd();
						//printf("%f, %f, %f - %f, %f, %f - %f, %f, %f\n", v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, col.x, col.y, col.z);
						break;
					}
				}
				res->erase(min_element(res->begin(), res->end()));
			}
			if(!res->size()){
				glLineWidth(2.f);
				glBegin(GL_LINES);
				glColor4f(col.x, col.y, col.z, 1.f);
				glVertex3f(v1.x, v1.y, v1.z);
				glVertex3f(v2.x, v2.y, v2.z);
				glEnd();
			}
			delete res;
			
			// apply
			
			
			
		}
		
		
		// vertex1
		n=cvertex1;
		xx=n%data->w;
		yy=(n/data->w)%data->h;
		zz=n/data->w/data->h;
		v=data->calcCoord(xx, yy, zz);
		p=data->nearest(xx, yy, zz);
		scl=p.v00.length();
		scl=max(scl, p.v10.length());
		scl=max(scl, p.v11.length());
		scl=max(scl, p.v12.length());
		scl=1.f/scl;
		//p.v00*=scl; p.v10*=scl;
		//p.v11*=scl; p.v12*=scl;
		
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glEnable(GL_LINE_SMOOTH);
		glLineWidth(4.f);
		
		glBegin(GL_LINES);
		glColor4f(p.v00.x+p.v10.x, p.v00.y+p.v10.y, p.v00.z+p.v10.z, 1.f);
		glVertex3f(v.x, v.y, v.z);
		glVertex3f(v.x+5.f, v.y, v.z);
		glColor4f(p.v00.x+p.v11.x, p.v00.y+p.v11.y, p.v00.z+p.v11.z, 1.f);
		glVertex3f(v.x, v.y, v.z);
		glVertex3f(v.x, v.y+5.f, v.z);
		glColor4f(p.v00.x+p.v12.x, p.v00.y+p.v12.y, p.v00.z+p.v12.z, 1.f);
		glVertex3f(v.x, v.y, v.z);
		glVertex3f(v.x, v.y, v.z+5.f);
		glColor4f(p.v00.x-p.v10.x, p.v00.y-p.v10.y, p.v00.z-p.v10.z, 1.f);
		glVertex3f(v.x, v.y, v.z);
		glVertex3f(v.x-5.f, v.y, v.z);
		glColor4f(p.v00.x-p.v11.x, p.v00.y-p.v11.y, p.v00.z-p.v11.z, 1.f);
		glVertex3f(v.x, v.y, v.z);
		glVertex3f(v.x, v.y-5.f, v.z);
		glColor4f(p.v00.x-p.v12.x, p.v00.y-p.v12.y, p.v00.z-p.v12.z, 1.f);
		glVertex3f(v.x, v.y, v.z);
		glVertex3f(v.x, v.y, v.z-5.f);
		glEnd();
		glPointSize(8.f);
		glBegin(GL_POINTS);
		glColor4f(p.v00.x, p.v00.y, p.v00.z, 1.f);
		glVertex3f(v.x, v.y, v.z);
		glEnd();
		
	}
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	
	// save matrix
	glGetDoublev(GL_MODELVIEW_MATRIX, osd_old_modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, osd_old_proj);
	
	// enter 2D
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(2.f/screen->w, -2.f/screen->h, 1.f);
	glTranslatef(-screen->w*.5f, -screen->h*.5f, 0.f);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	
	char buf[256];
	glDisable(GL_ALPHA_TEST);
	
	F_render();
	
	strcpy(buf, "gIGen");
	glColor4f(0.f, 0.f, 0.f, 1.f);
	font_roman36->draw(buf, 1.f, 1.f);
	glColor4f(1.f, 1.f, 0.f, 1.f);
	font_roman36->draw(buf, 0.f, 0.f);
	
	if(gmode==GG_grd){
		lightData_t *data=mp->lightData;
		sprintf(buf, "Grid - %d/%d, %d/%d", cvertex0+1,data->w*data->h*data->d, cvertex1+1, data->w*data->h*data->d);
	}else{
		if(cphase==-1){
			sprintf(buf, "Shadow - Face %d/%d, %d/%d", cvertex0+1, m->count_face, cvertex1+1, m->count_face);
		}else{
			sprintf(buf, "Phase %d - Face %d/%d, %d/%d", cphase+1, cvertex0+1, m->count_face, cvertex1+1, m->count_face);
		}
	}
	glColor4f(0.f, 0.f, 0.f, 1.f);
	font_roman36->drawhalf(buf, 1.f, 37.f);
	glColor4f(1.f, 1.f, 1.f, 1.f);
	font_roman36->drawhalf(buf, 0.f, 36.f);
	
	sprintf(buf, "remain of phase: %d:%02d:%02d", remainTime/3600, (remainTime/60)%60, remainTime%60);
	glColor4f(0.f, 0.f, 0.f, 1.f);
	font_roman36->drawhalf(buf, 1.f, 37.f+18.f);
	glColor4f(1.f, 1.f, 1.f, 1.f);
	font_roman36->drawhalf(buf, 0.f, 36.f+18.f);
	if(dirLight)
		sprintf(buf, "Final Output [V to switch]");
	else
		sprintf(buf, "GI Only [V to switch]");
	glColor4f(0.f, 0.f, 0.f, 1.f);
	font_roman36->drawhalf(buf, 1.f, 37.f+36.f);
	glColor4f(1.f, 1.f, 1.f, 1.f);
	font_roman36->drawhalf(buf, 0.f, 36.f+36.f);
	
	SDL_GL_SwapBuffers();
}
static volatile bool saving=false;
void GG_genrate(gg_mode_t md){
	m=mp->m;
	gmode=md;
	
	if(md!=GG_grd){
		consoleLog("GG_genrate: allocating gi\n");
		m->gi=new vec3_t[m->count_face*subDiv*subDiv];
		consoleLog("GG_genrate: allocating giShadow\n");
		m->giShadow=new float[m->count_face*subDiv*subDiv];
		consoleLog("GG_genrate: allocating lit\n");
		m->lit=new vec3_t[m->count_face*subDiv*subDiv];
		if(md==GG_lit){
			consoleLog("GG_genrate: allocating lit1\n");
			m->lit1=new vec3_t[m->count_face*subDiv*subDiv];
			consoleLog("GG_genrate: allocating lit2\n");
			m->lit2=new vec3_t[m->count_face*subDiv*subDiv];
			consoleLog("GG_genrate: allocating lit3\n");
			m->lit3=new vec3_t[m->count_face*subDiv*subDiv];
			consoleLog("GG_genrate: allocating lit4\n");
			m->lit4=new vec3_t[m->count_face*subDiv*subDiv];
		}
		consoleLog("GG_genrate: allocating ogi\n");
		ogi=new vec3_t[m->count_face*subDiv*subDiv];
	}
	
	bound=aabb_t(m->vertex, m->count_vertex);
	
	
	
	int n;
	if(md!=GG_grd){
		consoleLog("GG_genrate: allocating rots\n");
		rots=new vec3_t[m->count_vertex];
		consoleLog("GG_genrate: allocating dLights\n");
		dLights=new vec3_t[m->count_face*subDiv*subDiv];
		m->giDiv=subDiv;
		for(n=0;n<m->count_face*subDiv*subDiv;n++){
			m->gi[n]=vec3_t(0.f, 0.f, 0.f);
			if(md==GG_lit){
			m->lit1[n]=vec3_t(0.f, 0.f, 0.f);
			m->lit2[n]=vec3_t(0.f, 0.f, 0.f);
			m->lit3[n]=vec3_t(0.f, 0.f, 0.f);
			m->lit4[n]=vec3_t(0.f, 0.f, 0.f);
			}
			m->giShadow[n]=0.f;
		}
	}else{
		aabb_t bound2;
		bound=bound2=aabb_t(mp->way->vertex, mp->way->count_vertex);
		bound.minvec=bound2.minvec-vec3_t(gridSize)*2.f;
		bound.maxvec=bound2.maxvec+vec3_t(gridSize)*2.f;
		consoleLog("GG_genrate: allocating grid\n");
		mp->lightData=new lightData_t((int)ceilf((bound.maxvec.x-bound.minvec.x)/gridSize),
									  (int)ceilf((bound.maxvec.y-bound.minvec.y)/gridSize),
									  (int)ceilf((bound.maxvec.z-bound.minvec.z)/gridSize));
		mp->lightData->minVec=bound.minvec;
		mp->lightData->maxVec=bound.maxvec;
		ogi=m->gi;
		dLights=m->lit;
	}
	
	consoleLog("GG_genrate: starting calculation\n");
	
	SDL_Thread *th;
	th=SDL_CreateThread(ggThread, NULL);
	
	SDL_Event event;
	
	consoleLog("GG_genrate: start mainloop\n");
	
	/* Check for events */
	while(1){
		while ( SDL_PollEvent(&event) ) {
			switch (event.type) {
					
				case SDL_MOUSEMOTION:
					cx=(float)event.motion.x/(float)screen->w;
					cy=-((float)event.motion.y/(float)screen->h*2.f-1.f);
					break;
				case SDL_MOUSEBUTTONDOWN:
					break;
				case SDL_KEYDOWN:
					if(event.key.keysym.sym==SDLK_ESCAPE)
						exit(1);
					if(event.key.keysym.sym=='v'){
						dirLight=!dirLight;
					}
					if(event.key.keysym.sym==SDLK_UP)
						cdist/=1.2f;
					if(event.key.keysym.sym==SDLK_DOWN)
						cdist*=1.2f;
					if(event.key.keysym.sym==SDLK_RETURN){
						while(saving);
						return;
					}
					break;
				case SDL_QUIT:
					exit(0);
					break;
				case SDL_VIDEORESIZE:
					//screen=SDL_SetVideoMode(event.resize.w,event.resize.h, video_bpp, videoflags);
					break;
				case SDL_USEREVENT:
					
					break;
				default:
					break;
			}
		}
		ggRender();
		SDL_Delay(20);
		if(cphase==2 || (cphase==0 && (gmode==GG_lit || gmode==GG_grd))){
			SDL_Delay(200);
			while(saving);
			return;
		}
	}
	
	
	
}

void GG_save(bool temporary){
	char fn[256];
	char fn2[256];
	int n;
	saving=true;
	
	if(gmode==GG_lit){
		sprintf(fn, "res/maps/%s.lit", mp->mapname);
		
		consoleLog("GG_save: opening %s for writing\n", fn);
		FILE *f=fopen(fn, "wb");
		if(!f){
			consoleLog("GG_genrate: cannot open %s for writing\n", fn);
			return;
		}
		
		fwrite(m->lit, sizeof(vec3_t), mp->m->count_face*(subDiv*subDiv), f);
		fwrite(m->lit1, sizeof(vec3_t), mp->m->count_face*(subDiv*subDiv), f);
		fwrite(m->lit2, sizeof(vec3_t), mp->m->count_face*(subDiv*subDiv), f);
		fwrite(m->lit3, sizeof(vec3_t), mp->m->count_face*(subDiv*subDiv), f);
		fwrite(m->lit4, sizeof(vec3_t), mp->m->count_face*(subDiv*subDiv), f);
		
		fclose(f);
		
		saving=false;
		return;
	}
	
	if(gmode==GG_grd){
		sprintf(fn, "res/maps/%s.grd", mp->mapname);
		
		consoleLog("GG_save: opening %s for writing\n", fn);
		FILE *f=fopen(fn, "wb");
		if(!f){
			consoleLog("GG_save: cannot open %s for writing\n", fn);
			return;
		}
		
		uint32_t vl=LIGHT_DATA_MAGIC;
		lightData_t *data=mp->lightData;
		fwrite(&vl, 4, 1, f);
		
		consoleLog("GG_save: scaled range: (%f, %f, %f)-(%f, %f, %f)\n", data->minVec.x, data->minVec.y, data->minVec.z,
				   data->maxVec.x, data->maxVec.y, data->maxVec.z);
		
		data->minVec/=mp->scale;
		data->maxVec/=mp->scale;
		
		consoleLog("GG_save: original range: (%f, %f, %f)-(%f, %f, %f)\n", data->minVec.x, data->minVec.y, data->minVec.z,
				   data->maxVec.x, data->maxVec.y, data->maxVec.z);
		
		fwrite(&data->minVec, 12, 1, f);
		fwrite(&data->maxVec, 12, 1, f);
		fwrite(&data->w, 4, 1, f);
		fwrite(&data->h, 4, 1, f);
		fwrite(&data->d, 4, 1, f);
		fwrite(data->data, sizeof(lightPoint_t), data->w*data->h*data->d, f);
	
		
		fclose(f);
		
		saving=false;
		return;
	}
	
	if(!temporary){
		sprintf(fn, "res/maps/.%s.bgi~", mp->mapname);
		struct stat s;
		if(!stat(fn, &s)){
			sprintf(fn2, "res/maps/%s.bgi", mp->mapname);
			remove(fn2);
			rename(fn, fn2);
			saving=false;
			return;
		}
	}
	
	if(!temporary)
		sprintf(fn, "res/maps/%s.bgi", mp->mapname);
	else
		sprintf(fn, "res/maps/.%s.bgi~", mp->mapname);
	
	consoleLog("GG_save: opening %s for writing\n", fn);
	FILE *f=fopen(fn, "wb");
	if(!f){
		consoleLog("GG_genrate: cannot open %s for writing\n", fn);
		return;
	}
	
	int32_t val;
	
	val=0xb1d11249;
	fwrite(&val, 4, 1, f);
	
	val=subDiv;
	fwrite(&val, 4, 1, f);
	
	val=mp->m->count_face;
	fwrite(&val, 4, 1, f);
	
	val=mp->m->count_face*(subDiv*subDiv)*2;
	fwrite(&val, 4, 1, f);
	
	
	
	for(n=0;n<mp->m->count_face*(subDiv*subDiv);n++){
		
		fwrite(&(m->gi[n]), 12, 1, f);
		fwrite(&(m->giShadow[n]), 4, 1, f);

	}
	val=0;
	for(n=0;n<mp->m->count_face*(subDiv*subDiv);n++){
		
		fwrite(&(m->lit[n]), 12, 1, f);
		fwrite(&val, 4, 1, f); // dummy
		
	}
	
	fclose(f);
	saving=false;
}

