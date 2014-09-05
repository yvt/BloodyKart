/*
 *  SDL_tmixer.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/28.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"
#include "SDL_tmixer.h"

#define LOOPED_TIME -0x7fffffff

struct channel_t{
	int16_t *data;
	int samples;
	int pos_high;
	int pos_low;
	int speed;
	int vol1, vol2;
	int revsend;
	int ersend;
	bool looped;
	int delay;
	int time;
	bool operator <(const channel_t& c) const{
		return (looped?LOOPED_TIME:time)<(c.looped?LOOPED_TIME:c.time);
	}
};

static int start_chan=0;
static channel_t *chan;
static int chans;
static Uint32 omix;
static int *mixbuf=NULL;
static int *revbuf=NULL;
static int *revbuf2=NULL;
static int *erbuf=NULL;
static int mixbufsize=-1;
static int dry_level=256;

class comb_t{
protected:
	int size;
	int *buf;
	int bufpos;
	int fb;
	int fq;
	int vol;
	int damp;
	int lpf;
	bool invert;
public:
	void mute(){
		memset(buf, 0, size*sizeof(int));
		bufpos=0;
		lpf=0;
		invert=false;
	}
	void set_roomsize(float room){
		fb=(int)(256.f*powf(10.f, -3.f*((float)size*44100.f/(float)fq)/room/44100.f));
		//printf("fb: %d (%f %d %d)\n", fb, room, size, fq);
	}
	void set_damp(float d){
		damp=(int)(d*256.f);
	}
	void set_volume(int v){
		vol=v;
	}
	void set_invert(bool v){
		invert=v;
	}
	comb_t(int sz, int freq){
		size=sz;
		fq=freq;
		buf=new int[sz];
		set_roomsize(1.8f);
		set_volume(256);
		set_damp(0.9f);
		mute();
	}
	~comb_t(){
		delete[] buf;
	}

	void apply(int *dest, int *src, int samples){
		register int *buf=this->buf;
		register int pos=bufpos;
		register int size=this->size;
		register int feedback=fb;
		register int v=vol;
		register int d=damp;
		register int l=lpf;
		if(invert){
			while(samples--){
				*dest-=(buf[pos]*v)>>4;
				l+=((buf[pos]-l)*d)>>8;
				buf[pos]=*src-((l*feedback)>>8);
				
				pos++; if(pos==size){
					pos=0;
				}
				src++; dest+=2;
			};
		}else{
			while(samples--){
				*dest+=(buf[pos]*v)>>4;
				l+=((buf[pos]-l)*d)>>8;
				buf[pos]=*src-((l*feedback)>>8);
				
				pos++; if(pos==size){
					pos=0;
				}
				src++; dest+=2;
			};
		}
		bufpos=pos;
		lpf=l;
	}
	
};

class delay_t{
protected:
	int size;
	int *buf;
	int bufpos;
public:
	void mute(){
		memset(buf, 0, size*sizeof(int));
		bufpos=0;
	}
	delay_t(int sz){
		size=sz;
		buf=new int[sz];
		mute();
	}
	~delay_t(){
		delete[] buf;
	}
	void apply(int *dest, int *src, int samples){
		register int *buf=this->buf;
		register int pos=bufpos;
		register int size=this->size;
		while(samples--){
			*dest+=buf[pos];
			buf[pos]=*src;
			
			pos++; if(pos==size){
				pos=0;
			}
			src++; dest++;
		};
		bufpos=pos;
	}
	
};

template<int size>
class dyndelay_t{
protected:
	int *buf;
	int bufpos;
	int delay;
public:
	void mute(){
		memset(buf, 0, size*sizeof(int));
		bufpos=0;
	}
	dyndelay_t(){
		buf=new int[size];
		mute();
		delay=1;
	}
	~dyndelay_t(){
		delete[] buf;
	}
	void setDelay(int d){
		delay=d;
		if(delay<1)
			delay=1;
		if(delay>size-1)
			delay=size-1;
	}
	void apply(int *dest, int *src, int samples){
		register int *buf=this->buf;
		register int pos=bufpos;
	
		register int dl=delay;
		while(samples--){
			register int tmp=*src;
			*dest=buf[(pos-dl)&(size-1)];
			buf[pos]=tmp;
			
			pos++; pos&=(size-1);
			src++; dest++;
		};
		bufpos=pos;
	}
	
};

class lpf_t{
protected:
	int lpf;
	int flt;
public:
	
	lpf_t(){
		lpf=0;
	}
	void set_lpf(float v){
		flt=(int)(v*256.f);
	}
	void apply(int *src, int samples){
		register int tmp=lpf;
		register int f=flt;
		while(samples--){
			tmp+=((*src-tmp)*f)>>8;
			*src=tmp;
			src++;
		};
		lpf=tmp;
	}
};

class hpf_t{
protected:
	int hpf;
	int flt;
public:
	
	hpf_t(){
		hpf=0;
	}
	void set_hpf(float v){
		flt=(int)(v*256.f);
	}
	void apply(int *src, int samples){
		register int tmp=hpf;
		register int f=flt;
		register int tmp2;
		while(samples--){
			tmp2=*src;
			*src-=(tmp*f)>>8;
			tmp=tmp2;
			src++;
		};
		hpf=tmp;
	}
};

static const int combs=16;
static const int bases[]={
	1163, 1319, 1471, 1657, 1861, 2069, 1531, 1913,
	2339, 2417, 2521, 2683, 2803, 3067, 3253, 3407
};
static comb_t *comb[combs];
static delay_t *delay=NULL;
static lpf_t *lpf=new lpf_t();
static hpf_t *hpf=new hpf_t();
static lpf_t *erlpf=new lpf_t();
static dyndelay_t<65536> erdelay;
static int tmfreq;
static int oldMax=32768;
static float global_speed=1.f;
static float erDist=1.3f;

static void TMixer(void *, Uint8 *stream, int len){
	register int samples=len>>2;
	omix=SDL_GetTicks();
	if(samples!=mixbufsize){
		if(mixbuf){
			delete[] mixbuf;
			delete[] revbuf;
			delete[] revbuf2;
			delete[] erbuf;
		}
		mixbuf=new int[samples<<1];
		revbuf=new int[samples];
		revbuf2=new int[samples];
		erbuf=new int[samples];
		mixbufsize=samples;
	}
	memset(mixbuf, 0, sizeof(int)*samples*2);
	memset(revbuf, 0, sizeof(int)*samples);
	memset(revbuf2, 0, sizeof(int)*samples);
	memset(erbuf, 0, sizeof(int)*samples);
	int n, i;
	for(n=0;n<chans;n++){
		channel_t& c=chan[n];
		if(c.data){
			int start=c.delay;
			c.delay-=samples;
			c.time+=samples;
			if(c.delay<0)
				c.delay=0;
			if(start>=samples){
				continue;
			}
			register int16_t *data;
			register unsigned int datalen;
			register unsigned int pos1, pos2;
			register unsigned int speed;
			register int vol1, vol2;
			register int *mix=mixbuf;
			data=c.data;
			datalen=c.samples;
			pos1=c.pos_high;
			pos2=c.pos_low;
			speed=(unsigned int)((float)c.speed*global_speed);
			vol1=c.vol1;
			vol2=c.vol2;
			if(c.looped){
				for(i=start;i<samples;i++){
					mix[(i<<1)]+=((int)data[(pos1<<1)]*vol1)>>8;
					mix[(i<<1)+1]+=((int)data[(pos1<<1)+1]*vol2)>>8;
					pos2+=speed;
					pos1+=pos2>>16;
					pos2&=0xffff;
					while(pos1>=datalen)
						pos1-=datalen;
				}
			}else{
				for(i=start;i<samples;i++){
					mix[(i<<1)]+=((int)data[(pos1<<1)]*vol1)>>8;
					mix[(i<<1)+1]+=((int)data[(pos1<<1)+1]*vol2)>>8;
					pos2+=speed;
					pos1+=pos2>>16;
					pos2&=0xffff;
					if(pos1>=datalen){
						c.data=NULL;
						break;
					}
				}
			}
			if(c.revsend!=0){
				mix=revbuf;
				pos1=c.pos_high;
				pos2=c.pos_low;
				vol1+=vol2;
				vol1>>=2;
				vol1=(512*c.revsend)>>1;
				if(c.looped){
					for(i=start;i<samples;i++){
						mix[i]+=(((int)data[(pos1<<1)]+(int)data[(pos1<<1)+1])*vol1)>>12;
						pos2+=speed;
						pos1+=pos2>>16;
						pos2&=0xffff;
						while(pos1>=datalen)
							pos1-=datalen;
					}
				}else{
					for(i=start;i<samples;i++){
						mix[i]+=(((int)data[(pos1<<1)]+(int)data[(pos1<<1)+1])*vol1)>>12;
						pos2+=speed;
						pos1+=pos2>>16;
						pos2&=0xffff;
						if(pos1>=datalen){
							c.data=NULL;
							break;
						}
					}
				}
			}
			
			if(c.ersend!=0){
				mix=erbuf;
				pos1=c.pos_high;
				pos2=c.pos_low;
				vol1+=vol2;
				vol1>>=2;
				vol1=(c.ersend*(c.vol1+c.vol2))>>9;
				if(c.looped){
					for(i=start;i<samples;i++){
						mix[i]+=(((int)data[(pos1<<1)]+(int)data[(pos1<<1)+1])*vol1)>>8;
						pos2+=speed;
						pos1+=pos2>>16;
						pos2&=0xffff;
						while(pos1>=datalen)
							pos1-=datalen;
					}
				}else{
					for(i=start;i<samples;i++){
						mix[i]+=(((int)data[(pos1<<1)]+(int)data[(pos1<<1)+1])*vol1)>>8;
						pos2+=speed;
						pos1+=pos2>>16;
						pos2&=0xffff;
						if(pos1>=datalen){
							c.data=NULL;
							break;
						}
					}
				}
			}
			
			c.pos_high=pos1;
			c.pos_low=pos2;
		}
	}
	
	// calc ER
	erdelay.setDelay((int)(erDist/300.f*(float)tmfreq));
	erlpf->set_lpf(0.1f);
	erlpf->apply(erbuf, samples);
	erdelay.apply(erbuf, erbuf, samples);
	{
		register int vol;
		vol=(int)(64.f/erDist);
		if(vol>256)
			vol=256;
		for(n=0;n<samples;n++){
			register int tmp=erbuf[n];
			if(tmp<-500000)
				tmp=-500000;
			if(tmp>500000)
				tmp=500000;
			tmp=(tmp*vol)>>7;
			// limit er input
			mixbuf[(n<<1)]+=tmp;
			mixbuf[(n<<1)+1]+=tmp;
		}
	}
	
	// calc reverb
	{
		register int dl=dry_level;
		for(n=0;n<samples;n++){
			register int tmp=revbuf[n];
			if(tmp<-500000)
				tmp=-500000;
			if(tmp>500000)
				tmp=500000;
			// limit reverb input
			revbuf[n]=tmp;
			// apply dry level
			mixbuf[(n<<1)]=(mixbuf[(n<<1)]*dl)>>8;
			mixbuf[(n<<1)+1]=(mixbuf[(n<<1)+1]*dl)>>8;
		}
	}
	lpf->apply(revbuf, samples);
	hpf->apply(revbuf, samples);
	if(delay){
		delay->apply(revbuf2, revbuf, samples);
		
		for(n=0;n<combs;n++){
			comb[n]->apply(mixbuf+(n&1), revbuf2, samples);
		}
	}else{
		
		for(n=0;n<combs;n++){
			comb[n]->apply(mixbuf+(n&1), revbuf, samples);
		}
	}
	
	// autogain
	{
		register int mx, vl;
		mx=0;
		for(n=0;n<samples;n++){
			vl=mixbuf[(n<<1)];
			if(vl<0)
				vl=-vl;
			if(vl>mx)
				mx=vl;
			vl=mixbuf[(n<<1)|1];
			if(vl<0)
				vl=-vl;
			if(vl>mx)
				mx=vl;
		}
		if(mx<30000)
			mx=30000;
		if(mx>oldMax){
			oldMax=(oldMax+mx)>>1;
		}else{
			oldMax=(oldMax*63+mx)>>6;
		}
		
		// calc gain
		//mx=oldMax>>8;
		mx=(int)(256.f/(float)sqrtf(oldMax/32768.f));
		
		for(n=0;n<samples;n++){
			mixbuf[(n<<1)]=(mixbuf[(n<<1)]*mx)>>8;
			mixbuf[(n<<1)|1]=(mixbuf[(n<<1)|1]*mx)>>8;
		}
		/*
		// add hiss noise
		mx=(oldMax>>8)-2000;
		mx=(mx*500)/5;
		if(mx<0)
			mx=0;
		for(n=0;n<samples;n+=4){
			mixbuf[(n<<1)]+=mx;
			mixbuf[(n<<1)|1]-=mx;
			//mixbuf[(n<<1)|2]+=mx;
			//mixbuf[(n<<1)|3]-=mx;
			
			mixbuf[(n<<1)|4]-=mx;
			mixbuf[(n<<1)|5]+=mx;
			//mixbuf[(n<<1)|6]-=mx;
			//mixbuf[(n<<1)|7]+=mx;
		}*/
	}
	
	// mix
	{
		samples<<=1;
		register int16_t *str=(int16_t *)stream;

		for(n=0;n<samples;n++){
			register int vl;
			vl=mixbuf[n]>>1;
			vl+=(int)(*str);
			if(vl<-32768)
				vl=-32768;
			if(vl>32767)
				vl=32767;
			*str++=vl;
		}
	}
	
}

