/*
 *  ai.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/27.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#ifndef _AI_H
#define _AI_H

#include "client.h"

class ai_t : public controller_t{
protected:
	float steer;
	Uint32 ohit;
	Uint32 backstart;
	Uint32 stopStart; // because of hit to front client
	float itemFindTime;
	float fireTime;
	bool firing;
	bool likeFiring;
	float dotFactor;
	
	float curPrg;
	
	int goTarget;
	vec3_t targetPos;
	int targetItem;
	int targetClient;
	float targetTime;
	
	float getFrontSpeed();
	
protected:
	
	bool isFoward(vec3_t v);
	void calcTarget();
	
	
public:
	float mindist;
	ai_t();
	virtual void bind(client_t * c);
	virtual void framenext(float dt);
	virtual float get_steering();
	virtual int get_accel();
	virtual void hit(vec3_t);
	virtual bool fire();
	virtual void clientHit(int by, vec3_t pos);
};

#endif
