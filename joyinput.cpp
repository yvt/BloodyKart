/*
 *  joyinput.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/12/14.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "joyinput.h"
#include "clientgame.h"

static int joys;
static SDL_Joystick *joy[256];

// J_init - initialize joystick subsystem, and open joysticks
void J_init(){
	int n;
	joys=SDL_NumJoysticks();
	consoleLog("J_init: %d joysticks found\n", joys);
	for(n=0;n<joys;n++){
		joy[n]=SDL_JoystickOpen(n);
		consoleLog("J_init: opened joystick %d\n", n);
	}
}

// J_cframenext - retrive joystick state, updating control
void J_cframenext(float dt){
	int n;
	SDL_JoystickUpdate();
	for(n=0;n<joys;n++){
		int cx, cy;
		cx=SDL_JoystickGetAxis(joy[n], 0);
		cy=SDL_JoystickGetAxis(joy[n], 1);
		
		if(cx>=-3000 && cx<3000)cx=0;
		if(cy>=-3000 && cy<3000)cy=0;
		
		cg_steer-=(float)cx/28768.f;
		
		if(SDL_JoystickGetButton(joy[n], 0)){
			cg_accel=1;
		}
		if(SDL_JoystickGetButton(joy[n], 1)){
			cg_fire=true;
		}
		if(SDL_JoystickGetButton(joy[n], 2)){
			cg_accel=1;
		}
		if(SDL_JoystickGetButton(joy[n], 3)){
			cg_accel=-1;
		}
		if(SDL_JoystickGetButton(joy[n], 4)){
			cg_fire=true;
		}
		if(SDL_JoystickGetButton(joy[n], 5)){
			cg_fire=true;
		}
	}
}

