/*
 *  map.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/24.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "map.h"
#include "client.h"
#include "glpng.h"
#include "skybox.h"
#include "clientgame.h"
#include "effect.h"
#include "skygen.h"
map_t *mp;
lightPoint_t curLight;
//mesh *mesh_glass;
GLuint tex_grass;
GLuint tex_noise;
//GLuint tex_skyup;
#if GL_ARB_shader_objects
GLhandleARB prg_ocean;
GLhandleARB prg_grass;
#endif

float m_itemSpawnTime=10.f;
float m_wayMaxDistance=300000.f;

bool map_grass=true;

lightData_t::lightData_t(int ww, int hh, int dd){
	w=ww; h=hh; d=dd;
	data=new lightPoint_t[w*h*d];
}
lightData_t::lightData_t(const char *fn){
	uint32_t vl;
	FILE *f=fopen(fn, "rb");
	if(!f)
		throw "couldn't open lightData";
	fread(&vl, 1, 4, f);
	if(vl!=LIGHT_DATA_MAGIC){
		throw "invalid light data";
	}
	fread(&minVec, 1, 12, f);
	fread(&maxVec, 1, 12, f);
	fread(&w, 1, 4, f);
	fread(&h, 1, 4, f);
	fread(&d, 1, 4, f);
	data=new lightPoint_t[w*h*d];
	fread(data, w*h*d, sizeof(lightPoint_t), f);
	fclose(f);
}
lightData_t::~lightData_t(){
	if(data)
		delete[] data;
}
vec3_t lightData_t::calcCoord(vec3_t v){
	v=(v-minVec)/(maxVec-minVec);
	return v*vec3_t(w, h, d);
}
vec3_t lightData_t::calcCoord(int x, int y, int z){
	vec3_t v;
	v=vec3_t(x, y, z);
	v/=vec3_t(w, h, d);
	return minVec+v*(maxVec-minVec);
}
lightPoint_t& lightData_t::nearest(int x, int y, int z){
	if(x<0) x=0;
	if(y<0) y=0;
	if(z<0) z=0;
	if(x>=w) x=w-1;
	if(y>=h) y=h-1;
	if(z>=d) z=d-1;
	//printf("%d, %d, %d / %d, %d, %d\n", x,y, z, w-1, h-1, d-1);
	return data[x+y*w+z*w*h];
}
static lightPoint_t lerpLP(lightPoint_t& p1, lightPoint_t& p2, float per){
	lightPoint_t p=p1;
	p.v00+=(p2.v00-p1.v00)*per;
	p.v10+=(p2.v10-p1.v10)*per;
	p.v11+=(p2.v11-p1.v11)*per;
	p.v12+=(p2.v12-p1.v12)*per;
	return p;
}
lightPoint_t lightData_t::interpolate(vec3_t v){
	lightPoint_t p00, p01, p10, p11;
	lightPoint_t q00, q01, q10, q11;
	int ix, iy, iz; float per;
	ix=(int)floorf(v.x); iy=(int)floorf(v.y); iz=(int)floorf(v.z);
	// get data
	p00=nearest(ix, iy, iz);		p01=nearest(ix, iy+1, iz);
	p10=nearest(ix+1, iy, iz);		p11=nearest(ix+1, iy+1, iz);
	q00=nearest(ix, iy, iz+1);		q01=nearest(ix, iy+1, iz+1);
	q10=nearest(ix+1, iy, iz+1);	q11=nearest(ix+1, iy+1, iz+1);
	
	// interpolate X
	per=v.x-floorf(v.x);
	p00=lerpLP(p00, p10, per);
	p01=lerpLP(p01, p11, per);
	q00=lerpLP(q00, q10, per);
	q01=lerpLP(q01, q11, per);
	
	// interpolate Y
	per=v.y-floorf(v.y);
	p00=lerpLP(p00, p01, per);
	q00=lerpLP(q00, q01, per);
	
	// interpolate Z
	per=v.z-floorf(v.z);
	p00=lerpLP(p00, q00, per);
	return p00;
}


void M_init(){
	
	consoleLog("M_init: loading res/textures/grass2.png\n");
	
	glGenTextures(1, &tex_grass);
	glBindTexture(GL_TEXTURE_2D, tex_grass);
	glpngLoadTexture("res/textures/grass2.png", false);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	consoleLog("M_init: loading res/textures/noise.png\n");
	
	glGenTextures(1, &tex_noise);
	glBindTexture(GL_TEXTURE_2D, tex_noise);
	glpngLoadTexture("res/textures/noise.png", false);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#if GL_TEXTURE_MAX_ANISOTROPY_EXT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
#endif
	
	/*
	glGenTextures(1, &tex_skyup);
	glBindTexture(GL_TEXTURE_2D, tex_skyup);
	glpngLoadTexture("res/skyup.png", false);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);*/
	

#if GL_ARB_shader_objects
	
	if(cap_glsl){
	
	// load ocean shader
	
	prg_ocean=create_program("res/shaders/ocean.vs", "res/shaders/ocean.fs");
	if(prg_ocean)
		consoleLog("M_init: compiled program \"ocean\"\n");
	else
		consoleLog("M_init: couldn't compile program \"ocean\"\n");
	
	prg_grass=create_program("res/shaders/grass.vs", "res/shaders/grass.fs");
	if(prg_grass)
		consoleLog("M_init: compiled program \"grass\"\n");
	else
		consoleLog("M_init: couldn't compile program \"grass\"\n");
		
	}else{
	
#endif
	
		consoleLog("M_init: no programs to compile\n");
		
#if GL_ARB_shader_objects
	}	
#endif
}

