/*
 *  map.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/24.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#ifndef _BKMAP_H
#define _BKMAP_H

#include "mesh.h"
#include "sfx.h"

#define MAX_SND_ENV		32
#define MAX_ITEMS		256
#define MAX_LIGHTS		512

#define LIGHT_DATA_MAGIC		((uint32_t)0x1158da1a)

extern bool map_grass;
extern float m_itemSpawnTime;

struct lightPoint_t{
	vec3_t v00; float pad0;
	vec3_t v10; float pad1;
	vec3_t v11; float pad2;
	vec3_t v12; float pad3;
};

class lightData_t{
public:
	vec3_t minVec, maxVec;
	int32_t w, h, d;
	lightPoint_t *data;
	
	lightData_t(){data=NULL;}
	lightData_t(int ww, int hh, int dd);
	lightData_t(const char *fn);
	~lightData_t();
	vec3_t calcCoord(vec3_t v); // project global coord. to lightData_t's one
	vec3_t calcCoord(int x, int y, int z); // unproject
	lightPoint_t& nearest(int x, int y, int z); // point interpolate (actually not nearest)
	lightPoint_t interpolate(vec3_t v); // lightData_t coordinate
};

class map_t{
public:
	char mapname[256];
	mesh *m;
	mesh *way;
	float crashLevel;
	
	lightData_t *lightData;
	
	GLuint tex_skybox;
	GLuint tex_skyup;
	GLuint list_grass;
	GLuint list_way;
	float scale;
	
	int *grassPolys;
	bool grass_glsl;
	
	TMix_Chunk *snd_env[MAX_SND_ENV];
	int snd_env2[MAX_SND_ENV];
	bool snd_env_3d[MAX_SND_ENV];
	vec3_t snd_env_pos[MAX_SND_ENV];
	float snd_env_vol[MAX_SND_ENV];
	
	vec3_t spawns[64];
	float angles[64];
	bool spawnable[64];
	
	bool isGrass[256];
	bool isOcean[256];
	bool isCritical[256];
	
	// note: items are rendered in effect.cpp
	int items;
	vec3_t item[MAX_ITEMS];
	float itemspawn[MAX_ITEMS];  // time to spawn; item is unavailabe while itemspawn<0
	
	int lights;
	vec3_t lightPos[MAX_LIGHTS];
	vec3_t lightDir[MAX_LIGHTS];
	vec3_t lightColor[MAX_LIGHTS];
	float lightPower[MAX_LIGHTS];
	
	float fog;
	vec3_t fogcolor;
	
	vec3_t sunPos;
	vec3_t sunColor;
	vec3_t ambColor;
	vec3_t skyColor;
	
	map_t(const char *name, bool ignoreGI=false);
	virtual ~map_t();
	void render(bool lightEnable=true);
	void renderWayWire(); // for debugging
	void setup(vec3_t obj, bool staticLight=true, bool castShadow=false);
	vec3_t findfloor(vec3_t from, int& retFace);
	vec3_t findfloor(vec3_t from){
		int temp;
		return findfloor(from, temp);
	}
	bool isWay(vec3_t);
	float getProgress(vec3_t);
	bool isItemAvailable(int);
	
	
protected:
	
	void calcShadow();
	void normalizeWay();
	
};

extern map_t *mp;
extern lightPoint_t curLight;

void M_init();
void M_start();
void M_stop();
void M_framenext(float dt);
void M_cframenext(float dt);

#endif
