/*
 *  scenetree.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/12/12.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "scenetree.h"

/*
 *  frame_t
 */

frame_t::frame_t(scene_t *s){
	strcpy(name, "frameUnnamed");
	identityMatrix4(fMatrix);
	identityMatrix4(aMatrix);
	scene=s;
}
frame_t::frame_t(scene_t *s, XFILE f){
	char st[256];
	int n;
	
	xparserReadName(st, f);
	strcpy(name, st);
	
	identityMatrix4(fMatrix);
	identityMatrix4(aMatrix);
	scene=s;
	
	while(xparserNodeType(f)==XNODE_DATA){
		
		xparserBeginData(f);
		xparserReadID(st, f);
		
		if(!strcasecmp(st, "FrameTransformMatrix")){
			
			for(n=0;n<16;n++){
				fMatrix[n]=xparserReadValue(f);
			}
			
			xparserEndData(f);
			
		}else if(!strcasecmp(st, "Frame")){
			

			frame_t *frm=new frame_t(s, f);
			
			frm->parent=this;
			
			xparserEndData(f);
			
			children.push_back(frm);
			
		}else if(!strcasecmp(st, "Mesh")){
			
			mesh *m=new mesh(f);
			
			xparserEndData(f);
			
			meshes.push_back(m);
			
		}
		
	}
}
frame_t::~frame_t(){
	for(vector<frame_t *>::iterator it=children.begin();it!=children.end();it++){
		delete *it;
	}
	for(vector<mesh *>::iterator it=meshes.begin();it!=meshes.end();it++){
		delete *it;
	}
}
void frame_t::render(){
	float mx[16];
	multMatrix4(mx, fMatrix, aMatrix);
	glPushMatrix();
	glMultMatrixf(fMatrix);
	for(vector<mesh *>::iterator it=meshes.begin();it!=meshes.end();it++){
		mesh *m=*it;
		calcSkin(m);
		m->updateSkin();
		m->render();
	}
	for(vector<frame_t *>::iterator it=children.begin();it!=children.end();it++){
		(*it)->render();
	}
	glPopMatrix();
}
frame_t *frame_t::findByName(const char *str){
	if(!strcasecmp(name, str))
		return this;
	for(vector<frame_t *>::iterator it=children.begin();it!=children.end();it++){
		frame_t *frm;
		frm=(*it)->findByName(str);
		if(frm)
			return frm;
	}
	return NULL;
}

void frame_t::calcSkin(mesh *m){
	int n;
	float temp[16];
	for(n=0;n<m->count_bone;n++){
		frame_t *f;
		f=scene->root->findByName(m->boneName[n].c_str());
		if(f==NULL)
			continue;
		copyMatrix4(f->aMatrix, m->boneOff[n]);
	}
	for(n=0;n<m->count_bone;n++){
		frame_t *f;
		f=scene->root->findByName(m->boneName[n].c_str());
		if(f==NULL)
			continue; // invalid bone!
		f->calcFragMatrix4(temp, this);
		multMatrix4(m->boneM[n], m->boneOff[n], temp);
		//transposeMatrix4(m->boneM[n], m->boneM[n]);
	}
}

void frame_t::calcFragMatrix4(float *dest, frame_t *from){
	float temp1[16];
	float temp2[16];
	frame_t *cur=this;
	identityMatrix4(dest);
	while(cur!=from){
		copyMatrix4(temp1, cur->fMatrix);
		//inverseMatrix4(temp2);
		//multMatrix4(temp1, temp2, cur->fMatrix);
		multMatrix4(temp2, dest, temp1);
		copyMatrix4(dest, temp2);
		cur=cur->parent;
	}
}

/*
 *  animation_key_t
 */

void animation_key_t::makeMatrix4(float *dest){
	float temp1[16], temp2[16];
	if(useMatrix){
		copyMatrix4(dest, m);
		return;
	}
	scaleMatrix4(temp1, scale);
	rot.toMatrix4(temp2);
	multMatrix4(dest, temp1, temp2);
	dest[12]+=pos.x;
	dest[13]+=pos.y;
	dest[14]+=pos.z;
}

animation_key_t animation_key_t::slerp(const animation_key_t& a, const animation_key_t& b, float d){
	animation_key_t k;
	if(a.useMatrix != b.useMatrix)
		return a;
	if(a.useMatrix){
		k=a;
		int n;
		for(n=0;n<16;n++){
			k.m[n]=a.m[n]*(1.f-d)+b.m[n]*d;
		}
	}else{
		k.pos=a.pos*(1.f-d)+b.pos*d;
		k.scale=a.scale*(1.f-d)+b.pos*d;
		k.rot=quaternion_t::slerp(d, a.rot, b.rot);
	}
	return k;
}

/*
 *  animation_t
 */