map_t::map_t(const char *fn, bool ignoreGI){
	char str[256];
	char group[256];
	char tmp[256];
	char skybox[256];
	char skyboxup[256];
	char *ptr;
	char *name, *arg;
	int n;
	strcpy(mapname, fn);
	sprintf(str, "res/maps/%s.x", fn);
	consoleLog("map::map: loading geometry\n");
	m=new mesh(str);
	m->is_fastlight=true;
	sprintf(str, "res/maps/%s.way", fn);
	consoleLog("map::map: loading way\n");
	way=new mesh(str);
	normalizeWay();
	sprintf(str, "res/maps/%s.ini", fn);
	FILE *f;
	f=fopen(str, "r");
	group[0]=0;
	lightData=NULL;
	
	consoleLog("map::map: clearing states\n");
	
	memset(spawnable, 0, sizeof(spawnable));
	memset(str, 0, 256);
	memset(isGrass, 0, sizeof(isGrass));
	memset(isOcean, 0, sizeof(isOcean));
	memset(isCritical, 0, sizeof(isCritical));
	memset(snd_env, 0, sizeof(snd_env));
	memset(snd_env_3d, 0, sizeof(snd_env_3d));
	memset(snd_env_pos, 0, sizeof(snd_env_pos));
	memset(snd_env2, 255, sizeof(snd_env2));
	sunPos=vec3_t(0.15234375f, 0.959173304327417f, 0.23828125f);
	sunColor=vec3_t(.9f, .9f, .9f);
	ambColor=vec3_t(.3f, .3f, .3f);
	strcpy(skybox, "");
	strcpy(skyboxup, "res/clear.png");
	items=0; lights=0;
	fog=-1.f;
	//glTex=0;
	
	bool item_ground=false;
	vec3_t item_line;
	int item_lines=8;
	
	vec3_t light_dir(0.f, 0.f, 1.f);
	bool light_at=false;
	vec3_t light_color(1.f, 1.f, 1.f);
	float light_power=0.f;
	int light_lines=8;
	vec3_t light_line=vec3_t(0.f, 0.f, 0.f);
	bool light_ground=false;
	float giInt=1.f;
	float lightScale=1.f;
	
	scale=1.f;
	
	consoleLog("map::map: reading config\n");
	
	while(fgets(str, 256, f)){
		//puts(str);
		if(str[strlen(str)-1]=='\n')
			str[strlen(str)-1]=0;
		ptr=str;
		while(*ptr==' ')
			ptr++;
		if(ptr[0]=='*')
			strcpy(group, ptr+1);
		else if(ptr[0]=='#')
			continue;
		name=ptr;
		while(*name && *name!=' '){
			name++;
		}
		if(*name){
			*name=0;
			arg=name+1;
		}else{
			arg=name;
		}
		name=ptr;
		
		if(!strcasecmp(group, "global")){
			if(!strcasecmp(name, "fog")){
				fog=atof(arg);
			}else if(!strcasecmp(name, "fogcolor")){
				fogcolor=parse_vec3(arg);
			}else if(!strcasecmp(name, "scale")){
				scale=atof(arg);
			}
		}else if(!strcasecmp(group, "sky")){
			if(!strcasecmp(name, "skybox")){
				strcpy(skybox, arg);
			}else if(!strcasecmp(name, "skyup")){
				strcpy(skyboxup, arg);
			}
		}else if(!strcasecmp(group, "light")){
			if(!strcasecmp(name, "suncolor")){
				sunColor=parse_vec3(arg);
			}else if(!strcasecmp(name, "sunpos")){
				sunPos=parse_vec3(arg);
			}else if(!strcasecmp(name, "ambcolor")){
				ambColor=parse_vec3(arg);
			}
			else if(!strcasecmp(name, "skycolor")){
				skyColor=parse_vec3(arg);
			}else if(!strcasecmp(name, "lightat")){
				light_dir=parse_vec3(arg);
				light_at=true;
			}else if(!strcasecmp(name, "lightdir")){
				light_dir=parse_vec3(arg);
				light_at=false;
			}else if(!strcasecmp(name, "lightcolor")){
				light_color=parse_vec3(arg);
			}else if(!strcasecmp(name, "lightpower")){
				light_power=atof(arg);
			}else if(!strcasecmp(name, "light")){
				lightPos[lights]=parse_vec3(arg);
				if(light_ground){
					lightPos[lights]=findfloor(lightPos[lights]+vec3_t(0.f, 1.f, 0.f))+vec3_t(0.f, .1f, 0.f);
				}
				lightDir[lights]=(light_at?(light_dir-lightPos[lights]):light_dir).normalize();
				lightColor[lights]=light_color*lightScale*scale;
				lightPower[lights]=light_power;
				lights++;
			}else if(!strcasecmp(name, "lightcount")){
				light_lines=atoi(arg);
			}else if(!strcasecmp(name, "lightbegin")){
				light_line=parse_vec3(arg);
			}else if(!strcasecmp(name, "lightend")){
				vec3_t from=light_line;
				vec3_t to=parse_vec3(arg);
				vec3_t dt=(to-from)*(1.f/(float)light_lines);
				for(n=0;n<light_lines;n++){
					lightPos[lights]=from;
					if(light_ground){
						lightPos[lights]=findfloor(lightPos[lights]+vec3_t(0.f, 1.f, 0.f))+vec3_t(0.f, .1f, 0.f);
					}
					lightDir[lights]=(light_at?(light_dir-lightPos[lights]):light_dir).normalize();
					lightColor[lights]=light_color*lightScale*scale;
					lightPower[lights]=light_power;
					lights++;
					from+=dt;
				}
			}else if(!strcasecmp(name, "lightground")){
				light_ground=atoi(arg);
			}else if(!strcasecmp(name, "giintensity")){
				giInt=atof(arg);
			}else if(!strcasecmp(name, "lightscale")){
				lightScale=atof(arg);
			}
		}else if(!strcasecmp(group, "material")){
			if(!strcasecmp(name, "grass")){
				isGrass[atoi(arg)]=true;
			}else if(!strcasecmp(name, "ocean")){
				isOcean[atoi(arg)]=true;
			}else if(!strcasecmp(name, "critical")){
				isCritical[atoi(arg)]=true;
			}
		}else if(!strcasecmp(group, "spawnpoint")){
			strcpy(tmp, name);
			tmp[5]=0;
			if(!strcasecmp(tmp, "spawn")){
				ptr=name+5;
				spawns[atoi(ptr)]=parse_vec3(arg);
				spawnable[atoi(ptr)]=true;
				
			}else if(!strcasecmp(tmp, "angle")){
				ptr=name+5;
				angles[atoi(ptr)]=atof(arg);
				
			}
		}else if(!strcasecmp(group, "item")){
			if(!strcasecmp(name, "ground")){
				item_ground=atoi(arg);
			}else if(!strcasecmp(name, "linecount")){
				item_lines=atoi(arg);
			}else if(!strcasecmp(name, "linebegin")){
				item_line=parse_vec3(arg);
			}else if(!strcasecmp(name, "lineend")){
				vec3_t from=item_line;
				vec3_t to=parse_vec3(arg);
				vec3_t df=(to-from)*(1.f/(float)item_lines);
				for(n=0;n<item_lines;n++){
					vec3_t ps=from;
					if(item_ground){
						ps=findfloor(ps+vec3_t(0.f, 1.f, 0.f));
					}
					item[items]=ps;
					itemspawn[items]=1.f;
					items++;
					from+=df;
				}
			}else if(!strcasecmp(name, "point")){
				vec3_t ps=parse_vec3(arg);
				if(item_ground){
					ps=findfloor(ps+vec3_t(0.f, 1.f, 0.f));
				}
				item[items]=ps;
				itemspawn[items]=1.f;
				items++;
			}
			
		}else if(!strcasecmp(group, "sound")){
			strcpy(tmp, name);
			tmp[7]=0;
			if(!strcasecmp(tmp, "looppos")){
				ptr=name+7;
				snd_env_3d[atoi(ptr)]=true;
				snd_env_pos[atoi(ptr)]=parse_vec3(arg);
				continue;
			}
			if(!strcasecmp(tmp, "loopvol")){
				ptr=name+7;
				snd_env_vol[atoi(ptr)]=atof(arg);
				continue;
			}
			strcpy(tmp, name);
			tmp[4]=0;
			if(!strcasecmp(tmp, "loop")){
				ptr=name+4;
				sprintf(tmp, "res/%s", arg);
				snd_env[atoi(ptr)]=TMix_LoadWAV(tmp);
				snd_env_vol[atoi(ptr)]=1.f;
				continue;
			}
		}
	}
	fclose(f);
	if(!ignoreGI){
		if(sprintf(str, "res/maps/%s.grd", fn),f=fopen(str, "rb")){
			fclose(f);
			lightData=new lightData_t(str);
			consoleLog("map::map: original range: (%f, %f, %f)-(%f, %f, %f)\n", lightData->minVec.x, lightData->minVec.y, lightData->minVec.z,
					   lightData->maxVec.x, lightData->maxVec.y, lightData->maxVec.z);
			lightData->minVec*=scale;
			lightData->maxVec*=scale;
			consoleLog("map::map: scaled range: (%f, %f, %f)-(%f, %f, %f)\n", lightData->minVec.x, lightData->minVec.y, lightData->minVec.z,
					   lightData->maxVec.x, lightData->maxVec.y, lightData->maxVec.z);
			consoleLog("map::map: lightData file read\n");
			
		}
		if(sprintf(str, "res/maps/%s.bgi", fn),f=fopen(str, "rb")){
			// binary GI file
			consoleLog("map::map: Binary GI file found\n");
			int32_t val;
			fread(&val, 4, 1, f);
			if(val!=0xb1d11249){ // magic number, must be the same as one in gi2bgi.cpp
				consoleLog("map::map: invalid Binary GI magic number, or unsupported endian (real=0x%08x, expected=0x%08x)\n",
					   val, 0xb1d11249);
				goto abrtBGI;
			}
			fread(&val, 4, 1, f);
			m->giDiv=val;
			fread(&val, 4, 1, f);
			if(val!=m->count_face){
				consoleLog("map::map: invalid Binary GI data (invalid face count)\n");
				goto abrtBGI;
			}
			int i;
			i=m->count_face*m->giDiv*m->giDiv;
			fread(&val, 4, 1, f);
			if(val!=i && val!=i*2){
				consoleLog("map::map: invalid Binary GI data (invalid size)\n");
				goto abrtBGI;
			}
			m->gi=new vec3_t[i];
			m->giShadow=new float[i];
			for(n=0;n<i;n++){
				fread(&(m->gi[n]), 12, 1, f);
				fread(&(m->giShadow[n]), 4, 1, f);
				m->gi[n]=m->gi[n]*giInt;
			}
			if(val==i*2){
				// has static light map
				m->lit=new vec3_t[i];
				for(n=0;n<i;n++){
					fread(&(m->lit[n]), 12, 1, f);
					fread(&val, 4, 1, f); // dummy
					m->lit[n]*=lightScale;
				}
			}
			m->updateGi();
			sprintf(str, "res/maps/%s.lit", fn);
			fclose(f);
			consoleLog("map::map: Binary GI file read\n");
			if(f=fopen(str, "rb")){
				m->lit1=new vec3_t[i];
				m->lit2=new vec3_t[i];
				m->lit3=new vec3_t[i];
				m->lit4=new vec3_t[i];
				fread(m->lit1, sizeof(vec3_t), i, f);
				fread(m->lit2, sizeof(vec3_t), i, f);
				fread(m->lit3, sizeof(vec3_t), i, f);
				fread(m->lit4, sizeof(vec3_t), i, f);
				consoleLog("map::map: Binary Lit file read\n");
			}
		abrtBGI:
			fclose(f);
			
		}else if(sprintf(str, "res/maps/%s.gi", fn),f=fopen(str, "r")){
			// text GI file
			consoleLog("map::map: Text GI file found\n");
			fgets(str, 256, f);
			m->giDiv=atoi(str);
			fgets(str, 256, f);
			if(atoi(str)!=m->count_face){
				consoleLog("map::map: invalid GI data (invalid face count)\n");
				goto abrtGI;
			}
			float scl;
			fgets(str, 256, f);
			scl=atof(str);
			int i;
			i=m->count_face*m->giDiv*m->giDiv;
			m->gi=new vec3_t[i];
			m->giShadow=new float[i];
			for(n=0;n<i;n++){
				fgets(str, 256, f);
				m->gi[n]=parse_vec3(str)*giInt;
				fgets(str, 256, f);
				m->giShadow[n]=atof(str);
			}
			m->updateGi();
		abrtGI:
			fclose(f);
			consoleLog("map::map: Text GI file read\n");
		}
	}
	consoleLog("map::map: calculating shadow\n");
	
	
	for(n=0;n<64;n++){
		spawns[n]*=scale;
		
	}
	
	
	if(skybox[0]){
		consoleLog("map::map: loading skybox\n");
		glGenTextures(1, &tex_skybox);
		glBindTexture(GL_TEXTURE_2D, tex_skybox);
		glpngLoadTexture(skybox, false);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		
		glGenTextures(1, &tex_skyup);
		glBindTexture(GL_TEXTURE_2D, tex_skyup);
		glpngLoadTexture(skyboxup, true);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}else{
		consoleLog("map::map: generating skybox (skygen)\n");
		glGenTextures(1, &tex_skybox);
		glBindTexture(GL_TEXTURE_2D, tex_skybox);
		SG_generateSkyBox(this);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		
		glGenTextures(1, &tex_skyup);
		glBindTexture(GL_TEXTURE_2D, tex_skyup);
		SG_generateSkyBoxUp(this);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}
	
	consoleLog("map::map: preparing\n");
	for(n=0;n<MAX_SND_ENV;n++)
		snd_env_pos[n]*=scale;
	for(n=0;n<m->count_vertex;n++)
		m->vertex[n]*=scale;
	for(n=0;n<way->count_vertex;n++)
		way->vertex[n]*=scale;
	for(n=0;n<MAX_ITEMS;n++)
		item[n]*=scale;
	for(n=0;n<MAX_LIGHTS;n++)
		lightPos[n]*=scale;
	
	crashLevel=m->vertex[0].y;
	for(n=1;n<m->count_vertex;n++)
		crashLevel=min(crashLevel, m->vertex[n].y);
	crashLevel-=4.f;
	
	list_grass=glGenLists(m->count_face*4);
	list_way=0;
	grassPolys=new int[m->count_face*4];
	memset(grassPolys, 0, sizeof(int)*(m->count_face*4));
	calcShadow();
	m->makeRayCache();
	way->makeRayCache();
}



