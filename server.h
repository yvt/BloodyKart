/*
 *  server.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/30.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#ifndef _SERVER_H
#define _SERVER_H

#include "cmds.h"

extern float sv_fps;
extern bool sv_running;
extern float sv_speedFactor;
extern float sv_speedBase;
extern int sv_resendDelay;
extern int sv_resendTimes;

extern int sv_statCmdsSent;
extern int sv_statCmdsRecv;
extern int sv_statLostCmds;
extern int sv_statBytesSent;
extern int sv_statBytesRecv;

void SV_init();
void SV_start(const char *mapname, int port=DEFAULT_PORT);
void SV_sound(const char *name, vec3_t pos, float factor=1.f, int speed=65536);
void SV_sound(const char *name, float factor=1.f, int speed=65536);
void SV_msg(const char *m, ...);
void SV_kick(int);
void SV_blood(vec3_t);
void SV_bloodBig(vec3_t);
void SV_explode(vec3_t);
void SV_explodeBig(vec3_t);
void SV_bulletImpact(vec3_t, vec3_t);
void SV_screenKick(float);
void SV_hurt(int client, float amount, vec3_t from);
void SV_fired(int);
void SV_hit(int cli, int target);
void SV_fraggedMsg(int cli, const char *, ...);
void SV_logStat();
void SV_msgStat();

#endif

