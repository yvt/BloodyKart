/*
 *  global.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/23.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */
#include "global.h"
#include "map.h"
#include "client.h"



bool cap_multiTex=false;
bool cap_glsl=false; // ability
bool cap_toneMap=false;

bool use_glsl=false; // enable
bool use_hdr=false;
bool use_toneMap=false;
int multiSamples=1;
int superSamples=1;

bool g_useStereo=false;
float g_stereoPos=0.f;

#if GL_EXT_framebuffer_object
GLuint fb_main;
GLuint fb_down;
GLuint tex_fb;
GLuint tex_fbdepth;
GLuint tex_fbdown;
GLuint tex_fbdowndepth;
#endif

bool g_blood=false;
float g_scoreRankTime=.2f;
float g_scoreFadeTime=.2f;
bool g_backMirror=true;
bool g_currentlyRenderingBack;
bool evenFrame=false;
unsigned int totalPolys;
float lastDt=1.f;

double osd_old_modelview[16];
double osd_old_proj[16];

void rotate(float& x, float& y, float ang){
	float cx=x, cy=y;
	float c=cosf(ang), s=sinf(ang);
	x=cx*c-cy*s;
	y=cx*s+cy*c;
}
vec3_t rotate(vec3_t vec, vec3_t ang){
	rotate(vec.x, vec.y, -ang.z);
	rotate(vec.z, vec.y, ang.x);
	rotate(vec.x, vec.z, ang.y);
	return vec;
}
vec3_t unrotate(vec3_t vec, vec3_t ang){
	rotate(vec.x, vec.z, -ang.y);
	rotate(vec.z, vec.y, -ang.x);
	rotate(vec.x, vec.y, ang.z);
	return vec;
}
vec3_t unrotate(vec3_t vec){
	vec3_t ang;
	ang.y=atan2f(-vec.x, vec.z);
	ang.x=atan2f(vec.y, sqrtf(vec.x*vec.x+vec.z*vec.z));
	ang.z=0.f; // unknown
	return ang;
}
#define MADDR(x, y)		((x)+((y)<<2))
void inverseMatrix4(float *matrix){
	float det=1.f;
	float t, u;
	int k, j, i;
	for(k=0;k<4;k++){
		t=matrix[MADDR(k, k)];
		det*=t;
		for(i=0;i<4;i++)
			matrix[MADDR(k, i)]/=t;
		matrix[MADDR(k, k)]=1.f/t;
		for(j=0;j<4;j++){
			if(j==k)
				continue;
			u=matrix[MADDR(j, k)];
			for(i=0;i<4;i++)
				if(i!=k)
					matrix[MADDR(j, i)]-=matrix[MADDR(k, i)]*u;
				else
					matrix[MADDR(j, i)]=-u/t;
		}
	}
	
}
void identityMatrix4(float *dest){
	dest[0]=1.f; dest[4]=0.f; dest[8]=0.f;  dest[12]=0.f;
	dest[1]=0.f; dest[5]=1.f; dest[9]=0.f;  dest[13]=0.f; 
	dest[2]=0.f; dest[6]=0.f; dest[10]=1.f; dest[14]=0.f;
	dest[3]=0.f; dest[7]=0.f; dest[11]=0.f; dest[15]=1.f;
}
void translateMatrix4(float *dest, vec3_t v){
	dest[0]=1.f; dest[4]=0.f; dest[8]=0.f;  dest[12]=v.x;
	dest[1]=0.f; dest[5]=1.f; dest[9]=0.f;  dest[13]=v.y; 
	dest[2]=0.f; dest[6]=0.f; dest[10]=1.f; dest[14]=v.z;
	dest[3]=0.f; dest[7]=0.f; dest[11]=0.f; dest[15]=1.f;
}
void scaleMatrix4(float *dest, vec3_t v){
	dest[0]=v.x; dest[4]=0.f; dest[8]=0.f;  dest[12]=0.f;
	dest[1]=0.f; dest[5]=v.y; dest[9]=0.f;  dest[13]=0.f; 
	dest[2]=0.f; dest[6]=0.f; dest[10]=v.z; dest[14]=0.f;
	dest[3]=0.f; dest[7]=0.f; dest[11]=0.f; dest[15]=1.f;
}
void transposeMatrix4(float *to, float *from){
	if(to!=from){
		to[0] = from[0];
		to[1] = from[4];
		to[2] = from[8];
		to[3] = from[12];
		to[4] = from[1];
		to[5] = from[5];
		to[6] = from[9];
		to[7] = from[13];
		to[8] = from[2];
		to[9] = from[6];
		to[10] = from[10];
		to[11] = from[14];
		to[12] = from[3];
		to[13] = from[7];
		to[14] = from[11];
		to[15] = from[15];
	}else{
		swap(to[1], to[4]);
		swap(to[2], to[8]);
		swap(to[3], to[12]);
		swap(to[6], to[9]);
		swap(to[7], to[13]);
		swap(to[11], to[14]);
	}
}
void multMatrix4(float *dest, const float *mat1, const float *mat2){
	dest[0]=mat1[0]*mat2[0]+mat1[4]*mat2[1]+mat1[8]*mat2[2]+mat1[12]*mat2[3];
	dest[4]=mat1[0]*mat2[4]+mat1[4]*mat2[5]+mat1[8]*mat2[6]+mat1[12]*mat2[7];
	dest[8]=mat1[0]*mat2[8]+mat1[4]*mat2[9]+mat1[8]*mat2[10]+mat1[12]*mat2[11];
	dest[12]=mat1[0]*mat2[12]+mat1[4]*mat2[13]+mat1[8]*mat2[14]+mat1[12]*mat2[15];
	
	dest[1]=mat1[1]*mat2[0]+mat1[5]*mat2[1]+mat1[9]*mat2[2]+mat1[13]*mat2[3];
	dest[5]=mat1[1]*mat2[4]+mat1[5]*mat2[5]+mat1[9]*mat2[6]+mat1[13]*mat2[7];
	dest[9]=mat1[1]*mat2[8]+mat1[5]*mat2[9]+mat1[9]*mat2[10]+mat1[13]*mat2[11];
	dest[13]=mat1[1]*mat2[12]+mat1[5]*mat2[13]+mat1[9]*mat2[14]+mat1[13]*mat2[15];
	
	dest[2]=mat1[2]*mat2[0]+mat1[6]*mat2[1]+mat1[10]*mat2[2]+mat1[14]*mat2[3];
	dest[6]=mat1[2]*mat2[4]+mat1[6]*mat2[5]+mat1[10]*mat2[6]+mat1[14]*mat2[7];
	dest[10]=mat1[2]*mat2[8]+mat1[6]*mat2[9]+mat1[10]*mat2[10]+mat1[14]*mat2[11];
	dest[14]=mat1[2]*mat2[12]+mat1[6]*mat2[13]+mat1[10]*mat2[14]+mat1[14]*mat2[15];
	
	dest[3]=mat1[3]*mat2[0]+mat1[7]*mat2[1]+mat1[11]*mat2[2]+mat1[15]*mat2[3];
	dest[7]=mat1[3]*mat2[4]+mat1[7]*mat2[5]+mat1[11]*mat2[6]+mat1[15]*mat2[7];
	dest[11]=mat1[3]*mat2[8]+mat1[7]*mat2[9]+mat1[11]*mat2[10]+mat1[15]*mat2[11];
	dest[15]=mat1[3]*mat2[12]+mat1[7]*mat2[13]+mat1[11]*mat2[14]+mat1[15]*mat2[15];
}
void rotateMatrix4(float *dest, float x, float y, float z, float w){
	identityMatrix4(dest);
	dest[0]=1.f-2.f*(y*y+z*z);	dest[4]=2.f*(x*y-z*w);		dest[8]=2.f*(x*z+y*w);
	dest[1]=2.f*(x*y+z*w);		dest[5]=1.f-2.f*(x*x+z*z);	dest[9]=2.f*(y*z-x*w);
	dest[2]=2.f*(x*z-y*w);		dest[6]=2.f*(y*z+x*w);		dest[10]=1.f-2.f*(x*x+y*y);
	transposeMatrix4(dest, dest);
}
void copyMatrix4(float *dest, float *src){
	memcpy(dest, src, sizeof(float)*16);
}
#undef MADDR