void map_t::normalizeWay(){
	int n;
	aabb_t bound(way->uv, way->count_vertex);
	for(n=0;n<way->count_vertex;n++)
		way->uv[n].x=(way->uv[n].x-bound.minvec.x)/(bound.maxvec.x-bound.minvec.x);
}

void map_t::calcShadow(){
	m->shadow=new float[m->count_vertex];
	int n;
	for(n=0;n<m->count_vertex;n++){
		vec3_t v1, v2;
		v1=v2=m->vertex[n];
		v1+=sunPos*.1f;
		v2+=sunPos*10000.f;
		vector<isect_t> *rc=m->raycast(v1,v2);
		if(rc->size()){
			m->shadow[n]=0.f;
		}else{
			m->shadow[n]=1.f;
		}
		delete rc;
	}
}
struct light_sort_t{
	map_t *mp;
	int id; float dist;
	
	light_sort_t(){}
	light_sort_t(map_t *m, int i, vec3_t cent){
		mp=m; id=i;
		dist=(cent-mp->lightPos[i]).length_2();
	}
	bool operator <(const light_sort_t& t) const{
		return dist<t.dist;
	}
};
extern float oang;
void map_t::setup(vec3_t obj, bool staticLight, bool castShadow){
	int n;
	::tex_skybox=this->tex_skybox;
	if(fog<0.f)
		glDisable(GL_FOG);
	else{
		float col[]={fogcolor.x, fogcolor.y, fogcolor.z, 1.f};
		glEnable(GL_FOG);
		glFogi(GL_FOG_MODE, GL_LINEAR);
		glFogi(GL_FOG_END, fog);
		glFogfv(GL_FOG_COLOR, col);
		//glFogi(GL_FOG_COORD_SRC, GL_FRAGMENT_DEPTH);
#if GL_NV_fog_distance
		glFogi(GL_FOG_DISTANCE_MODE_NV, GL_EYE_RADIAL_NV );
#endif
	}
	
	curLight.v00=curLight.v10=
	curLight.v11=curLight.v12=vec3_t(0.f);
	
	// global illummination light
	if(lightData){
		curLight=lightData->interpolate(lightData->calcCoord(obj+vec3_t(0.f, .5f, 0.f)));
		//curLight.v11.z=rnd();
		//printf("%f, %f, %f\n", curLight.v00.x, curLight.v00.y, curLight.v00.z);
	}
	
	// directlight enable
	
	{
		vec3_t col=sunColor;
		if(!castShadow){
		}else{
			vector<isect_t> *rc2=m->raycast(obj, obj+sunPos.normalize()*10000.f);
			if(rc2->size()){
				// in shadow
				delete rc2;
				col=vec3_t(0.f);
			}else{
				// visible
				delete rc2;
			}
		}
		
		glLightfv(GL_LIGHT0, GL_DIFFUSE, color4f(col.x, col.y, col.z, 1.f));
		glLightfv(GL_LIGHT0, GL_SPECULAR, color4f(col.x, col.y, col.z, 1.f));
	}
	
	light_sort_t lt[MAX_LIGHTS];
	for(n=0;n<lights;n++){
		lt[n]=light_sort_t(this, n, obj);
		/*if(castShadow){
			vector<isect_t> *rc2=m->raycast(lightPos[n], obj);
			if(rc2->size()){
				// in shadow
				delete rc2;
				lt[n].dist=1e20f;
			}else{
				delete rc2;
			}
		}*/
	}
	sort(lt, lt+lights);
	
	GLint glLimit;
	glGetIntegerv(GL_MAX_LIGHTS, &glLimit);
	glLimit-=2;
	
	if(!staticLight){
		for(n=0;n<glLimit;n++){
			glDisable(GL_LIGHT0+2+n);
			glLightfv(GL_LIGHT0+2+n, GL_DIFFUSE, color4f(0.f, 0.f, 0.f, 1.f));
			glLightfv(GL_LIGHT0+2+n, GL_SPECULAR, color4f(0.f, 0.f, 0.f, 1.f));
		}
		return;
	}
	
	// secondary sort
	vector<light_sort_t> lt2;
	for(n=0;n<lights;n++){
		if(lt2.size()>=glLimit)
			break;
		if(!castShadow){
			lt2.push_back(lt[n]);
		}else{
			vector<isect_t> *rc2=m->raycast(obj, lightPos[lt[n].id]);
			if(rc2->size()){
				// in shadow
				delete rc2;
				continue;
			}else{
				// visible
				delete rc2;
				lt2.push_back(lt[n]);
			}
		}
	}
	
	
	
	for(n=0;n<glLimit;n++){
		if(n>=lt2.size()){
			glDisable(GL_LIGHT0+2+n);
			glLightfv(GL_LIGHT0+2+n, GL_DIFFUSE, color4f(0.f, 0.f, 0.f, 1.f));
			glLightfv(GL_LIGHT0+2+n, GL_SPECULAR, color4f(0.f, 0.f, 0.f, 1.f));
			continue;
		}
		int i=lt2[n].id;
		int cLight=GL_LIGHT0+2+n;
		vec3_t v;
		float scale=1.f;
		
		glEnable(cLight);
		
		v=lightColor[i];
		scale=1.f/max(v.x, max(v.y, v.z));
		v*=scale;
		glLightfv(cLight, GL_DIFFUSE, color4f(v.x, v.y, v.z, 1.f));
		glLightfv(cLight, GL_SPECULAR, color4f(v.x, v.y, v.z, 1.f));
		
		v=lightPos[i];
		glLightfv(cLight, GL_POSITION, color4f(v.x, v.y, v.z, 1.f));
		
		v=lightDir[i];
		glLightfv(cLight, GL_SPOT_DIRECTION, color4f(v.x, v.y, v.z, 1.f));
		
		glLightf(cLight, GL_SPOT_EXPONENT, lightPower[i]);
		glLightf(cLight, GL_SPOT_CUTOFF, (lightPower[i]>.001f)?90.f: 180.f);
		glLightf(cLight, GL_CONSTANT_ATTENUATION, 0.f);
		glLightf(cLight, GL_LINEAR_ATTENUATION, scale);
		glLightf(cLight, GL_QUADRATIC_ATTENUATION, 0.f);
	}
	
	
}
void map_t::renderWayWire(){
	if(list_way){
		glCallList(list_way);
		return;
	}
	list_way=glGenLists(1);
	glNewList(list_way, GL_COMPILE_AND_EXECUTE);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glLineWidth(1.f);
	glDisable(GL_TEXTURE_2D);
	int n; vec3_t v;
	for(n=0;n<way->count_face;n++){
		glBegin(GL_LINE_LOOP);
		//glColor4f(1.f-way->uv[way->face[n*4+0]].x, way->uv[way->face[n*4+0]].x, 0.f, 1.f);
		v=way->vertex[way->face[n*4+0]];
		glVertex3f(v.x, v.y, v.z);
		//glColor4f(1.f-way->uv[way->face[n*4+1]].x, way->uv[way->face[n*4+1]].x, 0.f, 1.f);
		v=way->vertex[way->face[n*4+1]];
		glVertex3f(v.x, v.y, v.z);
		//glColor4f(1.f-way->uv[way->face[n*4+2]].x, way->uv[way->face[n*4+2]].x, 0.f, 1.f);
		v=way->vertex[way->face[n*4+2]];
		glVertex3f(v.x, v.y, v.z);
		glEnd();
	}
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_FOG);
	glDepthMask(GL_TRUE);
	glEndList();
}
void map_t::render(bool lightEnable){
	int n;
	if(m->lit &&  (use_glsl || cap_multiTex)){ // if static light is already calculated
		setup(vec3_t(0,0,0), false); // remove static light
		
	}
	if(!lightEnable){
		setup(vec3_t(0,0,0), false);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, color4f(0.f, 0.f, 0.f, 1.f));
		glLightfv(GL_LIGHT0, GL_SPECULAR, color4f(0.f, 0.f, 0.f, 1.f));
	}
	if(m->gi)
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, color4f(0.f, 0.f, 0.f, 1.f)); // ambient light is already calculated (GI)
	for(n=0;n<m->count_material;n++){
		if(isOcean[n]){
#if GL_ARB_shader_objects && GL_ARB_multitexture
			if(use_glsl){
				glUseProgramObjectARB(prg_ocean);
				glUniform4fARB(glGetUniformLocationARB(prg_ocean, "color"),
							0.1f, 0.3f, .7f, 1.f);
				glUniform1iARB(glGetUniformLocationARB(prg_ocean, "tex1"),
							0);
				glUniform1iARB(glGetUniformLocationARB(prg_ocean, "tex2"),
						   1);
				glUniform1iARB(glGetUniformLocationARB(prg_ocean, "tex3"),
						   2);
				glUniform1fARB(glGetUniformLocationARB(prg_ocean, "pos"),
							(float)SDL_GetTicks()/1000.f*.03f);
				glUniform1fARB(glGetUniformLocationARB(prg_ocean, "ang"),
							oang);

				glActiveTextureARB(GL_TEXTURE1_ARB);
				glBindTexture(GL_TEXTURE_2D, tex_noise);
				glActiveTextureARB(GL_TEXTURE2_ARB);
				glBindTexture(GL_TEXTURE_2D, tex_skyup);
				glActiveTextureARB(GL_TEXTURE0_ARB);

			}else{
#endif
				
				m->mat[n].dr=0.01f;
				m->mat[n].dg=0.02f;
				m->mat[n].db=0.4f;
				m->mat[n].da=1.0f;
				
#if GL_ARB_shader_objects
			}
#endif

		}
		if(isGrass[n]){
#if GL_ARB_shader_objects && GL_ARB_multitexture
			if(use_glsl){
				
				m->mat[n].dr=0.8f;
				m->mat[n].dg=0.8f;
				m->mat[n].db=0.8f;
				m->mat[n].da=1.0f;
				
			}else{
#endif
				
				m->mat[n].dr=0.5f;
				m->mat[n].dg=0.5f;
				m->mat[n].db=0.5f;
				m->mat[n].da=1.0f;
				
#if GL_ARB_shader_objects
			}
#endif
			
		}
		m->render(n);
#if GL_ARB_shader_objects && GL_ARB_multitexture
		if(use_glsl)
			glUseProgramObjectARB(NULL);
#endif
	}
	
	// render glass.
	//  at first find all polygons near camera

	if(!map_grass)
		return;
	
	if(!use_glsl)
		glDisable(GL_LIGHTING);
	
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color4f(1.f, 1.f, 1.f, 1.f));
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color4f(0.f, 0.f, 0.f, 1.f));
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color4f(0.f, 0.f, 0.f, 1.f));
	vec3_t cam=camera_from;
	const float rrange=1.f/30.f;
	const float rsubrange=1.f/20.f;
	const float rsubsubrange=1.f/10.f;
	float interval;
	static const vec3_t vecs[]={
		vec3_t(-.5f, 0.f, 0.f), vec3_t(.5f, 0.f, 0.f),
		vec3_t(.5f, .5f, 0.1f), vec3_t(-.5f, .5f, 0.1f),
		vec3_t(-.43f, 0.f, .13f), vec3_t(-.02f, 0.f, -.21f),
		vec3_t(-.22f, .6f, -.21f), vec3_t(-.63f, .6f, .13f),
		vec3_t(-.18f, 0.f, -.21f), vec3_t(.23f, 0.f, .13f), 
		vec3_t(.43f, .6f, .13f), vec3_t(.02f, .6f, -.21f)
	};
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_grass);
	
	float oRef;
	
	glGetFloatv(GL_ALPHA_TEST_REF, &oRef);
	
	glDisable(GL_BLEND);
	glAlphaFunc(GL_GREATER, 0.6f);
	//glBlendFunc(GL_SRC_ALPHA,GL_ZERO);
	
	glColor4f(.8f, .8f, .8f, 1);
	
