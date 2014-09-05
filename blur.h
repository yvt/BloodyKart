/*
 *  blur.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/31.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

enum blur_t{
	Blur_none=0,
	Blur_basic,
#if GL_ARB_shader_objects
	Blur_glsl_simple,
#if GL_ARB_depth_texture
	Blur_glsl_depth,
#endif
#endif
	
	Blur_count
};

extern blur_t blur;

void Blur_init();
void Blur_apply();
void Blur_framenext(float);