animation_t::animation_t(scene_t *s, XFILE f){
	
	char st[256];
	int i, n, j;
	
	xparserReadName(st, f);
	strcpy(name, st);
	scene=s;
	
	// read target name
	
	xparserBeginData(f);
	xparserBeginData(f);
	xparserReadID(st, f);
	xparserEndData(f);
	target=s->root->findByName(st);
	if(target==NULL)
		consoleLog("animation_t: frame not found: \"%s\"\n", st);
	keys=NULL;
	start=0;
	
	while(xparserNodeType(f)==XNODE_DATA){
		
		xparserBeginData(f);
		xparserReadID(st, f);
		
		if(!strcasecmp(st, "AnimationKey")){
			
			i=(int)xparserReadValue(f);
			duration=(int)xparserReadValue(f);
			if(!keys)
				keys=new animation_key_t[duration];
			
			for(n=0;n<duration;n++){
				animation_key_t& k=keys[n];
				j=(int)xparserReadValue(f); // frame (ingore)
				xparserReadValue(f); // compnents (igored)
				if(i==0){
					quaternion_t q;
					q.w=(float)xparserReadValue(f);
					q.x=(float)xparserReadValue(f);
					q.y=(float)xparserReadValue(f);
					q.z=(float)xparserReadValue(f);
					k.rot=q;
					//consoleLog("%f %f %f %f\n", q.w, q.x, q.y, q.z);
				}else if(i==1){
					vec3_t v;
					v.x=(float)xparserReadValue(f);
					v.y=(float)xparserReadValue(f);
					v.z=(float)xparserReadValue(f);
					k.scale=v;
				}else if(i==2){
					vec3_t v;
					v.x=(float)xparserReadValue(f);
					v.y=(float)xparserReadValue(f);
					v.z=(float)xparserReadValue(f);
					k.pos=v;
				}else if(i==4){
					k.m[0]=(float)xparserReadValue(f);
					k.m[1]=(float)xparserReadValue(f);
					k.m[2]=(float)xparserReadValue(f);
					k.m[3]=(float)xparserReadValue(f);
					k.m[4]=(float)xparserReadValue(f);
					k.m[5]=(float)xparserReadValue(f);
					k.m[6]=(float)xparserReadValue(f);
					k.m[7]=(float)xparserReadValue(f);
					k.m[8]=(float)xparserReadValue(f);
					k.m[9]=(float)xparserReadValue(f);
					k.m[10]=(float)xparserReadValue(f);
					k.m[11]=(float)xparserReadValue(f);
					k.m[12]=(float)xparserReadValue(f);
					k.m[13]=(float)xparserReadValue(f);
					k.m[14]=(float)xparserReadValue(f);
					k.m[15]=(float)xparserReadValue(f);
					k.useMatrix=true;
					//consoleLog("\n%f %f %f %f\n", k.m[0], k.m[4], k.m[8], k.m[12]);
					//consoleLog("%f %f %f %f\n", k.m[1], k.m[5], k.m[9], k.m[13]);
					//consoleLog("%f %f %f %f\n", k.m[2], k.m[6], k.m[10], k.m[14]);
					//consoleLog("%f %f %f %f\n", k.m[3], k.m[7], k.m[11], k.m[15]);
				}
			}
			
			xparserEndData(f);
			
		}
		
	}
	
}
void animation_t::apply(float f){
	if(!target)
		return;
	if(!duration)
		return;
	animation_key_t key;
	int fi=(int)f;
	
	if(fi<start)
		key=keys[0];
	else if(fi>=start+duration-1)
		key=keys[duration-1];
	else{
		key=animation_key_t::slerp(keys[fi], keys[fi+1], f-(float)fi);
	}
	
	key.makeMatrix4(target->fMatrix);
}
animation_t::~animation_t(){
	if(keys)
		delete[] keys;
}

/*
 *  animation_set_t
 */

animation_set_t::animation_set_t(scene_t *s, XFILE f){
	
	char st[256];
	
	xparserReadName(st, f);
	strcpy(name, st);
	scene=s;
	
	while(xparserNodeType(f)==XNODE_DATA){
		
		xparserBeginData(f);
		xparserReadID(st, f);
		
		if(!strcasecmp(st, "Animation")){
			
			animation_t *a=new animation_t(s, f);
			anims.push_back(a);
			
			xparserEndData(f);
			
		}
		
	}
	
}
animation_set_t::~animation_set_t(){
	for(vector<animation_t *>::iterator it=anims.begin();it!=anims.end();it++){
		delete *it;
	}
}

void animation_set_t::apply(float f){
	for(vector<animation_t *>::iterator it=anims.begin();it!=anims.end();it++){
		(*it)->apply(f);
	}
}

/*
 *  scene_t
 */
scene_t::scene_t(XFILE f){
	loadScene(f);
}
scene_t::scene_t(const char *fn){
	XFILE f;
	f=xparserOpen((fn));
	if(f==NULL){
		throw "mesh: can't open mesh";
	}
	loadScene(f);
	xparserClose(f);
}
void scene_t::loadScene(XFILE f){
	
	char st[256];
	
	while(xparserNodeType(f)==XNODE_DATA){
		
		xparserBeginData(f);
		xparserReadID(st, f);
		
		if(!strcasecmp(st, "Frame")){
			
			frame_t *ff=new frame_t(this, f);
			root=ff;
			
			xparserEndData(f);
		
		}else if(!strcasecmp(st, "AnimationSet")){
			
			animation_set_t *ff=new animation_set_t(this, f);
			sets.push_back(ff);
			
			xparserEndData(f);
			
		}else if(!strcasecmp(st, "Mesh")){
			
			frame_t *ff=new frame_t(this);
			root=ff;
			
			mesh *m=new mesh(f);
			ff->meshes.push_back(m);
									
			xparserEndData(f);
			
		}else{
			xparserEndData(f);
		}
		
	}
}

scene_t::~scene_t(){
	for(vector<animation_set_t *>::iterator it=sets.begin();it!=sets.end();it++){
		delete *it;
	}
	delete root;
}

void scene_t::render(float f){
	if(sets.size())
		sets[0]->apply(f);
	root->render();
}