#if GL_ARB_shader_objects && GL_ARB_multitexture
	if(use_glsl && m->gi){
		glActiveTexture(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, m->giTex);
		glActiveTexture(GL_TEXTURE0_ARB);
		glUseProgramObjectARB(prg_grass);
		glUniform1iARB(glGetUniformLocationARB(prg_grass, "tex"),
					   0);
		glUniform1iARB(glGetUniformLocationARB(prg_grass, "giTex"),
					   1);
		//glColor4f(1.f, 1.f, 1.f, 1);
	}
#endif
	
	
	
	
	//glSecondaryColor3f(0.f, 0.f, 0.f);
	
	float frm;
	frm=(float)SDL_GetTicks()/1000.f*2.f;
	
	vec3_t camDir=(camera_at-camera_from).normalize();
	
	if(grass_glsl!=use_glsl){
		// GLSL mode changed, invalidating lists.
		for(n=0;n<m->count_face*4;n++)
			grassPolys[n]=0;
	}
	
	for(n=0;n<m->count_face;n++){
		if(!isGrass[m->face[(n<<2)+3]])
			continue;
		if((m->vertex[m->face[n<<2]]-cam).rlength()<rrange && (m->vertex[m->face[(n<<2)+1]]-cam).rlength()<rrange
			&& (m->vertex[m->face[(n<<2)+2]]-cam).rlength()<rrange)
			continue;
		vec3_t v1, v2, v3;
		vec3_t n1, n2, n3;
		vec3_t u1, u2, u3;
		v1=m->vertex[m->face[(n<<2)]];
		v2=m->vertex[m->face[(n<<2)+1]];
		v3=m->vertex[m->face[(n<<2)+2]];
		if(vec3_t::dot(v1-camera_from, camDir)<0.f && vec3_t::dot(v2-camera_from, camDir)<0.f && vec3_t::dot(v3-camera_from, camDir)<0.f){
			// not seen because it is behind camera
			continue;
		}
		n1=m->normal[m->face[(n<<2)]];
		n2=m->normal[m->face[(n<<2)+1]];
		n3=m->normal[m->face[(n<<2)+2]];
		u1=m->giUV[n*3];
		u2=m->giUV[n*3+1];
		u3=m->giUV[n*3+2];
		
		plane_t plane(v1, v2, v3);
		float len, pas;
		float scale=4.3f;
		float scaley=.5f;
		
		if(plane.n.y<0.6f)
			continue;
		
		for(int phase=0;phase<4;phase++){
			
			float tv1, tv2, tu;
			GLuint lst=list_grass+n*4+phase;
			
			//totalPolys+=;
			
			if(grassPolys[n*4+phase]){
				
				glCallList(lst);
				
			}else{
				
				grassPolys[n*4+phase]=0;
			
				
				
				tv1=(float)phase/4.f;
				tv2=tv1+.25f;
				tu=4.f;
				
				if(phase==0){
				
					if(plane.n.y>0.9f)
						interval=2.3f;
					else if(plane.n.y>0.8f)
						interval=3.5f;
					else
						interval=6.f;
					
				}else if(phase==1){
					
					if((v1-cam).rlength()<rsubsubrange && (v2-cam).rlength()<rsubsubrange
					   && (v3-cam).rlength()<rsubsubrange)
						continue;
					
					interval=0.8f;
					scale=2.4f;
					tu=2.f;
					if(plane.n.y<.8f){
						interval=.4f;
						scale=1.2f;
						tu=1.f;
					}
					
				}else if(phase==2){
					
					if((v1-cam).rlength()<rsubsubrange && (v2-cam).rlength()<rsubsubrange
					   && (v3-cam).rlength()<rsubsubrange)
						continue;
					
					interval=1.4f;
					scale=3.4f;
					tu=2.f;
					
					if(plane.n.y<.8f){
						interval=.7f;
						scale=1.7f;
						tu=1.f;
					}
					
				}else if(phase==3){
					
					if((v1-cam).rlength()<rsubrange && (v2-cam).rlength()<rsubrange
					   && (v3-cam).rlength()<rsubrange)
						continue;
					
					interval=2.0f;
					scale=0.6f;
					tu=1.f;
					
					
				}
				
				glNewList(lst, GL_COMPILE_AND_EXECUTE);
				
				vec3_t v, vv, w, ww;
				len=(v2-v1).length();
				v=v1; vv=(v2-v1).normalize()*interval;
				w=v1; ww=(v3-v1)*interval/len;
				
				vec3_t nm1, dnm1, dnm2, dnm3;
				vec3_t nm2, nm3;
				vec3_t uv1, uv2, uv3;
				vec3_t duv1, duv2, duv3;
				
				nm1=nm2=n1; uv1=uv2=u1;
				dnm1=(n2-n1)*interval/len;
				dnm2=(n3-n1)*interval/len;
				duv1=(u2-u1)*interval/len;
				duv2=(u3-u1)*interval/len;
				bool sf=false;
				
				glBegin(GL_QUADS);
				
				for(pas=0.f;pas<len-((phase==1 || phase==2)?0.f:(scale*.5f));pas+=interval){
					sf=!sf;
					vec3_t x, xx;
					float len2, pas2;
					x=v; xx=(w-v).normalize()*interval;
					len2=(w-v).length();
					//x.y-=.3f;
					nm3=nm1;
					dnm3=(nm2-nm1)*(interval/len2);
					uv3=uv1;
					duv3=(uv2-uv1)*(interval/len2);
					for(pas2=sf?(len2*.5f):0.f;pas2<len2-((phase==1 || phase==2)?0.f:(scale*.5f));pas2+=interval){
						vec3_t nm4=nm3.normalize();
						scaley=1.f*(1.f+((len*.5f)-fabs(pas-len*.5f))*((len2*.5f)-fabs(pas2-len2*.5f))/(len*len2)*10.f);
						if(phase==1){
							scaley*=.6f;
						}else if(phase==2){
							scaley*=.7f;
						}else if(phase==3){
							scaley*=.2f;
						}
						
						vec3_t col=vec3_t(1.f);
						if(m->gi && (!use_glsl)){
							float gx, gy;
							gy=pas/len*(float)(m->giDiv-1);
							gx=pas2/len2*gy;
							
							int ix, iy;
							float fx, fy;
							int ind;
							
							ind=n*m->giDiv*m->giDiv;
							ix=(int)floorf(gx);
							iy=(int)floorf(gy);
							fx=gx-floorf(gx);
							fy=gy-floorf(gy);
							
							const int subDiv=m->giDiv;
							
#define readVec(x, y)	m->gi[ind+((x)&(subDiv-1))+((y)&(subDiv-1))*subDiv]
#define readShd(x, y)	m->giShadow[ind+((x)&(subDiv-1))+((y)&(subDiv-1))*subDiv]
#define readGLt(x, y)	m->lit[ind+((x)&(subDiv-1))+((y)&(subDiv-1))*subDiv]
							
							vec3_t a, b, c, d;
							
							a=readShd(ix, iy); b=readShd(ix+1, iy);
							c=readShd(ix, iy+1); d=readShd(ix+1, iy+1);
							
							a+=(b-a)*fx;
							c+=(d-c)*fx;
							a+=(c-a)*fy;
							
							col=sunColor*a*max(0.f, vec3_t::dot(nm4, sunPos));
							
							a=readGLt(ix, iy); b=readGLt(ix+1, iy);
							c=readGLt(ix, iy+1); d=readGLt(ix+1, iy+1);
							
							a+=(b-a)*fx;
							c+=(d-c)*fx;
							a+=(c-a)*fy;
							
							col+=a;
							
							a=readVec(ix, iy); b=readVec(ix+1, iy);
							c=readVec(ix, iy+1); d=readVec(ix+1, iy+1);
							
							a+=(b-a)*fx;
							c+=(d-c)*fx;
							a+=(c-a)*fy;
							
							col+=a;
							
						}
						
						float sx;
						sx=sinf(x.x*.3f-frm)*.06f;
						glNormal3f(nm4.x, nm4.y, nm4.z);
		#if GL_ARB_multitexture
						if(cap_multiTex){
							glMultiTexCoord2fARB(GL_TEXTURE1_ARB, uv3.x, uv3.y);
						}
		#endif
						for(int i=0;i<12;i+=4){
							
							glColor4f(.6f*col.x, .6f*col.y, .6f*col.z, 1);
							glTexCoord2f(0.f, tv2);
							glVertex3f(vecs[i+0].x*scale+x.x, vecs[i+0].y*scaley+x.y, vecs[i+0].z*scale+x.z);
							glTexCoord2f(tu, tv2);
							glVertex3f(vecs[i+1].x*scale+x.x, vecs[i+1].y*scaley+x.y, vecs[i+1].z*scale+x.z);
							glColor4f(col.x, col.y, col.z, 1);
							glTexCoord2f(tu+sx, tv1);
							glVertex3f(vecs[i+2].x*scale+x.x, vecs[i+2].y*scaley+x.y, vecs[i+2].z*scale+x.z);
							glTexCoord2f(0.f+sx, tv1);
							glVertex3f(vecs[i+3].x*scale+x.x, vecs[i+3].y*scaley+x.y, vecs[i+3].z*scale+x.z);
							grassPolys[n*4+phase]++;
						}
						x+=xx;
						nm3+=dnm3;
						uv3+=duv3;
					}
					v+=vv;
					w+=ww;
					nm1+=dnm1;
					nm2+=dnm2;
					uv1+=duv1;
					uv2+=duv2;
				}
				
				glEnd();
				
				glEndList();
				
			}
			
			totalPolys+=grassPolys[n*4+phase];
		}
	}
	
