/*
 *  sfx.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/28.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#ifndef _SFX_H
#define _SFX_H

#include "SDL_tmixer.h"
#define MAX_SOUNDS			256

void S_init();
void S_update();
TMix_ChannelEx S_calc(vec3_t v, float factor);
void S_start();
void S_stop();

TMix_Chunk *S_find(const char *);
void S_sound(const char *name, vec3_t pos, float factor=1.f, int speed=65536);
void S_sound(const char *name, float factor=1.f, int speed=65536);

extern TMix_Chunk *snd_slip;


#endif
