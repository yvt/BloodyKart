/*
 *  glpng.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/23.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */
#include "global.h"
#include "glpng.h"
#include <stdio.h>
#include <math.h>
#ifdef __MACOSX__
#include <OpenGL/glu.h>
#include </usr/local/include/png.h>
#else
#include <GL/glu.h>
#include <png.h>
#endif

static inline int conv8Bit(float v){
	int vl=(int)((v+1.f)*127.5f);
	if(vl>255)vl=255;
	if(vl<0)vl=0;
	return vl;
}
void glpngLoadTexture(const char *fn, bool mipmap, bool bump, float *ar, float *ag, float *ab, float *aa, int *rww, int *rhh){
	FILE *f;
	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 w, h;
	int bits, col, intr;
	unsigned char *img2, *img3, *img4;
	png_bytepp img1;
	long n;
	
	
	consoleLog("glpngLoadTexture: loading %s\n", fn);
	
	float bump_scale=1.f;
	
	char str[256];
	sprintf(str, "%s.ini", fn);
	
	f=fopen(str, "r");
	if(f){
		while(fgets(str, 255, f)){
			char *ptr;
			ptr=strchr(str, ' ');
			if(ptr==NULL)
				continue;
			*ptr=0;
			ptr++;
			if(!strcasecmp(str, "bumpscale")){
				bump_scale=atof(ptr);
			}else if(!strcasecmp(str, "type")){
				if(!strcasecmp(ptr, "normal")){
					bump=false;
				}else if(!strcasecmp(ptr, "heightmap")){
					bump=true;
				}
			}else if(!strcasecmp(str, "mipmap")){
				mipmap=atoi(ptr);
			}
		}
		fclose(f);
	}
	
	f=fopen(fn, "rb");
	if(f==NULL){
		throw "glpngLoadTexture: can't open png";
	}
	png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr=png_create_info_struct(png_ptr);
	png_init_io(png_ptr, f);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &w, &h, &bits, &col, &intr, NULL, NULL);
	switch(col){
		case PNG_COLOR_TYPE_RGB:
			png_set_bgr(png_ptr);
			//png_set_filler(png_ptr, 0xff, PNG_FILLER_BEFORE);
			break;
		case PNG_COLOR_TYPE_RGB_ALPHA:
			png_set_bgr(png_ptr);
			break;
		case PNG_COLOR_TYPE_PALETTE:
			png_set_palette_to_rgb(png_ptr);
			break;
		/*case PNG_COLOR_TYPE_GRAY:
			if(bits<8)png_set_gray_1_2_4_to_8(png_ptr);
			break;*/
	}
	
	if(bits==16){
		png_set_strip_16(png_ptr);
	}
	
    if (png_get_valid(png_ptr, info_ptr,
					  PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
	
	
	img1=(png_bytepp)malloc(h*sizeof(png_bytep));
	if(img1==NULL){
		throw "glpngLoadTexture: can't allocate line map";
	}
	for(n=0;n<h;n++){
		img1[n]=(png_bytep)malloc(w*4);
		if(img1[n]==NULL){
			throw "glpngLoadTexture: can't allocate line";
		}
	}
	for(n=0;n<h;n++){
		//printf("[%ld : %ld]", n, img1[n]);
	}//
	//printf("\n\n");
	png_read_image(png_ptr, img1);
	for(n=0;n<h;n++){
		//printf("[%ld : %ld]", n, img1[n]);
	}
	img3=(unsigned char *)malloc(w*h*4);
	long x,y;
	for(y=0;y<h;y++){
		for(x=0;x<w;x++){
			if(col==PNG_COLOR_TYPE_RGB){
				img3[4*(x+y*w)+3]=img1[y][x*3+2];
				img3[4*(x+y*w)+2]=img1[y][x*3+1];
				img3[4*(x+y*w)+1]=img1[y][x*3+0];
				img3[4*(x+y*w)+0]=255;
				
			}else{
				img3[4*(x+y*w)]=img1[y][x*4+3];
				img3[4*(x+y*w)+1]=img1[y][x*4+0];
				img3[4*(x+y*w)+2]=img1[y][x*4+1];
				img3[4*(x+y*w)+3]=img1[y][x*4+2];
			}
		}
	}
	for(n=0;n<h;n++)
		free(img1[n]);
	free(img1);
	float ww, hh, lg;
	long w2, h2;
	ww=w; hh=h;
	lg=log(ww)/log(2.0);
	w2=pow(2, (long)ceilf(lg));
	lg=log(hh)/log(2.0);
	h2=pow(2, (long)ceilf(lg));
	GLint mx;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mx);
	if(w2>mx)
		w2=mx;
	if(h2>mx)
		h2=mx;
	img2=(unsigned char *)malloc(w2*h2*4);
	gluScaleImage(GL_RGBA, w, h, GL_UNSIGNED_BYTE, img3, 
				  w2, h2, GL_UNSIGNED_BYTE, img2);
	free(img3);
	if(bump){
		img4=(unsigned char *)malloc(w2*h2*4);
		memcpy(img4, img2, w2*h2*4);
		int iptr=0;
		int cnt=w2*h2;
		cnt<<=2;
		int mnAlp=255;
		for(x=0;x<cnt;x+=4)
			if(img4[x+1]<mnAlp)
				mnAlp=img4[x+1];
		mnAlp=img4[1];
		float scl=-2.f/128.f;
		bump_scale=1.f/bump_scale;
		for(y=0;y<h2;y++){
			for(x=0;x<w2;x++){
				int dx=((int)(img4[iptr+1])-(int)(img4[1+((x==w2-1)?(iptr-((w2-1)<<2)):(iptr+4))]));
				int dy=((int)(img4[iptr+1])-(int)(img4[1+((y==h2-1)?(iptr-(((h2-1)*w2)<<2)):(iptr+(w2<<2)))]));
				vec3_t v;
				v.x=(float)dx*scl; v.y=(float)dy*scl;
				v.z=bump_scale;
				v=v.normalize();
				img2[iptr+3]=conv8Bit(v.x);
				img2[iptr+2]=conv8Bit(v.y);
				img2[iptr+1]=conv8Bit(v.z);
				img2[iptr]=(((int)img4[iptr+1]-mnAlp)>>1)+127;
				iptr+=4;
			}
		}
		free(img4);
	}
	if(ar||ag||ab||aa){
	int cnt=w2*h2;
	cnt<<=2;
	int aar=0, aag=0, aab=0, aaa=0;
	for(x=0;x<cnt;x+=4){
		aaa+=img2[x+0];
		aar+=(img2[x+3]*img2[x+0])>>8;
		aag+=(img2[x+2]*img2[x+0])>>8;
		aab+=(img2[x+1]*img2[x+0])>>8;
		
	}
	if(aa){
		*aa=(float)aaa/(float)(w2*h2)/255.f;
	}
	if(ar){
		*ar=(float)aar/(float)(w2*h2)/(*aa*255.f);
	}
	if(ag){
		*ag=(float)aag/(float)(w2*h2)/(*aa*255.f);
	}
	if(ab){
		*ab=(float)aab/(float)(w2*h2)/(*aa*255.f);
	}
	
		
	}
	if(rww)
		*rww=w2;
	if(rhh)
		*rhh=h2;
#undef GL_ARB_texture_compression
#if GL_ARB_texture_compression
	if(mipmap){
#if GL_SGIS_generate_mipmap
		glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE );
		glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ARB, w2, h2, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, img2);
#else
		gluBuild2DMipmaps(GL_TEXTURE_2D, GL_COMPRESSED_RGBA_ARB, w2, h2, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, img2);
#endif
	}else{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ARB, w2, h2, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, img2);
	}
	if(glGetError()==GL_INVALID_ENUM){
#endif
		if(mipmap){
#if GL_SGIS_generate_mipmap
			glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE );
			if(glGetError()==GL_INVALID_ENUM)
#endif
				gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, w2, h2, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, img2);
#if GL_SGIS_generate_mipmap
			else
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w2, h2, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, img2);
#endif
		}else{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w2, h2, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, img2);
		}
#if GL_ARB_texture_compression
	}
#endif
	free(img2);
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
	fclose(f);
	
}

