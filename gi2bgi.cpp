/*
 *  gi2bgi.cpp
 *  BloodyKart GI2BGI Converter
 *
 *  Created by tcpp on 09/11/28.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <stdint.h>

using namespace std;


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

int main(int argc, char **argv){
	if(argc!=3){
		puts("USAGE: gi2bgi INPUT.GI OUTPUT.BGI");
		return 1;
	}
	FILE *f1, *f2;
	f1=fopen(argv[1], "r");
	f2=fopen(argv[2], "wb");
	if(!f1){
		fprintf(stderr, "cannot open %s for reading\n", argv[1]);
		return 2;
	}	
	if(!f2){
		fprintf(stderr, "cannot open %s for writing\n", argv[2]);
		return 3;
	}
	
	char str[256];
	int32_t n, i;
	int32_t div, faces;
	float scl;
	
	puts("processing header...");
	
	fgets(str, 256, f1);
	div=atoi(str);
	
	fgets(str, 256, f1);
	faces=atoi(str);
	
	fgets(str, 256, f1);
	scl=atof(str); // scale (ignored)
	
	printf("division: %d\n", div);
	printf("faces: %d\n", faces);
	printf("scale(ignored): %f\n", scl);
	
	n=0xb1d11249; // magic number, must be the same as one in map.cpp
	fwrite(&n, 4, 1, f2);
	fwrite(&div, 4, 1, f2);
	fwrite(&faces, 4, 1, f2);
	
	
	int oper, per;
	oper=-1;
	i=faces*div*div;
	fwrite(&i, 4, 1, f2);
	
	for(n=0;n<i;n++){
		vec3_t gi; float shadow;
		fgets(str, 256, f1);
		gi=parse_vec3(str);
		fgets(str, 256, f1);
		shadow=atof(str);
		
		// write
		
		fwrite(&gi, 12, 1, f2);
		fwrite(&shadow, 4, 1, f2);
		
		per=(n+1)*100/i;
		if(per!=oper){
			printf("progress %d%c (%d / %d faces)\n", per, '%', ((n/div/div)+1), faces);
		}
		oper=per;
	}
	
	fclose(f1);
	fclose(f2);
	
	printf("complete\n");
	
	return 0;
	
}