static float color4fBuf[64][4];
static int color4fInd=0;

float *set_color4f(float *buf, float r, float g, float b, float a){
	if(buf==NULL){
		buf=color4fBuf[color4fInd];
		color4fInd=(color4fInd+1)&63;
	}
	buf[0]=r; buf[1]=g; buf[2]=b; buf[3]=a;
	return buf;
}

vec3_t parse_vec3(const char * str){
	float elm[3];
	char buf[256];
	strcpy(buf, str);
	int bgn;
	int n, i=0;
	bgn=0;
	for(n=0;buf[n];n++){
		if(buf[n]==','){
			buf[n]=0;
			elm[i++]=atof(buf+bgn);
			bgn=n+1;
		}else if(buf[n]==' ' && bgn==n){
			bgn++;
		}
	}
	elm[i++]=atof(buf+bgn);
	return vec3_t(elm[0], elm[1], elm[2]);
}
#if GL_ARB_shader_objects

static GLhandleARB sh_global_vs=NULL;
static GLhandleARB sh_global_fs=NULL;


static void shader_program(GLhandleARB sh, const char *fn, const char *fn2=NULL){
	FILE *f;
	string str, str2;
	str="";
	char buf[2048];
	f=fopen(fn, "r");
	if(f==NULL)
		throw "Can't open shader";
	while(fgets(buf, 1024, f)){
		str+=string(buf);
	}
	fclose(f);
	if(fn2){
		
		f=fopen(fn2, "r");
		if(f==NULL)
			throw "Can't open shader2";
		while(fgets(buf, 1024, f)){
			str+=string(buf);
		}
		fclose(f);
		
	}
	const char *buf2=str.c_str();
	glShaderSourceARB(sh, 1, &buf2, NULL);
}