void TMix_OpenAudio(int freq, int chunksize){
	int n;
	chan=NULL;
	TMix_AllocateChannels(96);
	for(n=0;n<combs;n++){
		comb[n]=new comb_t(bases[n]>>1, freq);
		if(n&2)
			comb[n]->set_invert(n&1);
	}
	TMix_ReverbLevel(256);
	TMix_ReverbTime(1.8f);
	TMix_ReverbLPF(.3f);
	TMix_ReverbHPF(.3f);
	
	Mix_OpenAudio(freq, AUDIO_S16SYS, 2, chunksize);
	Mix_SetPostMix(TMixer, NULL);
	tmfreq=freq;

}
void TMix_CloseAudio(){
	Mix_CloseAudio();
}

void TMix_AllocateChannels(int ch){
	SDL_LockAudio();
	if(chan)
		delete[] chan;
	chan=new channel_t[ch];
	chans=ch;
	memset(chan, 0, sizeof(channel_t)*ch);
	SDL_UnlockAudio();
}
void TMix_ReserveChannels(int ch){
	start_chan=ch;
}

static int FindFreeChan(){
	int n;
	for(n=start_chan;n<chans;n++)
		if(!chan[n].data)
			return n;
	return max_element(chan+start_chan, chan+chans)-chan;
	return -1;
}

