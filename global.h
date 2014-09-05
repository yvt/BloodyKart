/*
 *  global.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/23.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#ifndef _BK_GLOBAL_H
#define _BK_GLOBAL_H

#ifdef _OPENMP
#include <omp.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include <string>
#include <list>
#ifdef WIN32
#include <malloc.h>
#endif

using namespace std;


//#define GL_EXT_histogram 1

#ifdef __MACOSX__
#include "OpenGL/gl.h"
#include "OpenGL/glu.h"
#include "OpenAL/al.h"
#include "GLUT/glut.h"
#include "SDL.h"
#include "SDL_net.h"
#else
#ifdef WIN32
#define GLEW_STATIC
#include <GL/glew.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <SDL/SDL.h>
#include <SDL/SDL_net.h>
//#include <GL/glut.h>
#endif

//#undef GL_ARB_multitexture
//#undef GL_ARB_shader_objects



#ifndef GL_HALF_FLOAT_ARB
#define GL_HALF_FLOAT_ARB 0x140b
#endif

extern char gl_ext[];

extern bool cap_multiTex;
extern bool cap_glsl;
extern bool cap_toneMap;

extern bool use_glsl;
extern bool use_hdr;
extern bool use_toneMap;
extern int multiSamples;
extern int superSamples;



#if GL_EXT_framebuffer_object
extern GLuint fb_main;
extern GLuint tex_fb;
extern GLuint tex_fbdepth;
extern GLuint fb_down; // for downsampling of multisample
extern GLuint tex_fbdown;
extern GLuint tex_fbdowndepth;
//extern GLuint tex_fbdowndepth;
#endif

#define TEXTURE_FILTER GL_LINEAR_MIPMAP_LINEAR


static inline float rsqrtf( float number )
{
	union {
		float f;
		int i;
	} t;
	float x2, y;
	const float threehalfs = 1.5F;
	
	x2 = number * 0.5F;
	t.f  = number;
	t.i  = 0x5f3759df - ( t.i >> 1 );               // what the fuck?
	y  = t.f;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
	//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed
	
	return y;
}

struct vec3_t{
	float x, y, z;
	vec3_t(){
		x=0;y=0;z=0;
	}
	vec3_t(float v){
		x=y=z=v;
	}
	vec3_t(float xx, float yy, float zz){
		x=xx; y=yy; z=zz;
	}
	vec3_t(float xx, float yy){
		x=xx; y=yy;
	}
	vec3_t operator +(const vec3_t& o) const{
		return vec3_t(x+o.x, y+o.y, z+o.z);
	}
	vec3_t operator -(const vec3_t& o) const{
		return vec3_t(x-o.x, y-o.y, z-o.z);
	}
	vec3_t operator -() const{
		return vec3_t(-x, -y, -z);
	}
	vec3_t operator *(const vec3_t& o) const{
		return vec3_t(x*o.x, y*o.y, z*o.z);
	}
	vec3_t operator /(const vec3_t& o) const{
		return vec3_t(x/o.x, y/o.y, z/o.z);
	}
	void operator +=(const vec3_t& o){
		x+=o.x; y+=o.y; z+=o.z;
	}
	void operator -=(const vec3_t& o){
		x-=o.x; y-=o.y; z-=o.z;
	}
	void operator *=(const vec3_t& o){
		x*=o.x; y*=o.y; z*=o.z;
	}
	void operator /=(const vec3_t& o){
		x/=o.x; y/=o.y; z/=o.z;
	}
	bool operator ==(const vec3_t& o) const{
		return x==o.x && y==o.y && z==o.z;
	}
	float distance() const{
		return sqrtf(x*x+y*y+z*z);
	}
	float distance_2() const{
		return x*x+y*y+z*z;
	}
	float rlength() const{
		return rsqrtf(x*x+y*y+z*z);
	}
	float length() const{
		return sqrtf(x*x+y*y+z*z);
	}
	float length_2() const{
		return x*x+y*y+z*z;
	}
	static float dot(vec3_t a, vec3_t b){
		return a.x*b.x+a.y*b.y+a.z*b.z;
	}
	static vec3_t cross(vec3_t a, vec3_t b){
		return vec3_t(a.y*b.z-a.z*b.y,
					  a.z*b.x-a.x*b.z,
					  a.x*b.y-a.y*b.x);
	}
	vec3_t normalize() const{
		return *this*rlength();
	}
	static vec3_t normal(vec3_t a, vec3_t b, vec3_t c){
		vec3_t ab=b-a;
		vec3_t ac=c-a;
		return cross(ab, ac).normalize();
	}
};


struct dvec3_t{
	double x, y, z;
	dvec3_t(){
		x=0;y=0;z=0;
	}
	dvec3_t(double v){
		x=y=z=v;
	}
	dvec3_t(double xx, double yy, double zz){
		x=xx; y=yy; z=zz;
	}
	dvec3_t(double xx, double yy){
		x=xx; y=yy;
	}
	dvec3_t(vec3_t v){
		x=v.x; y=v.y; z=v.z;
	}
	dvec3_t operator +(const dvec3_t& o) const{
		return dvec3_t(x+o.x, y+o.y, z+o.z);
	}
	dvec3_t operator -(const dvec3_t& o) const{
		return dvec3_t(x-o.x, y-o.y, z-o.z);
	}
	dvec3_t operator -() const{
		return dvec3_t(-x, -y, -z);
	}
	dvec3_t operator *(const dvec3_t& o) const{
		return dvec3_t(x*o.x, y*o.y, z*o.z);
	}
	dvec3_t operator /(const dvec3_t& o) const{
		return dvec3_t(x/o.x, y/o.y, z/o.z);
	}
	void operator +=(const dvec3_t& o){
		x+=o.x; y+=o.y; z+=o.z;
	}
	void operator -=(const dvec3_t& o){
		x-=o.x; y-=o.y; z-=o.z;
	}
	void operator *=(const dvec3_t& o){
		x*=o.x; y*=o.y; z*=o.z;
	}
	void operator /=(const dvec3_t& o){
		x/=o.x; y/=o.y; z/=o.z;
	}
	bool operator ==(const dvec3_t& o) const{
		return x==o.x && y==o.y && z==o.z;
	}
	double distance() const{
		return sqrt(x*x+y*y+z*z);
	}
	double distance_2() const{
		return x*x+y*y+z*z;
	}
	double rlength() const{
		return 1./sqrt(x*x+y*y+z*z);
	}
	double length() const{
		return sqrt(x*x+y*y+z*z);
	}
	double length_2() const{
		return x*x+y*y+z*z;
	}
	static double dot(dvec3_t a, dvec3_t b){
		return a.x*b.x+a.y*b.y+a.z*b.z;
	}
	static dvec3_t cross(dvec3_t a, dvec3_t b){
		return dvec3_t(a.y*b.z-a.z*b.y,
					  a.z*b.x-a.x*b.z,
					  a.x*b.y-a.y*b.x);
	}
	dvec3_t normalize() const{
		return *this*rlength();
	}
	static dvec3_t normal(dvec3_t a, dvec3_t b, dvec3_t c){
		dvec3_t ab=b-a;
		dvec3_t ac=c-a;
		return cross(ab, ac).normalize();
	}
	operator vec3_t() const{
		return vec3_t(x, y, z);
	}
	
};

struct plane_t{
	
	vec3_t n;
	float w;
	
	plane_t(){}
	plane_t(float xx, float yy, float zz, float ww){
		n=vec3_t(xx, yy, zz); w=ww;
	}
	plane_t(vec3_t v1, vec3_t v2, vec3_t v3){
		n=vec3_t::normal(v1, v2, v3);
		w=-vec3_t::dot(n, v1);
	}
	float distance(vec3_t vec) const{
		return vec3_t::dot(vec, n)+w;
	}
	vec3_t project(vec3_t vec) const{
		return vec-n*distance(vec);
	}
	plane_t toward(vec3_t vec) const{
		if(distance(vec)<0.f)
			return -*this;
		else
			return *this;
	}
	plane_t operator -() const{
		return plane_t(-n.x, -n.y, -n.z, -w);
	}
};

struct aabb_t{
	vec3_t minvec, maxvec;
	bool exist;
	
	aabb_t(){exist=false;}
	
	aabb_t(vec3_t v){
		exist=false;
		add(v);
	}
	aabb_t(vec3_t v1, vec3_t v2){
		exist=true;
		minvec.x=min(v1.x, v2.x);
		minvec.y=min(v1.y, v2.y);
		minvec.z=min(v1.z, v2.z);
		maxvec.x=max(v1.x, v2.x);
		maxvec.y=max(v1.y, v2.y);
		maxvec.z=max(v1.z, v2.z);
	}
	aabb_t(vec3_t v1, vec3_t v2, vec3_t v3){
		exist=true;
		minvec.x=min(min(v1.x, v2.x), v3.x);
		minvec.y=min(min(v1.y, v2.y), v3.y);
		minvec.z=min(min(v1.z, v2.z), v3.z);
		maxvec.x=max(max(v1.x, v2.x), v3.x);
		maxvec.y=max(max(v1.y, v2.y), v3.y);
		maxvec.z=max(max(v1.z, v2.z), v3.z);
	}
	aabb_t(vec3_t *vs, int cnt){
		if(cnt){
			exist=true;
			while(cnt--){
				add(*vs);
				vs++;
			}
		}else{
			exist=false;
		}
	}
	
	void add(vec3_t v){
		if(!exist){
			minvec=v; maxvec=v;
			exist=true;
		}else{
			if(v.x<minvec.x)
				minvec.x=v.x;
			if(v.y<minvec.y)
				minvec.y=v.y;
			if(v.z<minvec.z)
				minvec.z=v.z;
			if(v.x>maxvec.x)
				maxvec.x=v.x;
			if(v.y>maxvec.y)
				maxvec.y=v.y;
			if(v.z>maxvec.z)
				maxvec.z=v.z;
		}
	}
	
	void operator +=(const vec3_t& v){
		add(v);
	}
	
	aabb_t operator +(const vec3_t& v) const{
		aabb_t t=*this;
		t+=v;
		return t;
	}
	
	bool operator &&(const aabb_t& b) const{
		return maxvec.x>b.minvec.x && maxvec.y>b.minvec.y && maxvec.z>b.minvec.z &&
		minvec.x<b.maxvec.x && minvec.y<b.maxvec.y && minvec.z<b.maxvec.z;
	}
	
};

void identityMatrix4(float *);
void transposeMatrix4(float *, float *);

struct quaternion_t{
	float w, x, y, z;
	quaternion_t(){}
	quaternion_t(float ww, float xx, float yy, float zz){
		w=ww; x=xx; y=yy; z=zz;
	}
	quaternion_t operator +(const quaternion_t& q) const{
		return quaternion_t(w+q.w, x+q.w, y+q.y, z+q.z);
	}
	quaternion_t operator -(const quaternion_t& q) const{
		return quaternion_t(w-q.w, x-q.w, y-q.y, z-q.z);
	}
	quaternion_t operator *(const quaternion_t& q) const{
		return quaternion_t(w*q.w-x*q.x-y*q.y-z*q.z,
							w*q.x+x*q.w+y*q.z-y*q.y,
							w*q.y+y*q.w-x*q.z+z*q.x,
							w*q.z+z*q.w+x*q.y-y*q.x);
	}
	quaternion_t operator *(float v) const{
		return quaternion_t(w*v, x*v, y*v, z*v);
	};
	void toMatrix4(float *m){
		identityMatrix4(m);
		m[0]=1.f-2.f*(y*y+z*z);
		m[5]=1.f-2.f*(x*x+z*z);
		m[10]=1.f-2.f*(x*x+y*y);
		m[1]=2.f*(x*y+z*w);
		m[2]=2.f*(x*z-y*w);
		m[4]=2.f*(x*y-z*w);
		m[6]=2.f*(w*z+x*w);
		m[8]=2.f*(x*z+y*w);
		m[9]=2.f*(y*z-x*w);
		//transposeMatrix4(m, m);
	}
	static float dot(const quaternion_t& q1, const quaternion_t& q2){
		return q1.w*q2.w+q1.x*q2.x+q1.y*q2.y+q1.z*q2.z;
	}
	static quaternion_t slerp(float d, const quaternion_t& q1, const quaternion_t& q2){
		float dt=dot(q1, q2);
		if(dt>=.99999f)
			return q1; // no difference
		float ph=acosf(dt);
		float s1, s2;
		if(dt<0.f && ph>=M_PI*.5f){
			ph=acosf(-dt);
			s1= sinf(ph*(1.f-d))/sinf(ph);
			s2=-sinf(ph*     d )/sinf(ph);
		}else{
			s1= sinf(ph*(1.f-d))/sinf(ph);
			s2= sinf(ph*     d )/sinf(ph);
		}
		return q1*s1+q2*s1;
	}
};



typedef vec3_t VECTOR;

// geometry functions

void rotate(float& x, float& y, float ang);
vec3_t rotate(vec3_t vec, vec3_t ang);
vec3_t unrotate(vec3_t vec, vec3_t ang);	// inversed rotating
vec3_t unrotate(vec3_t vec);				// get ang
float lineDist(vec3_t v1, vec3_t v2, vec3_t vv);
float lineDist2(vec3_t v1, vec3_t v2, vec3_t vv);
void inverseMatrix4(float *matrix);
void identityMatrix4(float *matrix);
void transposeMatrix4(float *dest, float *src);
void multMatrix4(float *dest, const float *mat1, const float *mat2);
void translateMatrix4(float *dest, vec3_t v);
void scaleMatrix4(float *desc, vec3_t v);
void copyMatrix4(float *dest, float *src);
#define fromAngle(ang)	rotate(vec3_t(0.f, 0.f, 1.f), ang)

vec3_t parse_vec3(const char *);
enum hit_dest_t{
	HD_client,
	HD_map
};
struct raycast_t{
	hit_dest_t dest;
	vec3_t pos;
	int cli; //client
	float dist;
	int poly; //map
	bool operator <(const raycast_t& r) const{
		return dist<r.dist;
	}
};
void raycast(vec3_t v1, vec3_t v2, vector<raycast_t>&);
bool traceSphere(vec3_t v1, vec3_t v2, vec3_t vv, float radius, raycast_t& res);
void splashDamage(vec3_t center, float range, float damage, int by); // for server!

// render functions

// drawRay - renders ray, u1 is v1's side, u2 is v2's side
//           needs to have done glBegin(GL_QUADS)
void drawRay(vec3_t v1, vec3_t v2, float radius, float u1, float v1, float u2, float v2);
void osdEnter2D();
void osdLeave2D();

// calculation

float *set_color4f(float *buf, float r, float g, float b, float a);
#if defined( WIN32 ) || 1
#define color4f(r, g, b, a)		set_color4f(NULL, r, g, b, a)
#else
#define color4f(r, g, b, a)		set_color4f((float *)alloca(sizeof(float)*4), r, g, b, a)
#endif

int larger_pow2(int base);

// gl functions

#if GL_ARB_shader_objects
GLhandleARB create_program(const char *, const char *);
#else
// create_program doesn't exist without glsl!
#endif

void clearTexture(int w, int h);

static bool operator ==(const IPaddress& addr1, const IPaddress& addr2){
	return addr1.host==addr2.host && addr1.port==addr2.port;
}
static bool operator !=(const IPaddress& addr1, const IPaddress& addr2){
	return addr1.host!=addr2.host || addr1.port!=addr2.port;
}

void consoleLog(const char *format, ...);

void show_msg(const char *str); // client
void show_fraggedMsg(const char *str); // client, implemented in main.cpp

const char *getVersionString();

// Msg

#include "font.h"

struct msg_t{
	char text[256];
	Uint32 time;
};
extern list<msg_t> msgs;
extern Uint32 omsganim;
static const int msg_anim=250;
static const int msg_time=4000;

extern SDL_Surface *screen;

extern font_t *font_roman36;
extern vec3_t camera_from, camera_at;
extern vec3_t avgVel;

extern float screenKick;
extern bool evenFrame;


extern float oang;

extern bool g_blood;
extern float g_scoreRankTime;
extern float g_scoreFadeTime;
extern bool g_backMirror;
extern bool g_currentlyRenderingBack;
extern bool g_useStereo;
extern float g_stereoPos;

extern double osd_old_modelview[16];
extern double osd_old_proj[16];

extern unsigned int totalPolys;

static inline float rnd(){
	return (float)rand()/(float)RAND_MAX;
}

struct score_client_t{
	bool enable;
	float fade;
	float pos;
	float correctPos;
};

extern score_client_t scli[];

#endif



