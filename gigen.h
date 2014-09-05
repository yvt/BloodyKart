/*
 *  gigen.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/10/04.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#pragma once

extern bool gg_multiThread;

enum gg_mode_t{
	GG_none=0,
	GG_bgi,
	GG_lit,
	GG_grd
};

void GG_genrate(gg_mode_t mode);
void GG_save(bool temporary=false);
