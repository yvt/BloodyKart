/*
 *  noise.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/12/14.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "noise.h"

static float noiseTable[0x100];
static int32_t noisePerm[0x100];

// shuffleIndex - shuffle the index
static inline int32_t shuffleIndex(int32_t index){
	return noisePerm[index&0xff];
}

// noiseIndex - calculate index from 4d coordinate
static inline int32_t noiseIndex(int32_t x, int32_t y, int32_t z, int32_t t){
	return shuffleIndex(x+shuffleIndex(y+shuffleIndex(z+shuffleIndex(t))));
}

// noiseValue - retrive noise table from 4d coordinate
static inline float noiseValue(int32_t x, int32_t y, int32_t z, int32_t t){
	return noiseTable[noiseIndex(x, y, z, t)];
}

// noiseMix - 
static inline float noiseMix(float a, float b, float per){
	return a*(1.f-per)+b*per;
}

void N_init(){
	int32_t n;
	for(n=0;n<0x100;n++){
		noiseTable[n]=rnd()*2.f-1.f;
		noisePerm[n]=n;
	}
	random_shuffle(noisePerm, noisePerm+0x100);
}

float N_getNoise4f(float x, float y, float z, float t){
	int32_t ix, iy, iz, it;
	float fx, fy, fz, ft;
	int32_t i;
	float data[2][4];
	float value[2];
	
	ix=(int32_t)x; iy=(int32_t)y; iz=(int32_t)z; it=(int32_t)t;
	fx=x-(float)ix; fy=y-(float)iy;
	fz=z-(float)iz; ft=t-(float)it;
	
	for(i=0;i<2;i++){
		data[0][0]=noiseValue(ix, iy, iz, it+i);
		data[0][1]=noiseValue(ix+1, iy, iz, it+i);
		data[0][2]=noiseValue(ix, iy+1, iz, it+i);
		data[0][3]=noiseValue(ix+1, iy+1, iz, it+i);
		data[1][0]=noiseValue(ix, iy, iz+1, it+i);
		data[1][1]=noiseValue(ix+1, iy, iz+1, it+i);
		data[1][2]=noiseValue(ix, iy+1, iz+1, it+i);
		data[1][3]=noiseValue(ix+1, iy+1, iz+1, it+i);
		value[i]=noiseMix(noiseMix(noiseMix(data[0][0], data[0][1], fx), noiseMix(data[0][2], data[0][3], fx), fy),
						  noiseMix(noiseMix(data[1][0], data[1][1], fx), noiseMix(data[1][2], data[1][3], fx), fy), fz);
	}
	
	return noiseMix(value[0], value[1], ft);
	
}