GLhandleARB create_program(const char *vs, const char *fs){
	
	if(!cap_glsl)
		return NULL;
	
	GLhandleARB sh1, sh2;
	GLhandleARB gsh1, gsh2;
	GLhandleARB prg;
	char buf[2048];
	GLsizei len;
	GLint ret;
	
	glGetError();
	
	if(sh_global_vs==NULL){
		gsh1=glCreateShaderObjectARB(GL_VERTEX_SHADER);
		shader_program(gsh1, "res/shaders/global.vs");
		glCompileShaderARB(gsh1);
	}
	
	/*
	 this crashes in Snow Leopard
	if(sh_global_fs==NULL){
		gsh2=glCreateShaderObjectARB(GL_FRAGMENT_SHADER);
		shader_program(gsh2, "res/shaders/global.fs");
		glCompileShaderARB(gsh2);
	}*/
	
	sh1=glCreateShaderObjectARB(GL_VERTEX_SHADER);
	shader_program(sh1, vs);
	glCompileShaderARB(sh1);
	glGetObjectParameterivARB(sh1, GL_OBJECT_COMPILE_STATUS_ARB, &ret);
	
	if(glGetError()!=GL_NO_ERROR || ret==GL_FALSE){
		consoleLog("create_program: vertex shader compiliation error occured. info log:\n");
		glGetInfoLogARB(sh1, 2048, &len, buf);
		buf[len]=0;
		consoleLog("%s\n", buf);
		return NULL;
	}
	
	sh2=glCreateShaderObjectARB(GL_FRAGMENT_SHADER);
	shader_program(sh2, fs, "res/shaders/global.fs");
	glCompileShaderARB(sh2);
	glGetObjectParameterivARB(sh1, GL_OBJECT_COMPILE_STATUS_ARB, &ret);
	
	if(glGetError()!=GL_NO_ERROR || ret==GL_FALSE){
		consoleLog("create_program: fragmenu shader compiliation error occured. info log:\n");
		glGetInfoLogARB(sh2, 2048, &len, buf);
		buf[len]=0;
		consoleLog("%s\n", buf);
		return NULL;
	}
	
	prg=glCreateProgramObjectARB();
	glAttachObjectARB(prg, sh1);
	glAttachObjectARB(prg, sh2);
	glAttachObjectARB(prg, gsh1);
	//glAttachObjectARB(prg, gsh2);
	
	glLinkProgramARB(prg);
	
	glGetObjectParameterivARB(prg, GL_OBJECT_LINK_STATUS_ARB, &ret);
	if(glGetError()!=GL_NO_ERROR || ret==GL_FALSE){
		consoleLog("create_program: linking error occured. info log:\n");
		glGetInfoLogARB(prg, 2048, &len, buf);
		buf[len]=0;
		consoleLog("%s\n", buf);
		return NULL;
	}
	return prg;
}

