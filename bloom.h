/*
 *  bloom.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/30.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#ifndef _BLOOM_H
#define _BLOOM_H

enum bloom_method_t{
	BM_none=0,
	BM_basic,
	BM_overlap,
#if GL_ARB_shader_objects
	BM_glsl,
#endif
	
	BM_count
};

enum bloom_style_t{
	BS_basic=0,
	BS_cross,
	BS_color
};

enum bloom_quality_t{
	BQ_low=0,
	BQ_high
};

extern bloom_method_t bloom;
extern bloom_style_t bloom_style;
extern bloom_quality_t bloom_quality;

void B_init();
void B_apply();

#endif
