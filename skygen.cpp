/*
 *  skygen.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 10/01/02.
 *  Copyright 2010 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "skygen.h"

const int sg_size=32;
const float sg_planetRadius=6378100.f;
const float sg_skyHeight=1000.f;
vec3_t sg_rayleigh(0.000139f, 0.000183f, 0.000314f);
vec3_t sg_mie;

static float sphereStick(vec3_t v1, vec3_t dir){
	vec3_t norm; 
	norm=vec3_t(dir.y, -dir.x, 0.f);
	
	float shift;
	shift=vec3_t::dot(v1, norm);
	//if(shift<0.f)shift=0.f;
	//if(shift>1.f)shift=1.f;
	return sqrt(1.-shift*shift);
}

static vec3_t traceSky(const map_t *mp, vec3_t dir){
	float dist;
	// calculate position
	dir=dir.normalize();
	vec3_t v2=vec3_t(vec3_t(dir.x, 0.f, dir.z).length(), dir.y, 0.f).normalize();
	dist=sphereStick(vec3_t(0.f, sg_planetRadius/(sg_planetRadius+sg_skyHeight), 0.f), v2)/
		sg_planetRadius*(sg_planetRadius+sg_skyHeight);
	//printf("%f,%f %f \n", v2.x, v2.y, dist);
	dist-=sphereStick(vec3_t(0.f, 1.f, 0.f), v2);
	dist*=sg_planetRadius;
	dist=(dist-sg_skyHeight)*10.f+sg_skyHeight;
	//dist=sg_skyHeight/dir.y;
	//printf("%f\n", dist);
	
	// calculate light scattering
	dvec3_t rr=sg_rayleigh;
	dvec3_t mm=sg_mie;
	double gg=(.8);	
	dvec3_t ee=vec3_t(30.);		// irradiance
	dvec3_t r, m, lin, fex;
	double cosSun=dvec3_t::dot(dir, mp->sunPos.normalize());
	;
	r=rr*0.954929658551372/16.;
	r*=1.f+max(cosSun, 0.)*max(cosSun, 0.);
	
	m=mm/12.566370614359173;
	m*=(1.-gg)*(1.-gg);
	m*=pow(1.+gg*gg-2.*gg*cosSun, -1.5);
	
	fex.x=exp(-dist*(rr.x+mm.x));
	fex.y=exp(-dist*(rr.y+mm.y));
	fex.z=exp(-dist*(rr.z+mm.z));
	
	lin=(r+m)/(rr+mm);
	lin*=ee;
	lin*=(dvec3_t(1.)-fex);
	//lin=vec3_t(dist/200.f-12.f);
	//lin=.5f+v2.y*.5f;
	return (vec3_t)lin*.3f*mp->sunColor;
}

void SG_generateSkyBox(const map_t *mp){
	
	int x, y, ptr=0;
	vec3_t dir; vec3_t col;
	float ang;
	
	// allocate buffer
	float *buf=new float[sg_size*sg_size*4];
	
	// calculate mie coeff
	sg_mie=vec3_t(.1f/mp->fog);
	
	// generate skybox
	for(y=0;y<sg_size;y++){
		for(x=0;x<sg_size;x++){
			ang=(float)x/(float)sg_size*M_PI*2.f;
			dir.x=sinf(ang); dir.z=cosf(ang);
			ang=(float)(sg_size-y)/(float)sg_size*M_PI*.5f;
			dir.x*=cosf(ang); dir.z*=cosf(ang);
			dir.y=sinf(ang);
			
			col=traceSky(mp, dir);
			
			buf[ptr++]=col.x;
			buf[ptr++]=col.y;
			buf[ptr++]=col.z;
			buf[ptr++]=1.f;
		}
	}
	
	// upload
#if GL_ARB_texture_float
	if(use_hdr)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, sg_size, sg_size, 0, GL_RGBA, GL_FLOAT, buf);
	else
#endif
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sg_size, sg_size, 0, GL_RGBA, GL_FLOAT, buf);

	delete[] buf;
	
}

void SG_generateSkyBoxUp(const map_t *mp){
	
	int x, y, ptr=0;
	vec3_t dir; vec3_t col;
	float ang;
	
	// allocate buffer
	float *buf=new float[sg_size*sg_size*4];
	
	// calculate mie coeff
	sg_mie=vec3_t(.1f/mp->fog);
	
	// generate skybox
	for(y=0;y<sg_size;y++){
		for(x=0;x<sg_size;x++){
			float sq;
			dir.x=1.f-(float)x/(float)sg_size*2.f; dir.y=0.f;
			dir.z=(float)y/(float)sg_size*2.f-1.f;
			sq=dir.length_2();
			if(sq>=1.f){
				dir/=sqrtf(sq);
				sq=1.f;
			}
			dir.y=sqrtf(1.f-sq);
			
			col=traceSky(mp, dir);
			
			buf[ptr++]=col.x;
			buf[ptr++]=col.y;
			buf[ptr++]=col.z;
			buf[ptr++]=1.f;
		}
	}
	
	// upload
#if GL_ARB_texture_float
	if(use_hdr)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, sg_size, sg_size, 0, GL_RGBA, GL_FLOAT, buf);
	else
#endif
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sg_size, sg_size, 0, GL_RGBA, GL_FLOAT, buf);

	delete[] buf;
	
}


