/*
 *  skybox.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/24.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#ifndef _SKYBOX_H
#define _SKYBOX_H

extern GLuint tex_skybox;

void SB_init();
void SB_render();
void SB_load(const char *);

#endif
