/*
 *  clientgame.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/30.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#pragma once

#include "cmds.h"
extern scmd_update_t sstate;
extern bool cg_running;

extern int yourClient;

extern float cg_steer;
extern int cg_accel;
extern float cg_sfps2;
extern bool cg_fire;
extern float cg_strafe;
extern int cg_vertical;
extern float cg_pitch;
extern Uint32 ocommand;
extern Uint32 cg_oHitTime;

void CG_connect(const char *addr, const char * name, const char *model);
void CG_spectate(const char *addr, const char * name, const char *model);
void CG_enter();
void CG_leave();
void CG_disconnect();
void CG_framenext(float);
void CG_screenKick(float); // for not server, but CLIENT!