#if GL_ARB_shader_objects && GL_ARB_multitexture
	if(use_glsl && m->gi){
		glUseProgramObjectARB(0);
	}
#endif
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_GREATER, oRef);
	glEnable(GL_BLEND);
	if(m->lit) // restore all static light
		setup(camera_from, true);
	//glLightModelfv(GL_LIGHT_MODEL_AMBIENT, color4f(ambColor.x, ambColor.y, ambColor.z, 1.f));
	grass_glsl=use_glsl;
	glEnable(GL_LIGHTING);
}

vec3_t map_t::findfloor(vec3_t from, int& retFace){
	int n;
	typedef struct{
		vec3_t v1, v2;
		vec3_t hit;
		float per;
		void calc(float z){
			per=(z-v1.z)/(v2.z-v1.z);
			hit.x=v1.x+(v2.x-v1.x)*per;
			hit.y=v1.y+(v2.y-v1.y)*per;
			hit.z=z;
			
			assert(z>=min(v1.z, v2.z));
			assert(z<=max(v1.z, v2.z));
		}
	} spanhit_t;
	float maxy=-100000.f;;
	for(n=0;n<m->count_face;n++){
		vec3_t vt[3];
		vec3_t vt1, vt2, vt3;
		float minz, maxz;
		float minx, maxx;
		float miny;
		
		vt[0]=m->vertex[m->face[(n<<2)]];
		vt[1]=m->vertex[m->face[(n<<2)|1]];
		vt[2]=m->vertex[m->face[(n<<2)|2]];
		
		minz=min(vt[0].z, min(vt[1].z, vt[2].z));
		maxz=max(vt[0].z, max(vt[1].z, vt[2].z));
		
		if(from.z<minz || from.z>=maxz)
			continue;
		
		minx=min(vt[0].x, min(vt[1].x, vt[2].x));
		maxx=max(vt[0].x, max(vt[1].x, vt[2].x));
		
		if(from.x<minx || from.x>=maxx)
			continue;
		
		miny=min(vt[0].y, min(vt[1].y, vt[2].y));
		if(miny>from.y)
			continue;
		
		// insertion sort
		
		if(vt[0].z>vt[1].z){
			vt1=vt[1];
			vt2=vt[0];
		}else{
			vt1=vt[0];
			vt2=vt[1];
		}
		if(vt[2].z<vt1.z){
			vt3=vt2;
			vt2=vt1;
			vt1=vt[2];
		}else if(vt[2].z<vt2.z){
			vt3=vt2;
			vt2=vt[2];
		}else{
			vt3=vt[2];
		}
		assert(vt1.z<=vt2.z);
		assert(vt2.z<=vt3.z);
		assert(vt1.x!=vt2.x || vt1.y!=vt2.y || vt1.z!=vt2.z);
		assert(vt2.x!=vt3.x || vt2.y!=vt3.y || vt2.z!=vt3.z);
		spanhit_t hit1, hit2;
		
		assert(from.z>=vt1.z);
		assert(from.z<=vt3.z);
		if(from.z<vt2.z){
			hit1.v1=vt1;
			hit1.v2=vt2;
		}else{
			hit1.v1=vt2;
			hit1.v2=vt3;
		}
		hit2.v1=vt1;
		hit2.v2=vt3;
		
		hit1.calc(from.z);
		hit2.calc(from.z);
		
		if(from.x<min(hit1.hit.x, hit2.hit.x) || from.x>=max(hit1.hit.x, hit2.hit.x))
			continue;
		
		// x-direction
		
		float per;
		vec3_t hit;
		
		per=(from.x-hit1.hit.x)/(hit2.hit.x-hit1.hit.x);
		assert(per>=-.1f);
		assert(per<1.1f);
		
	
		
		hit.x=from.x;
		hit.y=hit1.hit.y+(hit2.hit.y-hit1.hit.y)*per;
		hit.z=from.z;
		
		if(hit.y<from.y){
			if(hit.y>maxy){
				maxy=hit.y;
				retFace=n;
			}
		}
		
	}
	
	return vec3_t(from.x, maxy, from.z);
}
bool map_t::isWay(vec3_t from){
	int n;
	typedef struct{
		vec3_t v1, v2;
		vec3_t hit;
		float per;
		void calc(float z){
			per=(z-v1.z)/(v2.z-v1.z);
			hit.x=v1.x+(v2.x-v1.x)*per;
			hit.y=v1.y+(v2.y-v1.y)*per;
			hit.z=z;
			
			assert(z>=min(v1.z, v2.z));
			assert(z<=max(v1.z, v2.z));
		}
	} spanhit_t;
	
	for(n=0;n<way->count_face;n++){
		vec3_t vt[3];
		vec3_t vt1, vt2, vt3;
		float minz, maxz;
		float minx, maxx;
		float miny, maxy;
		
		vt[0]=way->vertex[way->face[(n<<2)]];
		vt[1]=way->vertex[way->face[(n<<2)|1]];
		vt[2]=way->vertex[way->face[(n<<2)|2]];
		
		minz=min(vt[0].z, min(vt[1].z, vt[2].z));
		maxz=max(vt[0].z, max(vt[1].z, vt[2].z));
		
		if(from.z<minz || from.z>=maxz)
			continue;
		
		minx=min(vt[0].x, min(vt[1].x, vt[2].x));
		maxx=max(vt[0].x, max(vt[1].x, vt[2].x));
		
		if(from.x<minx || from.x>=maxx)
			continue;
		
		miny=min(vt[0].y, min(vt[1].y, vt[2].y));
		maxy=max(vt[0].y, max(vt[1].y, vt[2].y));
		if(miny>from.y)
			continue;
		if(maxy<from.y-m_wayMaxDistance)
			continue;
		
		// insertion sort
		
		if(vt[0].z>vt[1].z){
			vt1=vt[1];
			vt2=vt[0];
		}else{
			vt1=vt[0];
			vt2=vt[1];
		}
		if(vt[2].z<vt1.z){
			vt3=vt2;
			vt2=vt1;
			vt1=vt[2];
		}else if(vt[2].z<vt2.z){
			vt3=vt2;
			vt2=vt[2];
		}else{
			vt3=vt[2];
		}
		assert(vt1.z<=vt2.z);
		assert(vt2.z<=vt3.z);
		assert(vt1.x!=vt2.x || vt1.y!=vt2.y || vt1.z!=vt2.z);
		assert(vt2.x!=vt3.x || vt2.y!=vt3.y || vt2.z!=vt3.z);
		spanhit_t hit1, hit2;
		
		assert(from.z>=vt1.z);
		assert(from.z<=vt3.z);
		if(from.z<vt2.z){
			hit1.v1=vt1;
			hit1.v2=vt2;
		}else{
			hit1.v1=vt2;
			hit1.v2=vt3;
		}
		hit2.v1=vt1;
		hit2.v2=vt3;
		
		hit1.calc(from.z);
		hit2.calc(from.z);
		
		if(from.x<min(hit1.hit.x, hit2.hit.x) || from.x>=max(hit1.hit.x, hit2.hit.x))
			continue;
		
		// x-direction
		
		float per;
		vec3_t hit;
		
		per=(from.x-hit1.hit.x)/(hit2.hit.x-hit1.hit.x);
		assert(per>=-.1f);
		assert(per<1.1f);
		
		
		
		hit.x=from.x;
		hit.y=hit1.hit.y+(hit2.hit.y-hit1.hit.y)*per;
		hit.z=from.z;
		
		if(hit.y<=from.y && hit.y>=from.y-m_wayMaxDistance){
			return true;
		}
		
	}
	
	return false;
}
float map_t::getProgress(vec3_t from){
	int n;
	typedef struct{
		vec3_t v1, v2;
		float p1, p2;
		vec3_t hit;
		float p;
		float per;
		void calc(float z){
			per=(z-v1.z)/(v2.z-v1.z);
			hit.x=v1.x+(v2.x-v1.x)*per;
			hit.y=v1.y+(v2.y-v1.y)*per;
			hit.z=z;
			p=p1+(p2-p1)*per;
			
			assert(z>=min(v1.z, v2.z));
			assert(z<=max(v1.z, v2.z));
		}
	} spanhit_t;
	
	float maxy;
	float myprg;
	maxy=-100000.f;
	myprg=-1.f;
	
	for(n=0;n<way->count_face;n++){
		vec3_t vt[3];
		float p[3];
		vec3_t vt1, vt2, vt3;
		float p1, p2, p3;
		float minz, maxz;
		float minx, maxx;
		float miny;
		
		vt[0]=way->vertex[way->face[(n<<2)]];
		vt[1]=way->vertex[way->face[(n<<2)+1]];
		vt[2]=way->vertex[way->face[(n<<2)+2]];
		p[0]=way->uv[way->face[(n<<2)]].x;
		p[1]=way->uv[way->face[(n<<2)+1]].x;
		p[2]=way->uv[way->face[(n<<2)+2]].x;
		
		minz=min(vt[0].z, min(vt[1].z, vt[2].z));
		maxz=max(vt[0].z, max(vt[1].z, vt[2].z));
		
		if(from.z<minz || from.z>=maxz)
			continue;
		
		minx=min(vt[0].x, min(vt[1].x, vt[2].x));
		maxx=max(vt[0].x, max(vt[1].x, vt[2].x));
		
		if(from.x<minx || from.x>=maxx)
			continue;
		
		miny=min(vt[0].y, min(vt[1].y, vt[2].y));
		if(miny>from.y)
			continue;
		
		// insertion sort
		
		if(vt[0].z>vt[1].z){
			vt1=vt[1]; p1=p[1];
			vt2=vt[0]; p2=p[0];
		}else{
			vt1=vt[0]; p1=p[0];
			vt2=vt[1]; p2=p[1];
		}
		if(vt[2].z<vt1.z){
			vt3=vt2;	p3=p2;
			vt2=vt1;	p2=p1;
			vt1=vt[2];	p1=p[2];
		}else if(vt[2].z<vt2.z){
			vt3=vt2;	p3=p2;
			vt2=vt[2];	p2=p[2];
		}else{
			vt3=vt[2];	p3=p[2];
		}
		assert(vt1.z<=vt2.z);
		assert(vt2.z<=vt3.z);
		assert(vt1.x!=vt2.x || vt1.y!=vt2.y || vt1.z!=vt2.z);
		assert(vt2.x!=vt3.x || vt2.y!=vt3.y || vt2.z!=vt3.z);
		spanhit_t hit1, hit2;
		
		assert(from.z>=vt1.z);
		assert(from.z<=vt3.z);
		if(from.z<vt2.z){
			hit1.v1=vt1;
			hit1.v2=vt2;
			hit1.p1=p1;
			hit1.p2=p2;
		}else{
			hit1.v1=vt2;
			hit1.v2=vt3;
			hit1.p1=p2;
			hit1.p2=p3;
		}
		hit2.v1=vt1;
		hit2.v2=vt3;
		hit2.p1=p1;
		hit2.p2=p3;
		
		hit1.calc(from.z);
		hit2.calc(from.z);
		
		if(from.x<min(hit1.hit.x, hit2.hit.x) || from.x>=max(hit1.hit.x, hit2.hit.x))
			continue;
		
		// x-direction
		
		float per, pp;
		vec3_t hit;
		
		per=(from.x-hit1.hit.x)/(hit2.hit.x-hit1.hit.x);
		assert(per>=-.1f);
		assert(per<1.1f);
		
		
		
		hit.x=from.x;
		hit.y=hit1.hit.y+(hit2.hit.y-hit1.hit.y)*per;
		hit.z=from.z;
		
		pp=hit1.p+(hit2.p-hit1.p)*per;
		
		if(hit.y<from.y){
			if(hit.y>maxy){
				maxy=hit.y;
				myprg=pp;
			}
		}
		
	}
	return myprg;
}

