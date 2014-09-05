/*
 *  SDL_tmixer.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/28.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#ifndef _SDL_TMIXER_H
#define _SDL_TMIXER_H

#ifdef __MACOSX__
#include "SDL_mixer.h"
#else
#include <SDL/SDL_mixer.h>
#endif

static const int TMIX_DEFAULT_FREQUENCY=44100;

typedef Mix_Chunk TMix_Chunk;

struct TMix_ChannelEx{
	bool looped;
	int speed; // default=65536
	int vol_left; // def=256
	int vol_right; //def=256
	int reverb; //def=0
	int er;		//def=0;
	TMix_ChannelEx(){
		looped=false;
		speed=65536;
		vol_left=256;
		vol_right=256;
		reverb=0; er=0;
	}
};

void TMix_OpenAudio(int freq, int chunksize);
void TMix_CloseAudio();
static TMix_Chunk *TMix_LoadWAV_RW(SDL_RWops *src, int freesrc){
	return Mix_LoadWAV_RW(src, freesrc);
}
static TMix_Chunk *TMix_LoadWAV(const char *fn){
	return TMix_LoadWAV_RW(SDL_RWFromFile(fn, "rb"), 1);
}
static TMix_Chunk *TMix_QuickLoad_WAV(Uint8 *mem){
	return Mix_QuickLoad_WAV(mem);
}
static TMix_Chunk *TMix_QuickLoad_RAW(Uint8 *mem, Uint32 len){
	return Mix_QuickLoad_RAW(mem, len);
}
void TMix_AllocateChannels(int ch);
void TMix_ReserveChannels(int ch);

int TMix_PlayChannelEx(int channel, TMix_Chunk *chunk, const TMix_ChannelEx&);
static int TMix_PlayChannel(int channel, TMix_Chunk *chunk, bool looped){
	TMix_ChannelEx ex; ex.looped=looped;
	return TMix_PlayChannelEx(channel, chunk, ex);
}
void TMix_UpdateChannelEx(int chan, const TMix_ChannelEx&);
void TMix_StopChannel(int chan);
int TMix_Playing(int chan);
void TMix_Mute();
void TMix_MuteOneShot();

void TMix_ReverbLevel(int); //def=256
void TMix_ReverbTime(float); //def=1.8
void TMix_ReverbDelay(float); //def=0.
void TMix_ReverbLPF(float); //def=0.3
void TMix_ReverbAirLPF(float); //def=0.9
void TMix_ReverbHPF(float); //def=0.3
void TMix_DryLevel(float); //def=1
void TMix_GlobalSpeed(float); // def=1
void TMix_ERDistance(float); //def=1.3


#endif

