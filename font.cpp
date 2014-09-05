/*
 *  font.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/26.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "font.h"
#include "glpng.h"

int utf_bytes(unsigned char first){
	if((first&0xf8)==0xf0){
		return 4;
	}else if((first&0xf0)==0xe0){
		return 3;
	}else if((first&0xe0)==0xc0){
		return 2;
	}else if((first&0x80)==0x00){
		return 1;
	}else{
		return 0;
	}
}

font_t::font_t(const char *name){
	char fn[256];
	sprintf(fn, "res/fonts/%s.png", name);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glpngLoadTexture(fn, false);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	FILE *f;
	int ww, hh;
	sprintf(fn, "res/fonts/%s.ini", name);
	f=fopen(fn, "r");
	fscanf(f, "%d%d", &ww, &hh); // image size
	fscanf(f, "%f", &h); //font height
	
	unsigned char ch[32];
	float x; int y;
	float ox; int oy;
	ox=0; oy=0;
	while(fscanf(f, "%s%f%d", ch, &x, &y), x){
		glyph_t g;
		if(y!=oy){
			ox=0;
		}
		g.x=ox; g.y=(float)y*h;
		g.w=x-ox; g.h=h;
		oy=y;
		ox=x;
		unsigned char ch1=ch[0];
		switch(utf_bytes(ch1)){
			case 1:
				g.ch=ch[0];
				break;
			case 2:
				g.ch=ch[0];
				g.ch+=int(ch[1])<<8;
				break;
			case 3:
				g.ch=ch[0];
				g.ch+=int(ch[1])<<8;
				g.ch+=int(ch[2])<<16;
				break;
			case 4:
				g.ch=ch[0];
				g.ch+=int(ch[1])<<8;
				g.ch+=int(ch[2])<<16;
				g.ch+=int(ch[3])<<24;
				break;
			default:
				g.ch=0;
				break;
		}
		//assert(g.ch!='0');
		g.ww=g.w;
		g.x/=(float)ww; g.w/=(float)ww;
		g.y/=(float)hh; g.h/=(float)hh;
		gs.push_back(g);
	}
	
	fclose(f);
	stable_sort(gs.begin(), gs.end());
}

void font_t::draw(const char *str, float x, float y, float z){

	glBindTexture(GL_TEXTURE_2D, tex);
	
	float ox=x;
	
	glBegin(GL_QUADS);
	glNormal3f(0.f, 0.f, -1.f);
	
	for(;*str;str+=utf_bytes(*str)){
	
		if(*str==13){
			x=ox;
			y+=h;
			continue;
		}
		if(*str==' '){
			x+=h*.3f;
			continue;
		}
		
		int ch;
		unsigned char ch1=str[0];
		switch(utf_bytes(ch1)){
			case 1:
				ch=str[0];
				break;
			case 2:
				ch=str[0];
				ch+=int(str[1])<<8;
				break;
			case 3:
				ch=str[0];
				ch+=int(str[1])<<8;
				ch+=int(str[2])<<16;
				break;
			case 4:
				ch=str[0];
				ch+=int(str[1])<<8;
				ch+=int(str[2])<<16;
				ch+=int(str[3])<<24;
				break;
			default:
				ch=0;
				break;
		}
		
		vector<glyph_t>::iterator it;
		it=lower_bound(gs.begin(), gs.end(), glyph_t(ch));
		
		glyph_t& g=*it;
		
	//	if(g.ch!=ch)
	//		continue;
		
		glTexCoord2f(g.x, g.y);
		glVertex2f(x, y);
		glTexCoord2f(g.x+g.w, g.y);
		glVertex2f(x+g.ww, y);
		glTexCoord2f(g.x+g.w, g.y+g.h);
		glVertex2f(x+g.ww, y+h);
		glTexCoord2f(g.x, g.y+g.h);
		glVertex2f(x, y+h);
		totalPolys+=2;
		x+=g.ww;
		
	}
	
	glEnd();
	
}


void font_t::drawhalf(const char *str, float x, float y, float z){
	
	glBindTexture(GL_TEXTURE_2D, tex);
	
	float ox=x;
	float oy=y;
	const char *str2=str;
	
	int n;
	
	for(n=0;n<3;n++){
	
		glColorMask((n==2)?GL_TRUE:GL_FALSE,
					(n==1)?GL_TRUE:GL_FALSE,
					(n==0)?GL_TRUE:GL_FALSE,
					GL_FALSE);
		str=str2;
		glBegin(GL_QUADS);
		glNormal3f(0.f, 0.f, -1.f);
		
		x=ox;
		y=oy;
		
		for(;*str;str+=utf_bytes(*str)){
			
			if(*str==13){
				x=ox;
				y+=h;
				continue;
			}
			if(*str==' '){
				x+=h*.15f;
				continue;
			}
			
			int ch;
			unsigned char ch1=str[0];
			switch(utf_bytes(ch1)){
				case 1:
					ch=str[0];
					break;
				case 2:
					ch=str[0];
					ch+=int(str[1])<<8;
					break;
				case 3:
					ch=str[0];
					ch+=int(str[1])<<8;
					ch+=int(str[2])<<16;
					break;
				case 4:
					ch=str[0];
					ch+=int(str[1])<<8;
					ch+=int(str[2])<<16;
					ch+=int(str[3])<<24;
					break;
				default:
					ch=0;
					break;
			}
			
			vector<glyph_t>::iterator it;
			it=lower_bound(gs.begin(), gs.end(), glyph_t(ch));
			
			glyph_t& g=*it;
			
			//	if(g.ch!=ch)
			//		continue;
			
			glTexCoord2f(g.x, g.y);
			glVertex2f(x, y);
			glTexCoord2f(g.x+g.w, g.y);
			glVertex2f(x+g.ww*.5f, y);
			glTexCoord2f(g.x+g.w, g.y+g.h);
			glVertex2f(x+g.ww*.5f, y+h*.5f);
			glTexCoord2f(g.x, g.y+g.h);
			glVertex2f(x, y+h*.5f);
			totalPolys+=2;
			x+=g.ww*.5f;
			
		}
		
		glEnd();
		ox+=.3333f;
	}
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
}

float font_t::width(const char *str){
	float x, w;
	
	w=0.f;x=0.f;

	
	for(;*str;str+=utf_bytes(*str)){
		
		if(*str==13){
			x=0.f;
			
			continue;
		}
		if(*str==' '){
			x+=h*.3f;
			if(x>w)
				w=x;
			continue;
		}
		
		int ch;
		unsigned char ch1=str[0];
		switch(utf_bytes(ch1)){
			case 1:
				ch=str[0];
				break;
			case 2:
				ch=str[0];
				ch+=int(str[1])<<8;
				break;
			case 3:
				ch=str[0];
				ch+=int(str[1])<<8;
				ch+=int(str[2])<<16;
				break;
			case 4:
				ch=str[0];
				ch+=int(str[1])<<8;
				ch+=int(str[2])<<16;
				ch+=int(str[3])<<24;
				break;
			default:
				ch=0;
				break;
		}
		
		vector<glyph_t>::iterator it;
		it=lower_bound(gs.begin(), gs.end(), glyph_t(ch));
		
		glyph_t& g=*it;
		
		//	if(g.ch!=ch)
		//		continue;
		
		
		x+=g.ww;
		if(x>w)
			w=x;
		
	}
	return w;
}
