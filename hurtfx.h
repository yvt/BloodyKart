/*
 *  hurtfx.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/11/23.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#pragma once

// client only!

void H_init();
void H_cframenext(float);
void H_hurt(float amount, vec3_t pos);
void H_render(); // use in render_osd
