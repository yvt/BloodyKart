/*
 *  font.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/26.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#ifndef _FONT_H
#define _FONT_H

// supports UTF8

struct glyph_t{
	int ch;
	float x, y, w, h;
	float ww;
	bool operator ==(const glyph_t& g) const{
		return ch==g.ch;
	}
	bool operator <(const glyph_t& g) const{
		return ch<g.ch;
	}
	glyph_t(){}
	glyph_t(int c){ch=c;}
};

class font_t{
public:	
	GLuint tex;
	vector<glyph_t> gs;
	float h;
	font_t(const char *name);
	void draw(const char *str, float x, float y, float z=1.f);
	void drawhalf(const char *str, float x, float y, float z=1.f);
	float width(const char *str);
};

extern font_t *font_roman36;


#endif