int TMix_PlayChannelEx(int channel, TMix_Chunk *chunk, const TMix_ChannelEx& ex){
	SDL_LockAudio();
	if(channel==-1)
		channel=FindFreeChan();
	if(channel==-1){
		SDL_UnlockAudio();
		return -1;
	}	
	channel_t& c=chan[channel];
	c.data=(int16_t *)chunk->abuf;
	c.samples=(chunk->alen>>2);
	c.pos_high=0;
	c.pos_low=0;
	c.speed=ex.speed;
	c.vol1=ex.vol_right;
	c.vol2=ex.vol_left;
	c.revsend=ex.reverb;
	c.ersend=ex.er;
	c.looped=ex.looped;
	c.delay=(SDL_GetTicks()-omix)*441/10;
	c.time=0;
	SDL_UnlockAudio();
	return channel;
}

void TMix_UpdateChannelEx(int channel, const TMix_ChannelEx& ex){
	SDL_LockAudio();

	channel_t& c=chan[channel];
	c.speed=ex.speed;
	c.vol1=ex.vol_right;
	c.vol2=ex.vol_left;
	c.revsend=ex.reverb;
	c.looped=ex.looped;
	c.ersend=ex.er;
	
	SDL_UnlockAudio();
	
}

void TMix_StopChannel(int ch){
	SDL_LockAudio();
	chan[ch].data=NULL;
	SDL_UnlockAudio();
}