#endif
int larger_pow2(int base){
	int i;
	for(i=1;i<base;i<<=1);
	return i;
}

list<msg_t> msgs;
Uint32 omsganim=0;

SDL_Surface *screen;

font_t *font_roman36;
vec3_t camera_from, camera_at;

float oang=0.f;

void show_msg(const char *str){
	msg_t msg;
	strcpy(msg.text, str);
	msg.time=SDL_GetTicks();
	msgs.push_back(msg);
	if(SDL_GetTicks()>omsganim){
		omsganim=SDL_GetTicks()+msg_anim;
	}else{
		omsganim+=msg_anim;
	}
	consoleLog("show_msg: \"%s\"\n", str);
}

void raycast(vec3_t v1, vec3_t v2, vector<raycast_t>& res){
	raycast_t r;
	vec3_t dir;
	dir=(v2-v1).normalize();
	// map
	mesh *m=mp->m;
	vector<isect_t> *iss;
	vector<isect_t>::iterator it;
	
	iss=m->raycast(v1, v2);
	r.dest=HD_map;
	for(it=iss->begin();it!=iss->end();it++){
		r.pos=v1+dir*it->dist;
		r.dist=it->dist;
		r.poly=it->face;
		res.push_back(r);
	}
	delete iss;
	
	// clients
	int n;
	for(n=0;n<MAX_CLIENTS;n++){
		if(!cli[n].enable)
			continue;
		if(!cli[n].is_avail())
			continue;
		raycast_t rc;
		if(traceSphere(v1, v2, cli[n].pos, .8f, rc)){
			rc.dest=HD_client;
			rc.cli=n;
			res.push_back(rc);
		}
	}
	
}

bool traceSphere(vec3_t v1, vec3_t v2, vec3_t vv, float radius, raycast_t& res){
	if((v1-vv).length_2()<radius*radius){
		res.pos=v1; res.dist=-1.f;
		return true;
	}
	vec3_t dir, vv1, vv2;
	float length, l1, l2;
	dir=v2-v1;
	length=dir.length();
	dir*=1.f/length;
	//
	
	/*vec3_t proj=v1+dir*vec3_t::dot(vv-v1, dir);
	res.pos=proj;
	res.dist=(proj-v1).length();
	return true;*/
	
	l1=lineDist2(v1, v2, vv);
	vv1=v2-vv;
	l2=vv1.length_2();
	if(l1>=radius*radius && l2>=(radius+.00125f)*(radius+.00125f)){
		return false;
	}
	
	
	vv1=v1-vv;
	float a, b, c, d;
	a=1.f;
	b=2.f*vec3_t::dot(dir, vv1);
	c=vv1.length_2()-(radius+.00125f)*(radius+.00125f);
	d=b*b-4.f*c;
	
	if(d>0.f){
		float sqrtd=sqrtf(d);
		float dist;
		dist=(-b-sqrtd)*.5f;
		if(dist<0.f)
			dist=0.f;
		res.dist=dist;
		res.pos=v1+dir*dist;
		return true;
	}
	
	return false;
}

float lineDist(vec3_t v1, vec3_t v2, vec3_t vv){
	if(vec3_t::dot(vv-v1,v2-v1)<=0.f){
		return (vv-v1).length();
	}
	if(vec3_t::dot(vv-v2,v1-v2)<=0.f){
		return (vv-v2).length();
	}
	
	vec3_t dir=(v2-v1).normalize();
	vec3_t proj=v1+dir*vec3_t::dot(vv-v1, dir);
	
	return (vv-proj).length();
}

