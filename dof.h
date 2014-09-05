/*
 *  dof.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/30.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#ifndef _DOF_H
#define _DOF_H

enum dof_method_t{
	DM_none=0,
	DM_basic,
#if GL_ARB_shader_objects
	DM_glsl,
#endif
	DM_count
};

extern dof_method_t dof;

void D_init();
void D_apply();
void D_at(float d);

#endif