bool map_t::isItemAvailable(int it){
	if(it>=items)
		return false;
	if(itemspawn[it]>=0.f)
		return true;
	else
		return false;
}

map_t::~map_t(){
	int n;
	
	if(lightData)
		delete lightData;
	
	glDeleteLists(list_grass, m->count_face*4);
	delete[] grassPolys;
	
	delete m;
	delete way;
	for(n=0;n<MAX_SND_ENV;n++)
		if(snd_env[n])
			; //TODO:
	
	if(tex_skybox){
		glDeleteTextures(1, &tex_skybox);
		glDeleteTextures(1, &tex_skyup);
	}

}
void M_framenext(float dt){
	int n;
	for(n=0;n<mp->items;n++){
		
			mp->itemspawn[n]+=dt;
		
	}
}
void M_cframenext(float dt){
	int n;
	for(n=0;n<MAX_SND_ENV;n++){
		if(mp->snd_env_3d[n] && mp->snd_env[n]){
			TMix_ChannelEx ex=S_calc(mp->snd_env_pos[n], mp->snd_env_vol[n]);
			ex.looped=true;
			TMix_UpdateChannelEx(mp->snd_env2[n], ex);
		}
	}
}

void M_start(){
	int n;
	for(n=0;n<MAX_SND_ENV;n++)
		if(mp->snd_env[n]){
			TMix_ChannelEx ex;
			if(mp->snd_env_3d[n]){
				ex=S_calc(mp->snd_env_pos[n], mp->snd_env_vol[n]);
				ex.looped=true;
				
			}else{
				ex.vol_left=(float)(256.f*mp->snd_env_vol[n]);
				ex.vol_right=(float)(256.f*mp->snd_env_vol[n]);
				ex.looped=true;
			}
			mp->snd_env2[n]=TMix_PlayChannelEx(-1, mp->snd_env[n], ex);
		}else{
			mp->snd_env2[n]=0;
		}
	E_addMapItems();
}

void M_stop(){
	int n;
	for(n=0;n<MAX_SND_ENV;n++)
		if(mp->snd_env2[n])
			TMix_StopChannel(mp->snd_env2[n]);
	E_removeMapItems();
}