float lineDist2(vec3_t v1, vec3_t v2, vec3_t vv){
	if(vec3_t::dot(vv-v1,v2-v1)<=0.f){
		return (vv-v1).length_2();
	}
	if(vec3_t::dot(vv-v2,v1-v2)<=0.f){
		return (vv-v2).length_2();
	}
	
	vec3_t dir=(v2-v1).normalize();
	vec3_t proj=v1+dir*vec3_t::dot(vv-v1, dir);
	
	return (vv-proj).length_2();
}

void clearTexture(int w, int h){
	uint16_t *ptr=new uint16_t[w*h];
	memset(ptr, 0, w*h*2);
	glTexSubImage2D(GL_TEXTURE_2D, 0,  0, 0, w, h,  GL_LUMINANCE_ALPHA,
					GL_UNSIGNED_BYTE, ptr);
	delete[] ptr;
}

// output to console log (in windows, command prompt)
void consoleLog(const char *format, ...){
	va_list v;
	char buf[4096];
	va_start(v, format);
	vsprintf(buf, format, v);
	va_end(v);
	
	fprintf(stderr,"%s", buf);
}

static float calcVertexScale(vec3_t norm, vec3_t v, float radius){
	// too narrow ray causes aliasing.
	// so make it wider if needed.
	// at first calculate width.
	float w; // width
	w=vec3_t::dot(v-camera_from, -norm);
	if(w<.1f){
		// too close; use original width
		return 1.f;
	}
	w=(float)screen->w*radius*.5f/w;
	
	// if it's too narrow...
	if(w<2.f){
		// then correct width
		return 2.f/w;
	}else{
		// else original width
		return 1.f;
	}
}
void drawRay(vec3_t v1, vec3_t v2, float radius, float u1, float vv1, float u2, float vv2){
	vec3_t norm;		// direction to camera
	vec3_t side;		// side of ray
	vec3_t dir;			// v1 to v2 (normalized)
	vec3_t v;
	float vs;			// vertex scale;
	
	
	norm=(camera_from-camera_at).normalize();
	dir=(v2-v1).normalize();
	side=vec3_t::cross(v1-camera_from, dir).normalize();
	
	side*=radius;
	
	glNormal3f(norm.x, norm.y, norm.z);
	
	vs=calcVertexScale(norm, v1, radius);
	
	v=v1+side*vs;
	glTexCoord2f(u1, vv1);
	glVertex3f(v.x, v.y, v.z);
	
	v=v1-side*vs;
	glTexCoord2f(u1, vv2);
	glVertex3f(v.x, v.y, v.z);
	
	vs=calcVertexScale(norm, v2, radius);
	
	v=v2-side*vs;
	glTexCoord2f(u2, vv2);
	glVertex3f(v.x, v.y, v.z);
	
	v=v2+side*vs;
	glTexCoord2f(u2, vv1);
	glVertex3f(v.x, v.y, v.z);
	
	totalPolys+=2;
	
}

void splashDamage(vec3_t center, float range, float damage, int by){
	int n;
	mesh *m=mp->m;
	vector<isect_t> *iss;
	
	for(n=0;n<MAX_CLIENTS;n++){
		if(!cli[n].enable)
			continue;
		float dist;
		dist=(cli[n].pos-center).length()/range;
		if(dist>1.f)
			continue;
		bool canHit=false;
		
		iss=m->raycast(center, cli[n].pos);
		if(!iss->size())
			canHit=true;
		delete iss;
		
		iss=m->raycast(center, cli[n].pos+vec3_t(0.f, .7f, 0.f));
		if(!iss->size())
			canHit=true;
		delete iss;
		
		if(canHit){
			// hurt!
			dist=1.f-dist;
			float lastDamage=damage*dist*dist;
			cli[n].damage(lastDamage, center, by);
		}
	}
}

const char *getVersionString(){
	return "0.0.12";
}

