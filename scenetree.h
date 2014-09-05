/*
 *  scenetree.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/12/12.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#pragma once

#include "mesh.h"
#include "xparser.h"

class scene_t;

class frame_t{
protected:
	void calcSkin(mesh *);
	void calcFragMatrix4(float *, frame_t *from);
public:
	char name[64];
	float fMatrix[16]; // frame local matrix
	float aMatrix[16]; // matrix offset
	vector<frame_t *> children;
	vector<mesh *> meshes;
	frame_t *parent; // NULL when it's root
	scene_t *scene; 
	
	void render();
	frame_t *findByName(const char *);
	
	frame_t(scene_t *);
	frame_t(scene_t *, XFILE);
	~frame_t();
	
};

struct animation_key_t{
	vec3_t pos;
	vec3_t scale;
	quaternion_t rot;
	float m[16];
	bool useMatrix;
	animation_key_t(){
		pos=vec3_t(0.f);
		scale=vec3_t(1.f);
		rot=quaternion_t(1.f, 0.f, 0.f, 0.f);
		useMatrix=false;
		identityMatrix4(m);
	}
	void makeMatrix4(float *);
	static animation_key_t slerp(const animation_key_t& a, const animation_key_t& b, float d);
};

class animation_t{
public:
	char name[64];
	frame_t *target;
	int start;
	int duration;
	animation_key_t *keys;
	scene_t *scene;
	void apply(float);
	animation_t(scene_t *, XFILE);
	~animation_t();
};

class animation_set_t{
public:
	char name[256];
	vector<animation_t *> anims;
	scene_t *scene;
	
	void apply(float); //apply transform to frame
	
	animation_set_t(scene_t *, XFILE);
	~animation_set_t();
};

class scene_t{
protected:
	void loadScene(XFILE);
public:
	frame_t *root;
	vector<animation_set_t *> sets;
	
	// render 
	//   applys animation and render
	void render(float frame=0.f);
	
	scene_t(XFILE);
	scene_t(const char *);
	~scene_t();
	
};