void TMix_Mute(){
	SDL_LockAudio();
	for(int ch=0;ch<chans;ch++)
		chan[ch].data=NULL;
	SDL_UnlockAudio();
}

void TMix_MuteOneShot(){
	SDL_LockAudio();
	for(int ch=0;ch<chans;ch++)
		if(!chan[ch].looped)
			chan[ch].data=NULL;
	SDL_UnlockAudio();
}

int TMix_Playing(int ch){
	return chan[ch].data?1:0;
}

void TMix_ReverbLevel(int tm){
	int n;
	for(n=0;n<combs;n++)
		comb[n]->set_volume(tm>>2);
}
void TMix_ReverbTime(float tm){
	int n;
	for(n=0;n<combs;n++)
		comb[n]->set_roomsize(tm);
}

void TMix_ReverbDelay(float tm){
	SDL_LockAudio();
	int samp=(int)(tm*(float)tmfreq);
	if(delay){
		delete delay;
		delay=NULL;
	}
	if(samp<=0){
		
	}else{
		delay=new delay_t(samp);
	}
	SDL_UnlockAudio();
}

void TMix_ReverbLPF(float tm){
	lpf->set_lpf(tm);
}

void TMix_ReverbHPF(float tm){
	hpf->set_hpf(tm);
}

void TMix_ReverbAirLPF(float tm){
	int n;
	for(n=0;n<combs;n++)
		comb[n]->set_damp(tm);
}

void TMix_DryLevel(float vl){
	dry_level=(int)(vl*256.f);
}

void TMix_ERDistance(float vl){
	erDist=vl;
}

void TMix_GlobalSpeed(float vl){
	global_speed=vl;
}



