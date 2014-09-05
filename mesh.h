/*
 *  mesh.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/23.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#ifndef _MESH_H
#define _MESH_H

#include "xparser.h"

#define MAX_MATERIALS	64
#define MAX_BONES		64

class xmaterial{
public:
	char name[256];
    float dr,dg,db,da;
    float sr,sg,sb;
    float power;
    float er,eg,eb;
    GLuint tex;
	GLuint bump;
    bool texed;
	GLuint prg;
	
	float ar, ag, ab, aa; // average of texture
	int bW, bH;
	
    void begin(bool gi, bool is_fastlight, bool allowGLSL, bool lit1);
    void end();
};

struct isect_t{
	int face;
	vec3_t v1, v2, v3;
	float dist;
	isect_t(){}
	isect_t(int f, vec3_t vv1, vec3_t vv2, vec3_t vv3, float d){
		face=f; v1=vv1; v2=vv2; v3=vv3; dist=d;
		//assert(d>0.000001f);
	}
	bool operator <(const isect_t& i) const{
		return dist<i.dist;
	}
};

class mesh{
public:
    vec3_t *vertex; vec3_t *oVertex; // skinned/unskinned
    vec3_t *normal; vec3_t *oNormal; //    s  a  m  e
    vec3_t *uv;
	vec3_t *uu, *vv; vec3_t *oUU, *oVV;
	vec3_t *gi; float *giShadow;
	vec3_t *giUV;
	vec3_t *lit;
	vec3_t *lit1, *lit2, *lit3, *lit4;
	GLuint giTex, giTex1, giTex2, giTex3, giTex4;
	unsigned char *giTexBuf;
	float *giFTexBuf;
	int giDiv;
	int giSize;
	bool giGLSL;
	float *shadow;
	int *cnt;
    long *face; // V1 V2 V3 MATERIAL
	string boneName[MAX_BONES];
	float *boneW[MAX_BONES];
	float boneM[MAX_BONES][16];
	float boneOff[MAX_BONES][16];
    xmaterial *mat;
    long count_vertex;
    long count_face;
    long count_material;
	long count_bone;
	int matPolys[MAX_MATERIALS];
	GLuint list_mesh[MAX_MATERIALS];
	bool ouse_glsl[MAX_MATERIALS];
	bool invalid[MAX_MATERIALS];
	bool is_fastlight;
	bool allowGLSL;
	plane_t *rayCache;
	aabb_t *rayCache2;
	
	void render(int phase=-1);
	
	mesh(const char *fn);
	mesh(XFILE);
	
	virtual ~mesh();
	vector<isect_t> *isect(vec3_t center, float radius);
	vector<isect_t> *raycast(vec3_t v1, vec3_t v2, int exlude=-1);
	void updateGi(bool useLit=true);
	void updateSkin();
	void makeRayCache();
protected:
	void loadMesh(XFILE, bool strRead=false);
	void calc_tangent();
	
};

void Mesh_init();


#endif




