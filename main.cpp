
/* Simple program:  Create a blank window, wait for keypress, quit.

   Please see the SDL documentation for details on using the SDL API:
   /Developer/Documentation/SDL/docs.html
*/

#ifdef WIN32
#include <windows.h>
#endif
#include "global.h"
#include "client.h"
#include "skybox.h"
#include "map.h"
#include "font.h"
#include "ai.h"
#include "sfx.h"
#include "bloom.h"
#include "dof.h"
#include "server.h"
#include "clientgame.h"
#include "blur.h"
#include "weapon.h"
#include "mesh.h"
#include "effect.h"
#include "gigen.h"
#include "hurtfx.h"
#include "clienthud.h"
#include "tonemap.h"
#include "crosshair.h"
#include "scenetree.h"
#include "noise.h"
#include "glpng.h"
#include "joyinput.h"
#include "flare.h"
#include "effect.h"

char gl_ext[65536];

static float fov=90.f;
static float dofat=10.f;

static Uint8 *keys;
bool freezed=false;
bool firstPersonView=true;

void setview(float s=0.f);
scene_t *m_kart;

vec3_t avgVel=vec3_t(0.f, 0.f, 0.f);
float screenKick;
static float screenKickTime=0.f;

#if GL_ARB_shader_objects
static GLhandleARB prg_ss;
#endif

Uint32 ofMsg;
char fMsg[256];

scene_t *tux;

score_client_t scli[MAX_CLIENTS];

static GLuint tex_shadow;


void show_fraggedMsg(const char *str){
	strcpy(fMsg, str);
	ofMsg=SDL_GetTicks();
}

static float calcFov(float fOrig){
	float f;
	fOrig*=M_PI/180.f;
	f=tanf(fOrig*.5f);
	if((float)screen->h/(float)screen->w/(9.f/16.f)>1.f)
		f*=(float)screen->h/(float)screen->w/(9.f/16.f);
	f=atanf(f)*2.f;
	return f*180.f/M_PI;//(float)screen->h/(float)screen->w/(9.f/16.f);
}

void setview(float steps, bool back){
	vec3_t from, at, ang, up;
	vec3_t accel;
	vec3_t kickShift=vec3_t(N_getNoise4f(1.f, 0.f, 0.f, screenKickTime),
							N_getNoise4f(1.f, 1.f, 0.f, screenKickTime),
							N_getNoise4f(1.f, 0.f, 1.f, screenKickTime));
	int cc;
	cc=yourClient;
	
	scmd_update_client_t c=sstate.clients[cc];
	c.pos.x+=c.vel.x*steps;
	c.pos.y+=c.vel.y*steps;
	c.pos.z+=c.vel.z*steps;
	accel=vec3_t(c.vel)-avgVel;
	
	if(back){
		// back mirror
		
		ang=vec3_t(c.ang)+vec3_t(c.mang)*steps;
		// kick
		{
			vec3_t front, side, top;
			front=rotate(vec3_t(0.f, 0.f, 1.f), ang);
			side=rotate(vec3_t(1.f, 0.f, 0.f), ang);
			top=rotate(vec3_t(0.f, 1.f, 0.f), ang);
			ang.x+=vec3_t::dot(accel, front)*.03f;
			ang.z-=vec3_t::dot(accel, side)*.02f;
			if(cli[cc].is_avail()){
				ang.z+=cli[cc].dead_time;
			}
		}
		// screenKick
		ang+=kickShift*screenKick*.3f;
		from=rotate(vec3_t(.0f, .7f, -.0f), ang)+c.pos;
		at=rotate(vec3_t(.0f, .7f, -1.5f), ang)+c.pos;
		up=rotate(vec3_t(0.f, 1.f, 0.f), ang);
		
	}else if(cli[cc].spectate || (firstPersonView && !cli[cc].is_dead())){
		ang=vec3_t(c.ang)+vec3_t(c.mang)*steps;
		// kick
		{
			vec3_t front, side, top;
			front=rotate(vec3_t(0.f, 0.f, 1.f), ang);
			side=rotate(vec3_t(1.f, 0.f, 0.f), ang);
			top=rotate(vec3_t(0.f, 1.f, 0.f), ang);
			ang.x+=vec3_t::dot(accel, front)*.03f;
			ang.z-=vec3_t::dot(accel, side)*.02f;
			if(cli[cc].is_avail()){
				ang.z+=cli[cc].dead_time;
			}
		}
		// screenKick
		ang+=kickShift*screenKick*.3f;
		from=rotate(vec3_t(.0f, .7f, -.0f), ang)+c.pos;
		at=rotate(vec3_t(.0f, .7f, 1.5f), ang)+c.pos;
		up=rotate(vec3_t(0.f, 1.f, 0.f), ang);
		
	}else{
		ang=vec3_t(c.ang)+vec3_t(c.mang)*steps;
		ang.x=0.f;
		oang+=(ang.y-oang)*.1f;
		ang.y=oang;
		
		if(cli[cc].is_dead() && cli[cc].is_avail()){
			ang.z-=cli[cc].dead_time;
		}
		
		// screenKick
		ang+=kickShift*screenKick*.4f;
		
		vec3_t faring=vec3_t(0, 0, 0);
		if(cli[cc].is_dead() && !cli[cc].is_avail()){
			// after disappear
			faring=vec3_t(0.f, cli[cc].dead_time*2.f, cli[cc].dead_time*-2.5f);
		}
		
		from=rotate(vec3_t(0.f, 1.0f, -1.5f)+faring, ang)+c.pos;
		at=rotate(vec3_t(0.f, 0.5f, 0.f), ang)+c.pos;
		up=vec3_t(0.f, 1.f, 0.f);
		
		// avoid penetrating walls
		
		mesh *m=mp->m;
		vector<isect_t> *lst=m->raycast(at, from);
		if(lst->size()){
			// penetrating
			sort(lst->begin(), lst->end());
			isect_t s=(*lst)[0];
			from=at+(from-at).normalize()*(s.dist-.5f);
		}
		delete lst;
		
	}
	
	camera_from=from;
	camera_at=at;
	/*
	from=rotate(vec3_t(0.f, 1.f, -1.f), ang)+c.pos;
	at=rotate(vec3_t(0.f, 0.f, -.8f), ang)+c.pos;*/
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	
	
	if(back){
		gluPerspective(90.f, 4.f, .1f, 100000.f); // back view is h-mirrored
	}else if(g_useStereo){
		gluPerspective(calcFov(fov), -(float)screen->w/(float)screen->h*.5f, .05f, 100000.f);
	}else{
		gluPerspective(calcFov(fov), -(float)screen->w/(float)screen->h, .05f, 100000.f);
	}
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(g_stereoPos*.03f, 0.f, 0.f);
	gluLookAt(from.x, from.y, from.z, at.x, at.y, at.z,
			  up.x, up.y, up.z);
}
void setlight(){
	glEnable(GL_LIGHTING);
	vec3_t v;
	
	v=mp->ambColor;
	float ambientcolor[]={v.x, v.y, v.z, 1};
	
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientcolor);
	{
		v=mp->sunColor;
		float lightcolor[]={v.x, v.y, v.z, 1};
		
		v=mp->sunPos;
		float lightpos[]={v.x, v.y, v.z, 0};

		
		glEnable(GL_LIGHT0);
		glLightfv(GL_LIGHT0, GL_POSITION, lightpos);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, lightcolor);
		glLightfv(GL_LIGHT0, GL_SPECULAR, lightcolor);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientcolor);
	}
	{
		
		v=mp->sunColor*.3f;
		float lightcolor[]={v.x, v.y, v.z, 1};
		
		v=-mp->sunPos; v=-vec3_t(0.f, 1.f, 0.f);
		float lightpos[]={v.x, v.y, v.z, 0};
		
		
		glEnable(GL_LIGHT1);
		glLightfv(GL_LIGHT1, GL_POSITION, lightpos);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, lightcolor);
		//glLightfv(GL_LIGHT1, GL_SPECULAR, lightcolor);
		//glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientcolor);
	}
}
void render_grid(){
	float x1, x2, x;
		float z1, z2, z;
	const float range=20.f;
	const float steps=1.f;
	glDisable(GL_TEXTURE_2D);
	x1=floorf(cli[0].pos.x/steps)*steps-range; x2=x1+range*2.f;
	z1=floorf(cli[0].pos.z/steps)*steps-range; z2=z1+range*2.f;
	glDisable(GL_LIGHTING);
	glBegin(GL_LINES);
	glColor4f(1.f, .8f, .8f, 1.f);
	for(x=x1;x<=x2;x+=steps){
		glVertex3f(x, 0.f, z1);
		glVertex3f(x, 0.f, z2);
	}
	glColor4f(.8f, 1.f, .8f, 1.f);
	for(z=z1;z<=z2;z+=steps){
		glVertex3f(x1, 0.f, z);
		glVertex3f(x2, 0.f, z);
	}
	glEnd();
	glEnable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
}

int fps; Uint32 ofps;
float fps2;

void osdEnter2D(){
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(2.f/screen->w, -2.f/screen->h, 1.f);
	glTranslatef(-screen->w*.5f, -screen->h*.5f, 0.f);
}
void osdLeave2D(){
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(calcFov(90.f), (float)screen->w/(float)screen->h, .1f, 100000.f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void render_osd_msg(){
	float y;
	int n, i;
	y=0.f;//screen->h*.7f;
	i=SDL_GetTicks()-omsganim;
	if(i>0)
		i=0;
	y+=(float)i/msg_anim*18.f;
	list<msg_t>::reverse_iterator it, it2;
	
	osdLeave2D();
	
	glScalef(1.f, -1.f, -1.f);
	
	glPushMatrix();
	
	
	
	glTranslatef(-585.f, 00.f, 400.f);
	glRotatef(-35.f, 0.f, 1.f, 0.f);
	glScalef(1.f, 1.3f, 1.f);
	
	for(it=msgs.rbegin();it!=msgs.rend();it++){
		float fade;
		i=SDL_GetTicks()-it->time;
		if(i<msg_anim){
			fade=(float)i/(float)msg_anim;
		}else if(i>msg_time-msg_anim){
			fade=1.f-(float)(i-msg_time+msg_anim)/(float)msg_anim;
			
		}else{
			fade=1.f;
		}
		glColor4f(1.f, 1.f, 1.f, fade*.9f);
		font_roman36->drawhalf(it->text, 20.f, y);
		y+=18.f;
		if(fade<-.1f){
			break;
		}
	}
	if(it!=msgs.rend()){
		n=0;
		while(it!=msgs.rend()){
			it++;
			n++;
		}
		while(n--){
			msgs.erase(msgs.begin());
		}
	}
	
	glPopMatrix();
	
	glPushMatrix();
	
	glTranslatef(0.f, -150.f, 400.f);
	glRotatef(13.f, 1.f, 0.f, 0.f);
	
	do{
		float fade;
		fade=3.f-(float)(SDL_GetTicks()-ofMsg)/1000.f;
		
		if(fade>1.f)
			fade=1.f;
		if(fade<0.f)
			break;
		
		glColor4f(1.f, 1.f, 1.f, fade);
		font_roman36->draw(fMsg, -font_roman36->width(fMsg)*.5f, 0.f);
		
	}while(0);
	
	glPopMatrix();
	
	osdEnter2D();
}


void render_osd_score(){
	float y;
	int n;
	y=0.f;
	char buf[256];
	
	osdLeave2D();
	
	glScalef(1.f, -1.f, -1.f);
	
	glPushMatrix();

	
	glTranslatef(585.f, -200.f, 360.f);
	glRotatef(35.f, 0.f, 1.f, 0.f);
	glScalef(1.f, 1.3f, 1.f);
	glColor4f(1.f, 1.f, 1.f, 1.f);
	
	
	
	for(n=0;n<MAX_CLIENTS;n++){
		client_t& c=cli[n];
		score_client_t& sc=scli[n];
		if(!sc.enable)
			continue;
		glColor4f(1.f, 1.f, 1.f, sc.fade);
		y=18.f*sc.pos;
		font_roman36->drawhalf(c.name, -200.f, y);
		sprintf(buf, "%d", c.score);
		font_roman36->drawhalf(buf, -40.f, y);
	}
	
	glPopMatrix();
	
	osdEnter2D();
}
void render_map_internal(aabb_t bound, mesh *m, float scl, float ms, float mg){
	int n, i;
	int vts=m->count_face<<2;
	float sx=mg;
	float sy=(float)screen->h-mg-ms;
	sx-=((bound.minvec+bound.maxvec)*.5f).x*scl;
	sy-=((bound.minvec+bound.maxvec)*.5f).x*scl;
	sx+=ms*.5f; sy+=ms*.5f;
	for(n=0;n<vts;n+=4){
		for(i=n;i<n+3;i++){
			vec3_t v=m->vertex[m->face[i]];
			v.x=(v.x*scl)+sx;
			v.z=(v.z*scl)+sy;
			glVertex3f(v.x, v.z, 1.f);
		}
	}
}
void render_map(){
	aabb_t bound;
	glLineWidth(2.00f);
	mesh *m=mp->way;
	int n;
	for(n=0;n<m->count_vertex;n++)
		bound+=m->vertex[n];
	
	const float map_size=200.f;
	const float margin=20.f;
	
	float sw=(float)screen->w;
	float sh=(float)screen->h;
	float scl;
	scl=min(map_size/(bound.maxvec-bound.minvec).x,
			map_size/(bound.maxvec-bound.minvec).z);
	scl*=.85f;
	glDisable(GL_TEXTURE_2D);
	
	glBegin(GL_QUADS);
	glColor4f(0.f, 0.f, 0.f, .65f);
	glVertex2f(margin, sh-map_size-margin);
	glVertex2f(margin+map_size, sh-map_size-margin);
	glVertex2f(margin+map_size, sh-margin);
	glVertex2f(margin, sh-margin);
	glEnd();
	
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
	glBegin(GL_TRIANGLES);
	glColor4f(0.f, 0.f, 0.f, .5f);
	render_map_internal(bound, m, scl, map_size, margin-3.f);
	glEnd();
	
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);
	
	glBegin(GL_TRIANGLES);
	glColor4f(0.f, 1.f, 0.f, .5f);
	render_map_internal(bound, m, scl, map_size, margin);
	glEnd();
	
	float sx=margin;
	float sy=(float)screen->h-margin-map_size;
	sx-=((bound.minvec+bound.maxvec)*.5f).x*scl;
	sy-=((bound.minvec+bound.maxvec)*.5f).x*scl;
	sx+=map_size*.5f; sy+=map_size*.5f;
	
	glBegin(GL_QUADS);
	
	
	for(n=0;n<MAX_CLIENTS;n++){
		const client_t& c=cli[n];
		if(c.enable){
			vec3_t pos=c.pos;
			pos.x=(pos.x*scl)+sx;
			pos.z=(pos.z*scl)+sy;
			if(n==yourClient)
				glColor4f(0.1f, 1.f, 0.2f, 1.f);
			else
				glColor4f(1.f, 0.2f, 0.1f, 1.f);
			glVertex3f(pos.x-2.f, pos.z-2.f, 1.f);
			glVertex3f(pos.x+2.f, pos.z-2.f, 1.f);
			glVertex3f(pos.x+2.f, pos.z+2.f, 1.f);
			glVertex3f(pos.x-2.f, pos.z+2.f, 1.f);
		}
	}
	
	glEnd();
	
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
	glBegin(GL_LINE_LOOP);
	glColor4f(0.f, 1.f, 0.f, 1.f);
	glVertex2f(margin, sh-map_size-margin);
	glVertex2f(margin+map_size, sh-map_size-margin);
	glVertex2f(margin+map_size, sh-margin);
	glVertex2f(margin, sh-margin);
	glEnd();
	
	glEnable(GL_TEXTURE_2D);
}



void render_osd(){
	char buf[64];
	int elp=SDL_GetTicks()-ofps;
	int cc=yourClient;
	int n;
	
	if(elp>500){
		fps2=(float)fps/(float)elp*1000.f;
		ofps=SDL_GetTicks();
		fps=0;
	}
	fps++;
	
	
	
	
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	
	osdEnter2D();
	
	// render letter box
	
	glDisable(GL_TEXTURE_2D);
	glColor4f(0.f, 0.f, 0.f, 0.9f);
	
	glBegin(GL_QUADS);
	if((float)screen->h/(float)screen->w>9.f/16.f){
		// too high
		float nh;
		nh=(float)screen->w*9.f/16.f;
		nh=-((float)nh-(float)screen->h)*.5f;
		glVertex2f(0.f, 0.f);
		glVertex2f(screen->w, 0.f);
		glVertex2f(screen->w, nh);
		glVertex2f(0, nh);
		glVertex2f(0.f, screen->h);
		glVertex2f(screen->w, screen->h);
		glVertex2f(screen->w, screen->h-nh);
		glVertex2f(0, screen->h-nh);
	}else{
		// too wide
		float nw;
		nw=(float)screen->h*16.f/9.f;
		nw=-((float)nw-(float)screen->w)*.5f;
		glVertex2f(0.f, 0.f);
		glVertex2f(0.f, screen->h);
		glVertex2f(nw, screen->h);
		glVertex2f(nw, 0);
		glVertex2f(screen->w, 0);
		glVertex2f(screen->w, screen->h);
		glVertex2f(screen->w-nw, screen->h);
		glVertex2f(screen->w-nw, 0);
	}
	glEnd();
	
	glEnable(GL_TEXTURE_2D);
	
	CH_render();
	H_render();
	
	
	if(!cli[cc].spectate){
		CrossHair_render();
		if(!cli[cc].is_avail()){
			// black out after disappear
			glDisable(GL_TEXTURE_2D);
			glColor4f(0.f, 0.f, 0.f, 1.f-(cli[cc].dead_time-c_availTime)/(c_respawnTime-c_availTime));
			glBegin(GL_QUADS);
			glVertex2i(0, 0); glVertex2i(0, screen->h);
			glVertex2i(screen->w, screen->h); glVertex2i(screen->w, 0);
			glEnd();
			glEnable(GL_TEXTURE_2D);
		}else if(cli[cc].is_dead()){
			// gray out before disappear
			glDisable(GL_TEXTURE_2D);
			glColor4f(.5f, .5f, .5f, .5f);
			glBegin(GL_QUADS);
			glVertex2i(0, 0); glVertex2i(0, screen->h);
			glVertex2i(screen->w, screen->h); glVertex2i(screen->w, 0);
			glEnd();
			glEnable(GL_TEXTURE_2D);
		}
	
		if(cli[cc].alive_time<c_spawnTime && !cli[cc].is_dead()){
			// white out after spawn
			glDisable(GL_TEXTURE_2D);
			glColor4f(1.f, 1.f, 1.f, 1.f-(cli[cc].alive_time)/(c_spawnTime));
			glBegin(GL_QUADS);
			glVertex2i(0, 0); glVertex2i(0, screen->h);
			glVertex2i(screen->w, screen->h); glVertex2i(screen->w, 0);
			glEnd();
			glEnable(GL_TEXTURE_2D);
		}
	
	
		osdLeave2D();
		
		glScalef(1.f, -1.f, -1.f);
		
		glPushMatrix();
		
		glTranslatef(405.f, 180.f, 400.f);
		glRotatef(20.f, 0.f, 1.f, 0.f);
		
		
		
		// draw weapon Icon
		
		const float iconSize=160.f;
		
		if(cli[cc].weapon[0]){
			
			// have, or will have one
			weapon_t *w=W_find(cli[cc].weapon);
			
			if(cli[cc].has_weapon()){
				// fixed
				glBindTexture(GL_TEXTURE_2D, w->icon);
				glColor4f(1, 1, 1, .8);
				glBegin(GL_QUADS);
				glTexCoord2f(0.f, 0.f);
				glVertex2f(0.f, 0.f);
				glTexCoord2f(1.f, 0.f);
				glVertex2f(iconSize, 0.f);
				glTexCoord2f(1.f, 1.f);
				glVertex2f(iconSize, iconSize);
				glTexCoord2f(0.f, 1.f);
				glVertex2f(0.f, iconSize);
				glEnd();
				
				
				
				
			}else{
				// looks choosing (actually already chosen)
				n=W_indexOf(w); // get index
				float scr=powf((float)cli[cc].weapon_wait, 2.f)*10.f; // scroll pos
				scr+=(float)n; // shift to current weapon
				
				for(n=(int)scr-1;n<=(int)scr+1;n++){
					weapon_t *ww=W_find((n+W_count()*128)%W_count());
					float scrY=(float)n-scr;
					glColor4f(1, 1, 1, 1.f-fabs(scrY));
					scrY*=iconSize;
					glBindTexture(GL_TEXTURE_2D, ww->icon);
					
					glBegin(GL_QUADS);
					glTexCoord2f(0.f, 0.f);
					glVertex2f(0.f, scrY);
					glTexCoord2f(1.f, 0.f);
					glVertex2f(iconSize, scrY);
					glTexCoord2f(1.f, 1.f);
					glVertex2f(iconSize, scrY+iconSize);
					glTexCoord2f(0.f, 1.f);
					glVertex2f(0.f, scrY+iconSize);
					glEnd();
				}
				
			}
		}
		
		glPopMatrix();
		glPushMatrix();
		
		glTranslatef(585.f, 280.f, 400.f);
		glRotatef(20.f, 0.f, 1.f, 0.f);
		glScalef(1.f, 1.3f, 1.f);
		
		if(cli[cc].has_weapon()){
			float fade;
			fade=min(-cli[cc].weapon_wait*4.f, 1.f);
			sprintf(buf, "%d", cli[cc].ammo);
			if(cli[cc].ammo)
				glColor4f(1, 1, 1, fade);
			else
				glColor4f(1, 0, 0, fade);
			font_roman36->draw(buf, -font_roman36->width(buf), 0.f);
			
			weapon_t *w=W_find(cli[cc].weapon);
			if(w){
				fade=min(-(cli[cc].weapon_wait+.25f)*4.f, 1.f);
				glColor4f(1.f, 1.f, 1.f, fade*.8f);
				font_roman36->drawhalf(w->fullName, -font_roman36->width(w->fullName)*.5f+18.f, 39.f);
			}
		}else{
			strcpy(buf, "--");
			font_roman36->draw(buf, -font_roman36->width(buf), 0.f);
		}
		
		
		glPopMatrix();
		glPushMatrix();
		
		glTranslatef(-585.f, 280.f, 400.f);
		glRotatef(-20.f, 0.f, 1.f, 0.f);
		glScalef(1.f, 1.3f, 1.f);
		
		sprintf(buf, "%d", (int)cli[cc].health);
		glColor4f(1, 1, 1, .8);
		font_roman36->draw(buf, 0.f, 0.f);
		
		glPopMatrix();
		
		osdEnter2D();
		
	}
		
	/*
	glColor4f(1.f, 0.7f, 0.4f, 0.6f);
	sprintf(buf, "%.02f / %.02f", fps2, cg_sfps2);
	font_roman36->draw(buf, 20, 20);
	
	float w=mp->getProgress(cli[cc].pos+vec3_t(0.f, .3f, 0.f));
	if(w<-.5f)
		sprintf(buf, "Out of Route");
	else
		sprintf(buf, "%.02f %c", w*100.f, '%');
	glColor4f(1, 1, 0, .8);
	font_roman36->draw(buf, screen->w*.5f-font_roman36->width(buf)*.5f, 50);
	*/
	
	//render_map();
	
	render_osd_msg();
	render_osd_score();
	
	if(SDL_GetTicks()>ocommand+3000){
	
		sprintf(buf, "Disconnected");
		glColor4f(1, 1, 0, 1);
		font_roman36->draw(buf, (screen->w-font_roman36->width(buf))*.5f, (screen->h-36.f)*.5f);
		
	}
	
	sprintf(buf, "%d", totalPolys);
	font_roman36->drawhalf(buf, 0.f, 0.f);
	
	glEnable(GL_LIGHTING);
	
	glEnable(GL_DEPTH_TEST);
}
extern float blurdt;
GLfloat mx_old[16];

void renderScene(bool back=false){
	int n;
	g_currentlyRenderingBack=back;
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor(0.5f, 0.5f, 0.8f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_NORMALIZE);
	glAlphaFunc(GL_GREATER, 0.99);
	
	
#if GL_EXT_separate_specular_color
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_EXT,GL_SEPARATE_SPECULAR_COLOR_EXT);
#endif
	if(multiSamples!=1)
		glEnable(GL_MULTISAMPLE_ARB);
	//
	
	setview(-blurdt*.2f, back);
	if(!back)
		glGetFloatv(GL_MODELVIEW_MATRIX, mx_old);
	setview(0.f, back);
	
	// save 3D matrix for 3d osd
	if(!back){
		glGetDoublev(GL_MODELVIEW_MATRIX, osd_old_modelview);
		glGetDoublev(GL_PROJECTION_MATRIX, osd_old_proj);
	}
	
	setlight();
	
	glDisable(GL_FOG);
	glPushMatrix();
	{
		vec3_t v=camera_from;
		glTranslatef(v.x, v.y, v.z);
	}
	glDepthMask(GL_FALSE);
	SB_render();
	glDepthMask(GL_TRUE);
	glPopMatrix();
	mp->setup(camera_from, true, false);
	glDisable(GL_DEPTH_TEST);
	//render_grid();
	glEnable(GL_DEPTH_TEST);
	
	mp->render();
	//mp->renderWayWire();
	
	//
	for(n=0;n<MAX_CLIENTS;n++){
		scmd_update_client_t& c=sstate.clients[n];
		if(c.enable && cli[n].is_avail()){
			
			// Lighting
			mp->setup(vec3_t(c.pos)+vec3_t(0.f, .5f, 0.f), true, true);
			
			// Shadow
			glDisable(GL_LIGHTING);
			glDisable(GL_ALPHA_TEST);
			glDepthMask(GL_FALSE);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, tex_shadow);
			//glEnable( GL_POLYGON_OFFSET_FILL );
			//glPolygonOffset( 1.0, -4.0 );
			glBegin(GL_TRIANGLES);
			totalPolys+=E_renderMark(vec3_t(c.pos)+vec3_t(0.f, .1f, 0.f)+
									 rotate(vec3_t(0.f, 0.f, -.03f), c.ang), vec3_t(0.f, -1.f, 0.f), 
									 vec3_t(0.f, 0.f, 0.f), 1.f,
									 1.25f, -c.ang.y, tex_shadow,
									 0.f, .5f+(1.f)*.5f, 1.f, -1.f);
			glEnd();
			glEnable(GL_LIGHTING);
			glDisable(GL_POLYGON_OFFSET_FILL);
			glEnable(GL_ALPHA_TEST);
			glDepthMask(GL_TRUE);
		
			
			// Body
			glPushMatrix();
			glTranslatef(c.pos.x, c.pos.y, c.pos.z);
			
			glRotatef(-c.ang.y*180.f/M_PI, 0.f, 1.f, 0.f);
			glRotatef(-c.ang.x*180.f/M_PI, 1.f, 0.f, 0.f);
			glRotatef(-c.ang.z*180.f/M_PI, 0.f, 0.f, 1.f);
			
			
			m_kart->render(60.f-c.view_steer*30.f);
			int tm;
			tm=SDL_GetTicks()/30;
			tm=tm%120;
			if(tm>60)
				tm=120-tm;
			if(n!=yourClient || (!firstPersonView) || cli[n].is_dead())
				tux->render(0);
			
			if(cli[n].has_weapon()){
				weapon_t *w=W_find(cli[n].weapon);
				glPushMatrix();
				vec3_t accel=vec3_t(0.f);
				if(n==yourClient){
					glTranslatef(.3f, 0.4f, .0f);
					accel=vec3_t(c.vel)-avgVel;
					vec3_t front, side, upside;
					accel*=-.02f;
					front=rotate(vec3_t(0.f, 0.f, 1.f), cli[n].ang);
					side=rotate(vec3_t(1.f, 0.f, 0.f), cli[n].ang);
					upside=rotate(vec3_t(0.f, 1.f, 0.f), cli[n].ang);
					accel=vec3_t(vec3_t::dot(accel, side),
								 vec3_t::dot(accel, upside),
								 vec3_t::dot(accel, front));
					if(cli[n].fired){
						accel.x+=rnd()*.04f-.02f;
						accel.y+=rnd()*.04f-.02f;
					}
					glTranslatef(accel.x, accel.y, accel.z);
					if(firstPersonView){
						glTranslatef(w->fpPos.x, w->fpPos.y, w->fpPos.z);
					}
					
				}else{
					glTranslatef(.3f, 0.4f, .0f);
				}
				w->render(n, accel);
				
				glPopMatrix();
			}
			
			/*
			 if(n==yourClient)
			 glColor4f(0.1f, 1.f, 0.2f, 1.f);
			 else
			 glColor4f(1.f,0.2f, 0.1f, 1.f);
			 glDisable(GL_LIGHTING);
			 glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, color4f(.8f, .8f, .8f, .5f));
			 glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color4f(0.f, 0.f, 0.f, 1.f));
			 glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, color4f(0.f, 0.f, 0.f, 1.f));
			 glDisable(GL_TEXTURE_2D);
			 glTranslatef(0.f, .5f, 0.f);
			 glLineWidth(10.f);
			 glBlendFunc(GL_ONE,GL_ONE);
			 for(int c=0;c<1;c++){
			 glBegin(GL_LINE_STRIP);
			 glNormal3f(0.f, 1.f, 0.f);
			 glVertex3f(-.5f, -.5f, -.5f);
			 glVertex3f(.5f, -.5f, -.5f);
			 glVertex3f(.5f, -.5f, .5f);
			 glVertex3f(-.5f, -.5f, .5f);
			 glVertex3f(-.5f, -.5f, -.5f);
			 
			 glVertex3f(-.5f, .5f, -.5f);
			 glVertex3f(.5f, .5f, -.5f);
			 glVertex3f(.5f, .5f, .5f);
			 glVertex3f(-.5f, .5f, .5f);
			 glVertex3f(-.5f, .5f, -.5f);
			 glEnd();
			 glBegin(GL_LINES);
			 glVertex3f(.5f, -.5f, -.5f);
			 glVertex3f(.5f, .5f, -.5f);
			 glVertex3f(.5f, -.5f, .5f);
			 glVertex3f(.5f, .5f, .5f);
			 glVertex3f(-.5f, -.5f, .5f);
			 glVertex3f(-.5f, .5f, .5f);
			 glEnd();
			 }
			 //glutSolidCube(.5f);
			 glEnable(GL_LIGHTING);
			 
			 glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
			 glEnable(GL_TEXTURE_2D);*/
			glPopMatrix();
			
		}
	}
	glDepthMask(GL_FALSE);
	if((true || !firstPersonView) && false){
		//raycast test
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		vec3_t v1, v2;
		client_t& c=cli[yourClient];
		v1=c.pos+rotate(vec3_t(.2f, 0.5f, -.2f), c.ang); 
		v2=c.pos+rotate(vec3_t(.2f, 0.5f, 1000.f), c.ang);
		
		;
		
		glBlendFunc(GL_ONE,GL_ONE);
		
		
		
		vector<raycast_t> res;
		raycast(v1, v2, res);
		
		sort(res.begin(), res.end());
		
		if(res.size())
			res.erase(res.begin());
		
		if(res.size()){
			
			for(vector<raycast_t>::iterator it=res.begin();it!=res.end();it++){
				if(it->dest==HD_map || true){
					v2=it->pos;
					break;
				}
			}
			
		}
		
		glLineWidth(1.5f);
		glBegin(GL_LINES);
		glColor4f(.17f, .02f, 0.012f, 1.f);
		glVertex3f(v1.x, v1.y, v1.z);
		glVertex3f(v2.x, v2.y, v2.z);
		glEnd();
		glLineWidth(1.75f);
		glBegin(GL_LINES);
		glVertex3f(v1.x, v1.y, v1.z);
		glVertex3f(v2.x, v2.y, v2.z);
		glEnd();
		glLineWidth(2.0f);
		glBegin(GL_LINES);
		glVertex3f(v1.x, v1.y, v1.z);
		glVertex3f(v2.x, v2.y, v2.z);
		glEnd();
		glLineWidth(2.25f);
		glBegin(GL_LINES);
		glVertex3f(v1.x, v1.y, v1.z);
		glVertex3f(v2.x, v2.y, v2.z);
		glEnd();
		
		glPointSize(4.f);
		glBegin(GL_POINTS);
		glColor4f(.5f, .1f, .05f, 1.f);
		for(vector<raycast_t>::iterator it=res.begin();it!=res.end();it++){
			glVertex3f(it->pos.x, it->pos.y, it->pos.z);
			if(it->dest==HD_map || true)
				break;
		}
		glEnd();
		glPointSize(6.f);
		glBegin(GL_POINTS);
		glColor4f(.5f, .1f, .05f, 1.f);
		for(vector<raycast_t>::iterator it=res.begin();it!=res.end();it++){
			glVertex3f(it->pos.x, it->pos.y, it->pos.z);
			if(it->dest==HD_map || true)
				break;
		}
		glEnd();
		glPointSize(8.f);
		glBegin(GL_POINTS);
		glColor4f(.5f, .1f, .05f, 1.f);
		for(vector<raycast_t>::iterator it=res.begin();it!=res.end();it++){
			glVertex3f(it->pos.x, it->pos.y, it->pos.z);
			if(it->dest==HD_map || true)
				break;
		}
		glEnd();
		
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
	}
	glDepthMask(GL_TRUE);
	
	mp->setup(camera_from, true, false);
	
	glAlphaFunc(GL_GREATER, 0.01);
	glEnable(GL_ALPHA_TEST);
	
	W_render();
	
	E_render();
	
	//mp->renderWayWire();
	
	glDisable(GL_ALPHA_TEST);
}

bool allowBackMirror(){
	if(yourClient==-1)
		return false;
	client_t& c=cli[yourClient];
	return !c.is_dead();
}

void render(){
	int n;
	evenFrame=!evenFrame;
#if GL_EXT_framebuffer_object
	if(use_hdr)
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb_main);
#endif
	
	glEnable(GL_SCISSOR_TEST);
	
	totalPolys=0;
	
	if(!g_useStereo){
		glViewport(0, 0, screen->w*superSamples, screen->h*superSamples);
		glScissor(0, 0, screen->w*superSamples, screen->h*superSamples);
		g_stereoPos=0.f;
		renderScene(false);
	}else{
		glViewport(0, 0, screen->w*superSamples/2, screen->h*superSamples);
		glScissor(0, 0, screen->w*superSamples/2, screen->h*superSamples);
		g_stereoPos=-1.f;
		renderScene(false);
		glViewport(screen->w*superSamples/2, 0, screen->w*superSamples/2, screen->h*superSamples);
		glScissor(screen->w*superSamples/2, 0, screen->w*superSamples/2, screen->h*superSamples);
		g_stereoPos=1.f;
		renderScene(false);
		g_stereoPos=0.f;
		glViewport(0, 0, screen->w, screen->h);
		glScissor(0, 0, screen->w, screen->h);
		setview(0.f, false);
	}
	
	
#if GL_EXT_framebuffer_object
	
	if(use_hdr && multiSamples!=1){
		glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, fb_main); // source
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, fb_down); // dest
		glBlitFramebufferEXT(0, 0, screen->w, screen->h, 0, 0, screen->w, screen->h,
							 GL_COLOR_BUFFER_BIT, GL_LINEAR);
		//glBlitFramebufferEXT(0, 0, screen->w, screen->h, 0, 0, screen->w, screen->h,
		//					 GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb_down);
	}
#if GL_ARB_shader_objects
	if(use_hdr && superSamples!=1){
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb_down);
		
		glViewport(0, 0, screen->w, screen->h);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
	
		glBindTexture(GL_TEXTURE_2D, tex_fb);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, tex_fbdepth);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glUseProgramObjectARB(prg_ss);
		glUniform1iARB(glGetUniformLocationARB(prg_ss, "tex"),
					   0);
		glUniform1iARB(glGetUniformLocationARB(prg_ss, "texDepth"),
					   1);
		glUniform2fARB(glGetUniformLocationARB(prg_ss, "unit"),
							  1.f/(screen->w*superSamples), 1.f/(screen->h*superSamples));
		
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		glBegin(GL_QUADS);
	
		glColor4f(1, 1, 1,  .25f);
		glTexCoord2i(0, 0);
		glVertex2i(-1, -1);
		glTexCoord2i(1, 0);
		glVertex2i(1, -1);
		glTexCoord2i(1, 1);
		glVertex2i(1, 1);
		glTexCoord2i(0, 1);
		glVertex2i(-1, 1);
		
		
		glEnd();
		
		glUseProgramObjectARB(0);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glEnable(GL_LIGHTING);
		
		
		
	}
#endif
#endif
	
	if(multiSamples!=1)
		glDisable(GL_MULTISAMPLE_ARB);
	
	
	if(use_hdr){
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
		glClear(GL_COLOR_BUFFER_BIT);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
	}
	
	F_render();
	
	Blur_apply();
	if(g_backMirror && allowBackMirror()){
		int ww, hh;
		int sh;
		ww=screen->w/3;
		hh=ww/4;
		if(screen->w>screen->h/9.f*16.f){
			// too wide screen
			sh=screen->h;
		}else{
			// too narrow screen
			sh=screen->w*9/16;
		}
		sh=(screen->h+sh)/2;
		glViewport((screen->w-ww)/2, sh-hh-(hh>>2), ww, hh);
		glScissor((screen->w-ww)/2, sh-hh-(hh>>2), ww, hh);
		renderScene(true);
		
		// restore previous view
		glViewport(0, 0, screen->w, screen->h);
		glScissor(0, 0, screen->w, screen->h);
		setview(0.f, false);
	}

	B_apply();
	
	D_apply();
	
	TM_apply();
	
	
	render_osd();
	
#if GL_EXT_framebuffer_object
	
	if(use_hdr){
		

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glClearColor(0.0f, 0.0f, 0.5f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glDisable(GL_BLEND);
		
		glViewport(0, 0, screen->w, screen->h);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glScalef(2.f/screen->w, -2.f/screen->h, 1.f);
		glTranslatef(-screen->w*.5f, -screen->h*.5f, 0.f);
		
		if(multiSamples!=1 || superSamples!=1)
			glBindTexture(GL_TEXTURE_2D, tex_fbdown);
		else
			glBindTexture(GL_TEXTURE_2D, tex_fb);
		glEnable(GL_TEXTURE_2D);
		
		glBegin(GL_QUADS);
		
		
		
		glColor4f(1, 1, 1,  1.f);
		glTexCoord2i(0, 0);
		glVertex2i(0, screen->h);
		glTexCoord2i(1, 0);
		glVertex2i(screen->w, screen->h);
		glTexCoord2i(1, 1);
		glVertex2i(screen->w, 0);
		glTexCoord2i(0, 1);
		glVertex2i(0, 0);
	
		
		glEnd();
		
		
		glEnable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	}
#endif
	
	SDL_GL_SwapBuffers();
}

struct clisorter_t{
	int id;
	clisorter_t(){}
	clisorter_t(int i){id=i;}
	bool operator <(const clisorter_t& c)const{
		return cli[id].score>cli[c.id].score;
	}
};
void SCLI_framenext(float dt){
	vector<clisorter_t> inds;
	int n;
	
	for(n=0;n<MAX_CLIENTS;n++){
		client_t& c=cli[n];
		if(!c.enable)
			continue;
		inds.push_back(n);
	}
	
	sort(inds.begin(), inds.end());
	
	for(n=0;n<inds.size();n++)
		scli[inds[n].id].correctPos=(float)n;
	
	for(n=0;n<MAX_CLIENTS;n++){
		score_client_t& sc=scli[n];
		client_t& c=cli[n];
		if(c.enable && (!sc.enable)){
			// new client
			sc.fade=0.f;
			sc.enable=true;
			sc.pos=sc.correctPos;
		}else if((!c.enable) && sc.enable){
			// disconnect
			sc.fade-=dt/g_scoreFadeTime;
			sc.enable=(sc.fade<=0.f);
		}else if(sc.enable){
			// connected
			sc.fade=min(sc.fade+dt/g_scoreFadeTime, 1.f);
			if(sc.pos>sc.correctPos){
				sc.pos-=dt/g_scoreRankTime;
				if(sc.pos<sc.correctPos)
					sc.pos=sc.correctPos;
			}else{
				sc.pos+=dt/g_scoreRankTime;
				if(sc.pos>sc.correctPos)
					sc.pos=sc.correctPos;
			}
		}
	}
}

static Uint8 keysBuf[1024];

void framenext(float dt){
	
	dt*=sv_speedFactor;
	screenKickTime+=dt*20.f;
	
	int cnt=0;
	Uint8 *ptr;
	memset(keysBuf, 0, sizeof(keysBuf));
	
	
	//ptr=SDL_GetKeyboardState(&cnt);
	keys=SDL_GetKeyState(NULL);
	//memcpy(keysBuf, ptr, cnt);
	//keys=keysBuf;
	
	cg_steer=0.f; cg_accel=0; cg_fire=false;
	cg_strafe=0.f; cg_vertical=0;
	cg_pitch=0.f;
	// keyboard input
	
	
	
	if(SDL_WM_GrabInput(SDL_GRAB_QUERY)==SDL_GRAB_ON){
		int cx, cy, mx, my;
		int btn;
		cx=screen->w/2;cy=screen->h/2;
		btn=SDL_GetMouseState(&mx, &my);
		if(mx!=cx || my!=cy){
			SDL_WarpMouse(cx, cy);
		}
		cg_steer-=(float)(mx-cx)/10.f;
		cg_pitch-=(float)(my-cy)/10.f;
		if(cli[yourClient].spectate){
			if(btn&SDL_BUTTON(1)){	
				cg_vertical=-1;
			}
			if(btn&SDL_BUTTON(3)){
				cg_vertical=1;
			}
			if(btn&SDL_BUTTON(2)){
				cg_accel=1;
			}
		}else{
			if(btn&SDL_BUTTON(1)){	
				cg_fire=true;
			}
			if(btn&SDL_BUTTON(3)){
				cg_accel=1;
			}
			if(btn&SDL_BUTTON(2)){
				cg_accel=-1;
			}
		}
	}
	
	if(cli[yourClient].spectate){
		
		// special control in specating
#define SDL_GetScancodeFromKey(a) (a)
#define SDL_SCANCODE_RIGHT SDLK_RIGHT
#define SDL_SCANCODE_LEFT SDLK_LEFT
#define SDL_SCANCODE_UP SDLK_UP
#define SDL_SCANCODE_DOWN SDLK_DOWN
#define SDL_SCANCODE_LSHIFT SDLK_LSHIFT
#define SDL_SCANCODE_RSHIFT SDLK_RSHIFT
#define SDL_SCANCODE_RETURN SDLK_RETURN
		if(keys[SDL_GetScancodeFromKey(SDLK_a)])
			cg_strafe+=-1.f;
		if(keys[SDL_GetScancodeFromKey(SDLK_a)])
			cg_strafe+=1.f;
		
		if(keys[SDL_SCANCODE_RIGHT])
			cg_steer+=-1.f;
		if(keys[SDL_SCANCODE_LEFT])
			cg_steer+=1.f;
		if(keys[SDL_SCANCODE_UP])
			cg_pitch+=1.f;
		if(keys[SDL_SCANCODE_DOWN])
			cg_pitch-=1.f;
		
		if(keys[SDL_GetScancodeFromKey(SDLK_SPACE)])
			cg_vertical+=1;
		if(keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT])
			cg_vertical+=-1;
		
		if(keys[SDL_GetScancodeFromKey(SDLK_w)])
			cg_accel=1;
		else if(keys[SDL_GetScancodeFromKey(SDLK_s)])
			cg_accel=-1;
		
	}else{
	
		if(keys[SDL_GetScancodeFromKey(SDLK_a)])
			cg_steer=1.f;
		if(keys[SDL_GetScancodeFromKey(SDLK_d)])
			cg_steer=-1.f;
		
		if(keys[SDL_SCANCODE_RSHIFT] || keys[SDL_SCANCODE_LSHIFT] || keys[SDL_GetScancodeFromKey(SDLK_w)])
			cg_accel=1;
		else if(keys[SDL_SCANCODE_RETURN] || keys[SDL_GetScancodeFromKey(SDLK_s)])
			cg_accel=-1;
		
		cg_fire|=keys[SDL_GetScancodeFromKey(SDLK_SPACE)];
		
	}
	
	// joystick
	
	J_cframenext(dt);
	
	if(cg_steer<-1.f)
		cg_steer=-1.f;
	if(cg_steer>1.f)
		cg_steer=1.f;
	
	
	if(cli[yourClient].spectate){
	
		fov=100.f;
		dofat=200.f;
		
	}else{
		
		float nfov;
		nfov=90.f+28.f*fabs(cli[yourClient].now_speed)/cli[yourClient].max_speed;
		if(yourClient==-1)
			nfov=100.f;
		else{
			if(cli[yourClient].is_dead()){
				nfov+=cli[yourClient].dead_time*30.f;
			}
		}	
		fov+=(nfov-fov)*(1.f-powf(0.7f, dt));
		
		float ndof;
		ndof=30.f+350.f*fabs(cli[yourClient].now_speed)/cli[yourClient].max_speed;
		if(yourClient==-1)
			ndof=200.f;
		else{
			if(cli[yourClient].is_dead()){
				ndof/=1.f+cli[yourClient].dead_time;
			}
		}	
		dofat+=(ndof-dofat)*(1.f-powf(0.5f, dt));
		
	}
	
	avgVel+=(cli[yourClient].vel-avgVel)*(1.f-powf(.05f, dt));
	
	if(cli[yourClient].spectate){
		avgVel=cli[yourClient].vel; // no accelerating kick
	}
	
	screenKick=max(screenKick-dt, 0.f);
	
	D_at(dofat);

	
	
	CG_framenext(dt/sv_speedFactor);
	Blur_framenext(dt);
	SCLI_framenext(dt);
	F_cframenext(dt);
}

void gameStart(){
	S_start();
	M_start();
	consoleLog("gameStart: started game.\n");
	
}
void gameStop(){
	M_stop();
	S_stop();
	consoleLog("gameStop: stopped game.\n");
}
extern bool arg_server;
void mainInit(){
	glEnable(GL_TEXTURE_2D);
	consoleLog("mainInit: loading font roman36\n");
	font_roman36=new font_t("roman36");
	
	consoleLog("mainInit: loading res/sprites/shadow.png\n");
	
	glGenTextures(1, &tex_shadow);
	glBindTexture(GL_TEXTURE_2D, tex_shadow);
	glpngLoadTexture("res/sprites/shadow.png", false);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	N_init();			// noise
	Mesh_init();		// mesh
	if(!arg_server)
		J_init();		// joystick
	if(!arg_server)
		D_init();		// depth of field
	if(!arg_server)
		Blur_init();	// motion blur
	if(!arg_server)
		S_init();		// sfx
	if(!arg_server)
		B_init();		// bloom
	if(!arg_server)
		TM_init();		// tone mapping
	C_init();			// client
	M_init();			// map
	SB_init();			// skybox
	W_init();			// weapon
	if(!arg_server)
		E_init();		// effect
	if(!arg_server)
		CH_init();		// clienthud
	CrossHair_init();	// crosshair
	F_init();			// flare
	m_kart=new scene_t("res/kart/kart.x");
	tux=new scene_t("res/chars/tux.x");
	consoleLog("mainInit: completed main initializing.\n");
}
void init(){
	fps=0; fps2=60.f;
	ofps=SDL_GetTicks();
}

Uint32 timer_func(Uint32 inv, void *){
	SDL_Event ev;
	ev.type=SDL_USEREVENT;
	SDL_PushEvent(&ev);
	return inv;
}

void err_render(const char *msg, float fade){
	
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_COLOR_MATERIAL);
	glAlphaFunc(GL_GREATER, 0.01);
	glViewport(0, 0, screen->w, screen->h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(2.f/screen->w, -2.f/screen->h, 1.f);
	glTranslatef(-screen->w*.5f, -screen->h*.5f, 0.f);
	
	char buf[256];
	glColor4f(1.f, 1.f, 0.f, 1.f);
	sprintf(buf, "Error");
	font_roman36->draw(buf, ((float)screen->w-font_roman36->width(buf))*.5f, 20);
	
	glColor4f(1.f, 1.f, 1.f, fade);
	strcpy(buf, msg);
	font_roman36->drawhalf(buf, ((float)screen->w-font_roman36->width(buf)*.5f)*.5f, 80);
	
	SDL_GL_SwapBuffers();
	
}
void show_error(const char *msg){
	Uint32 ot=SDL_GetTicks();
	consoleLog("show_error: \"%s\"\n", msg);
	while(1){
		int frm;
		frm=SDL_GetTicks()-ot;
		if(frm>500)
			frm=500;
		err_render(msg, (float)frm/500.f);
		SDL_Event event;
		while(SDL_PollEvent(&event)){
			switch(event.type){
				case SDL_QUIT:
				case SDL_MOUSEBUTTONDOWN:
					return;
				case SDL_KEYDOWN:
					return;
			}
		}
	}
}

void server_render( float fade){
	int n;
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_COLOR_MATERIAL);
	glAlphaFunc(GL_GREATER, 0.01);
	glViewport(0, 0, screen->w, screen->h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(2.f/screen->w, -2.f/screen->h, 1.f);
	glTranslatef(-screen->w*.5f, -screen->h*.5f, 0.f);
	
	char buf[256];
	glColor4f(1.f, 1.f, 0.f, 1.f);
	sprintf(buf, "Server");
	font_roman36->draw(buf, ((float)screen->w-font_roman36->width(buf))*.5f, 20);
	
	glColor4f(1.f, 1.f, 1.f, fade);
	strcpy(buf, "Server is running.");
	font_roman36->drawhalf(buf, ((float)screen->w-font_roman36->width(buf)*.5f)*.5f, 80);
	
	glColor4f(1.f, 1.f, 1.f, fade);
	strcpy(buf, "Close this window or press ESC to stop this server.");
	font_roman36->drawhalf(buf, ((float)screen->w-font_roman36->width(buf)*.5f)*.5f, 80+18);
	
	
	
	int cnt=0;
	for(n=0;n<MAX_CLIENTS;n++)
		if(cli[n].enable)
			cnt++;
	
	glColor4f(1.f, 1.f, 1.f, fade);
	sprintf(buf, "%d clients are available.", cnt);
	font_roman36->drawhalf(buf, ((float)screen->w-font_roman36->width(buf)*.5f)*.5f, 80+18*3);
	
	sprintf(buf, "sv_statCmdsSent: %d", sv_statCmdsSent);
	font_roman36->drawhalf(buf, ((float)screen->w-font_roman36->width(buf)*.5f)*.5f, 80+18*5);
	sprintf(buf, "sv_statCmdsRecv: %d", sv_statCmdsRecv);
	font_roman36->drawhalf(buf, ((float)screen->w-font_roman36->width(buf)*.5f)*.5f, 80+18*6);
	sprintf(buf, "sv_statLostCmds: %d", sv_statLostCmds);
	font_roman36->drawhalf(buf, ((float)screen->w-font_roman36->width(buf)*.5f)*.5f, 80+18*7);
	sprintf(buf, "sv_statBytesSent: %d", sv_statBytesSent);
	font_roman36->drawhalf(buf, ((float)screen->w-font_roman36->width(buf)*.5f)*.5f, 80+18*8);
	sprintf(buf, "sv_statBytesRecv: %d", sv_statBytesRecv);
	font_roman36->drawhalf(buf, ((float)screen->w-font_roman36->width(buf)*.5f)*.5f, 80+18*9);
	
	SDL_GL_SwapBuffers();
	
}

void server_display(){
	Uint32 ot=SDL_GetTicks();
	while(1){
		int frm;
		frm=SDL_GetTicks()-ot;
		if(frm>500)
			frm=500;
		server_render( (float)frm/500.f);
		SDL_Event event;
		while(SDL_PollEvent(&event)){
			switch(event.type){
				case SDL_QUIT:
					return;
				case SDL_KEYDOWN:
					if(event.key.keysym.sym==SDLK_ESCAPE)
						return;
			}
		}
		SDL_Delay(50);
	}
}

// create_hdr_buffer - make sure whether HDR is supported, and initialize HDR

#if GL_EXT_framebuffer_object && GL_ARB_texture_float

void create_hdr_buffer(){
	
	
	
	use_hdr=false;
	
	if(multiSamples!=1){
		consoleLog("create_hdr_buffer: warning - multiSamples with HDR and depth textures causes driver failure\n");
		//return;
	}
	
#ifdef WIN32
	if(!GLEW_EXT_framebuffer_object){
		consoleLog("create_hdr_buffer: EXT_framebuffer_object is compiled but unavailable, disabling\n");
		return;
	}
#endif
	if(!strstr(gl_ext, "GL_EXT_framebuffer_object")){
		consoleLog("create_hdr_buffer: EXT_framebuffer_object is compiled but unavailable, disabling\n");
		return;
	}
	
	if(multiSamples!=1){
	
		consoleLog("create_hdr_buffer: allocating HDR framebuffer (multisample)\n");
		GLuint rb_main, rb_depth;
		glGenRenderbuffersEXT(1, &rb_main);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rb_main);
		glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, multiSamples, GL_RGBA16F_ARB, screen->w, screen->h);
		
		glGenRenderbuffersEXT(1, &rb_depth);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rb_depth);
		glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, multiSamples, GL_DEPTH_COMPONENT24_ARB, screen->w, screen->h);
		
		
		consoleLog("create_hdr_buffer: building framebuffer\n");
		glGenFramebuffersEXT(1, &fb_main);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb_main);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
									 GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, rb_main);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
									 GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rb_depth);
	
		consoleLog("create_hdr_buffer: allocating HDR framebuffer\n");
		glGenFramebuffersEXT(1, &fb_down);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb_down);
		
		glGenTextures(1, &tex_fbdown);
		glBindTexture(GL_TEXTURE_2D, tex_fbdown);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, 
					 screen->w, screen->h, 0, GL_RGBA, GL_INT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
								  GL_TEXTURE_2D, tex_fbdown, 0);
	/*	
		glGenTextures(1, &tex_fbdowndepth);
		glBindTexture(GL_TEXTURE_2D, tex_fbdowndepth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24_ARB, 
					 screen->w, screen->h, 0, GL_DEPTH_COMPONENT, GL_INT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, 
								  GL_TEXTURE_2D, tex_fbdowndepth, 0);
	*/
			
	}else if(superSamples!=1){
		
		consoleLog("create_hdr_buffer: allocating HDR framebuffer (supersample)\n");
		glGenFramebuffersEXT(1, &fb_main);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb_main);
		
		glGenTextures(1, &tex_fb);
		glBindTexture(GL_TEXTURE_2D, tex_fb);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, 
					 screen->w*superSamples, screen->h*superSamples, 0, GL_RGBA, GL_INT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
								  GL_TEXTURE_2D, tex_fb, 0);
		
		glGenTextures(1, &tex_fbdepth);
		glBindTexture(GL_TEXTURE_2D, tex_fbdepth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24_ARB, 
					 screen->w*superSamples, screen->h*superSamples, 0, GL_DEPTH_COMPONENT, GL_INT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, 
								  GL_TEXTURE_2D, tex_fbdepth, 0);
		
		consoleLog("create_hdr_buffer: allocating HDR framebuffer\n");
		glGenFramebuffersEXT(1, &fb_down);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb_down);
		
		glGenTextures(1, &tex_fbdown);
		glBindTexture(GL_TEXTURE_2D, tex_fbdown);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, 
					 screen->w, screen->h, 0, GL_RGBA, GL_INT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
								  GL_TEXTURE_2D, tex_fbdown, 0);
		
		 glGenTextures(1, &tex_fbdowndepth);
		 glBindTexture(GL_TEXTURE_2D, tex_fbdowndepth);
		 glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24_ARB, 
		 screen->w, screen->h, 0, GL_DEPTH_COMPONENT, GL_INT, NULL);
		 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		 glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, 
		 GL_TEXTURE_2D, tex_fbdowndepth, 0);
		
		
		 
		
	}else{
		consoleLog("create_hdr_buffer: allocating HDR framebuffer\n");
		glGenFramebuffersEXT(1, &fb_main);
		glGenTextures(1, &tex_fb);
		glGenTextures(1, &tex_fbdepth);
		glBindTexture(GL_TEXTURE_2D, tex_fb);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F_ARB, 
					 screen->w, screen->h, 0, GL_RGBA, GL_HALF_FLOAT_ARB, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		
		consoleLog("create_hdr_buffer: allocating HDR depthbuffer\n");
		glBindTexture(GL_TEXTURE_2D, tex_fbdepth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24_ARB, 
					 screen->w, screen->h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		
		consoleLog("create_hdr_buffer: building framebuffer\n");
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb_main);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
								  GL_TEXTURE_2D, tex_fb, 0);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, 
								  GL_TEXTURE_2D, tex_fbdepth, 0);
	}
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	
	
	
	if(!glGetError()){
		use_hdr=true;
	}else{
		consoleLog("create_hdr_buffer: error occured, disabling HDR\n");
	}

}

#else

void create_hdr_buffer(){
		
	
	
}

#endif

#if GL_ARB_imaging 
#if GL_EXT_framebuffer_object

void enableToneMap(){
	cap_toneMap=false;

	if(!use_hdr){
		consoleLog("enableToneMap: HDR is unavailable, disabling\n");
		return;
	}
	if(!cap_multiTex){
		consoleLog("enableToneMap: multi-texture is unavailable, disabling\n");
		return;
	}
	if(!strstr(gl_ext, "GL_ARB_imaging")){
		consoleLog("enableToneMap: ARB_imaging is compiled but unavailable, disabling\n");
		return;
	}
	
	cap_toneMap=true;
	use_toneMap=true;
}

#else

void enableToneMap(){
	cap_toneMap=false;

}

#endif
#else

void enableToneMap(){
	cap_toneMap=false;
	
}

#endif

// enableGLSL - make sure whether GLSL is usable

#if GL_ARB_shader_objects

void enableGLSL(){
	
	cap_glsl=false;
#ifdef WIN32
	if(!GLEW_ARB_shader_objects){
		consoleLog("enableGLSL: ARB_shader_objects is compiled but unavailable, disabling\n");
		return;
	}
	if(!GLEW_ARB_vertex_shader){
		consoleLog("enableGLSL: ARB_vertex_shader is compiled but unavailable, disabling\n");
		return;
	}
	if(!GLEW_ARB_fragment_shader){
		consoleLog("enableGLSL: ARB_fragment_shader is compiled but unavailable, disabling\n");
		return;
	}
#endif
	
	if(!strstr(gl_ext, "GL_ARB_shader_objects")){
		consoleLog("enableGLSL: ARB_shader_objects is compiled but unavailable, disabling\n");
		return;
	}
	if(!strstr(gl_ext, "GL_ARB_vertex_shader")){
		consoleLog("enableGLSL: ARB_vertex_shader is compiled but unavailable, disabling\n");
		return;
	}
	if(!strstr(gl_ext, "GL_ARB_fragment_shader")){
		consoleLog("enableGLSL: ARB_fragment_shader is compiled but unavailable, disabling\n");
		return;
	}
	
	cap_glsl=true;
	use_glsl=true;
	consoleLog("enableGLSL: GLSL enabled\n");
	
}

#else

void enableGLSL(){
	
	cap_glsl=false;
	consoleLog("enableGLSL: not compiled in, disabling GLSL\n");
	
}

#endif

void enableSuperSamplesBefore(){
	if(superSamples==1)
		return;
	
	if(!cap_glsl){
		consoleLog("enableSuperSamples: doesn't support GLSL, disabling supersampling\n");
		superSamples=1;
		return;
	}
}

void enableSuperSamples(){

	if(superSamples==1)
		return;
	

	
	if(!use_hdr){
		consoleLog("enableSuperSamples: doesn't support HDR, disabling supersampling\n");
		superSamples=1;
		return;
	}
#if GL_ARB_shader_objects

	prg_ss=create_program("res/shaders/ss2.vs", "res/shaders/ss2.fs");
	if(prg_ss)
		consoleLog("enableSuperSamples: compiled program \"ss2\"\n");
	else
		consoleLog("enableSuperSamples: couldn't compile program \"ss2\"\n");
#endif
}

#if GL_ARB_multitexture

void enableMultiTex(){
	
	cap_multiTex=false;
#ifdef WIN32
	if(!GLEW_ARB_multitexture){
		consoleLog("enableMultiTex: ARB_multitexture is compiled but unavailable, disabling\n");
		return;
	}
#endif
	
	if(!strstr(gl_ext, "GL_ARB_multitexture")){
		consoleLog("enableMultiTex: ARB_multitexture is compiled but unavailable, disabling\n");
		return;
	}
	
	cap_multiTex=true;
	consoleLog("enableMultiTex: multi-texture enabled\n");
	
}

#else

void enableMultiTex(){
	
	cap_multiTex=false;
	consoleLog("enableMultiTex: not compiled in, disabling multi-texture\n");
	
}

#endif


static void deleteResources(){
	GLuint n;
	consoleLog("deleteResources: removing all textures\n");
	for(n=0;n<65536;n++)
		if(glIsTexture(n))
			glDeleteTextures(1, (&n));
}

static int arg_screenW=1280;
static int arg_screenH=720;
static gg_mode_t arg_giGen=GG_none;
static const char *arg_startMap=NULL;;
       bool arg_server=false;
static const char *arg_connectTo=NULL;
static bool arg_fullScreen=false;
static bool arg_tryHDR=true;
static bool arg_tryMultiTex=true;
static bool arg_tryGLSL=true;
static bool arg_tryToneMap=true;
static bool arg_spectate=false;

void print_help(){
	
	printf("   bloodykart - BloodyKart by tcpp\n");
	printf("USAGE:\n");
	printf("\n");
	printf("--width=WIDTH, -w WIDTH\n");
	printf("    specify screen width to WIDTH\n");
	printf("\n");
	printf("--height=HEIGHT, -t HEIGHT\n");
	printf("    specify screen height to WIDTH\n");
	printf("\n");
	printf("--gigen=MODE, -g MODE\n");
	printf("    only generate GI\n");
	printf("    MODE is bgi, grd or lit\n");
	printf("\n");
	printf("--server, -s\n");
	printf("    only run as a server\n");
	printf("\n");
	printf("--map=MAPNAME, -p SAMPLES\n");
	printf("    start from MAPNAME\n");
	printf("\n");
	printf("--connect=SERVER, -c SERVER\n");
	printf("    connect to SERVER\n");
	printf("\n");
	printf("--spectate, -e\n");
	printf("    start as spectator\n");
	printf("\n");
	printf("--fullscreen, -f\n");
	printf("    fullscreen mode\n");
	printf("\n");
	printf("--nohdr, -l\n");
	printf("    don't use HDR\n");
	printf("\n");
	printf("--nomultitex, -n\n");
	printf("    don't use multi-texture\n");
	printf("\n");
	printf("--noglsl, -x\n");
	printf("    don't use GLSL\n");
	printf("\n");
	printf("--notonemap\n");
	printf("    don't use tone mapping\n");
	printf("\n");
	printf("--samples=SAMPLES, -m SAMPLES\n");
	printf("    use multisampling antialias\n");
	printf("\n");
	printf("--supersamples, -u\n");
	printf("    use 4x supersampling antialias (requires GLSL and HDR)\n");
	printf("\n");
	printf("--blood, -b\n");
	printf("    enable blood splatter\n");
	printf("\n");
	printf("--speed SCALE, -d SCALE\n");
	printf("    set speed scale\n");
	printf("\n");
	
	exit(0);
}

void parse_arg(int argc, char **argv){
	int n, i, on;
	for(n=1;n<argc;n++){
		const char *v=argv[n];
		char nm[256];
		char vl[256];
		puts(v);
		if(v[0]=='-' && v[1]=='-'){
			
			v+=2;
			if(strchr(v, '=')){
				strcpy(nm, v+2);
				*strchr(nm, '=')=0;
				strcpy(vl, strchr(v, '=')+1);
			}
			
			if(!strcasecmp(nm, "width")){
				arg_screenW=atoi(vl);
			}
			if(!strcasecmp(nm, "height")){
				arg_screenH=atoi(vl);
			}
			if(!strcasecmp(nm, "speed")){
				sv_speedBase=atof(vl);
			}
			if(!strcasecmp(nm, "gigen")){
				if(!strcasecmp(vl, "bgi"))
					arg_giGen=GG_bgi;
				else if(!strcasecmp(vl, "lit"))
					arg_giGen=GG_lit;
				else if(!strcasecmp(vl, "grd"))
					arg_giGen=GG_grd;
				else
					print_help();
			}
			if(!strcasecmp(nm, "connect")){
				char *ptr=new char[256];
				strcpy(ptr, vl);
				arg_connectTo=ptr;
			}
			if(!strcasecmp(nm, "server")){
				arg_server=true;
			}
			if(!strcasecmp(nm, "fullscreen")){
				arg_fullScreen=true;
			}
			if(!strcasecmp(nm, "nohdr")){
				arg_tryHDR=false;
			}
			if(!strcasecmp(nm, "nomultitex")){
				arg_tryMultiTex=false;
			}
			if(!strcasecmp(nm, "noglsl")){
				arg_tryGLSL=false;
			}
			if(!strcasecmp(nm, "notonemap")){
				arg_tryToneMap=false;
			}
			if(!strcasecmp(nm, "samples")){
				multiSamples=atoi(vl);
				superSamples=1;
			}
			if(!strcasecmp(nm, "supersamples")){
				superSamples=2;
				multiSamples=1;
			}
			if(!strcasecmp(nm, "spectate")){
				arg_spectate=true;
			}
			if(!strcasecmp(nm, "map")){
				char *ptr=new char[256];
				strcpy(ptr, vl);
				arg_startMap=ptr;
			}
			if(!strcasecmp(nm, "help")){
				print_help();
			}
			if(!strcasecmp(nm, "blood")){
				g_blood=true;
			}
			
		}else if(v[0]=='-'){
			on=n;
			for(i=1;v[i];i++){
				switch(v[i]){
					case 'w':
						arg_screenW=atoi(argv[n+1]);
						n++;
						break;
					case 't':
						arg_screenH=atoi(argv[n+1]);
						n++;
						break;
					case 'g':
						if(!strcasecmp(argv[n+1], "bgi"))
							arg_giGen=GG_bgi;
						else if(!strcasecmp(argv[n+1], "lit"))
							arg_giGen=GG_lit;
						else if(!strcasecmp(argv[n+1], "grd"))
							arg_giGen=GG_grd;
						else
							print_help();
						n++;
						break;
					case 'c':
						// ignore this parameter; used for some other purpose on macOS applications
						// arg_connectTo=argv[n+1];
						n++;
						break;
					case 'd':
						sv_speedBase=atof(argv[n+1]);
						n++;
						break;
					case 's':
						arg_server=true;
						break;
					case 'f':
						arg_fullScreen=true;
						break;
					case 'l':
						arg_tryHDR=false;
						break;
					case 'x':
						arg_tryGLSL=false;
						break;
					case 'm':
						multiSamples=atoi(argv[n+1]);
						superSamples=1;
						n++;
						break;
					case 'u':
						multiSamples=1;
						superSamples=2;
						break;
					case 'n':
						arg_tryMultiTex=false;
						break;
					case 'p':
						arg_startMap=argv[n+1];
						n++;
						break;
					case 'b':
						g_blood=true;
						break;
					case 'e':
						arg_spectate=true;
						break;
					case 'h':
						print_help();
					
				}
				if(n!=on)
					break;
			}
		}
	}
}

#ifdef WIN32

void parse_cmdline(char *cmdline){
	char *argv[256];
	int argc=0;
	
	if(strlen(cmdline))
		argc=1;
	
	argc++;
	
	consoleLog("parse_cmdline: parsing command-line \"%s\"\n", cmdline);
	
	char *ptr=cmdline;
	argv[0]="BloodyKart";
	argv[1]=ptr;
	
	while(*ptr){
		if(*ptr==' '){
			*ptr=0;
			argc++;
			argv[argc-1]=ptr+1;
		}
		ptr++;
	}
	
	consoleLog("parse_cmdline: argc is %d\n", argc);
	
	parse_arg(argc, argv);
}

#endif

extern "C" 
#ifdef WIN32

int __stdcall WinMain(HINSTANCE hInstance,
					  HINSTANCE hPrevInstance,
					  char *    lpCmdLine,
					  int       nCmdShow )
#else
	
	int SDL_main(int argc, char **argv)
#endif
{
	Uint32 initflags = SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_JOYSTICK|SDL_INIT_AUDIO|SDL_INIT_NOPARACHUTE;  /* See documentation for details */
	srand(time(0));
	Uint8  video_bpp = 0;
	Uint32 videoflags = SDL_OPENGL;
	int    done;
	SDL_Event event;
#ifndef WIN32
	char buf[256];
	strcpy(buf, argv[0]);
	*strrchr(buf, '/')=0;
	chdir(buf);
#endif


	sprintf(buf, "BloodyKart %s", getVersionString());
	SDL_WM_SetCaption(buf, buf);
	
#ifdef __MACOSX__
	
	chdir("../Resources");
	consoleLog("main: changed directory\n");
#endif
	
#ifndef WIN32
	parse_arg(argc, argv);
#else
	parse_cmdline(lpCmdLine);
#endif
	//	arg_giGen="block";
	/* Initialize the SDL library */
	if ( SDL_Init(initflags) < 0 ) {
		consoleLog("main: couldn't initialize SDL: %s\n",
			SDL_GetError());
		exit(1);
	}
	
	if(multiSamples!=1){
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, multiSamples);
	}
	
/* Set 640x480 video mode */
	//arg_screenW=1280;
	//arg_screenH=720;
	if(arg_fullScreen)
		videoflags|=SDL_FULLSCREEN;
	screen=SDL_SetVideoMode(arg_screenW,arg_screenH, video_bpp, videoflags);
	if (screen == NULL) {
		consoleLog("main: couldn't set %dx%dx%d video mode: %s\n",
                        arg_screenW, arg_screenH, video_bpp, SDL_GetError());
		SDL_Quit();
		exit(2);
	}
		
	atexit(deleteResources);

#ifdef WIN32
	glewInit();	
#endif
		
	strcpy(gl_ext, (const char *)glGetString(GL_EXTENSIONS));
		
	if(arg_tryMultiTex)
		enableMultiTex();
	if(arg_tryGLSL)
		enableGLSL();
	enableSuperSamplesBefore();
	if(arg_tryHDR)
		create_hdr_buffer();
	if(arg_tryToneMap)
		enableToneMap();
	enableSuperSamples();

	glFlush();
	int err;
	err=glGetError();
	SDLNet_Init();
	try{
		mainInit();
	}catch(const char *str){
		show_error(str);
		return 0;
	}
	//arg_giGen="block";	
	if(arg_giGen!=GG_none){
		consoleLog("main: starting giGen\n");
		mp=new map_t(arg_startMap?arg_startMap:"default", arg_giGen!=GG_grd);
		GG_genrate(arg_giGen);
		GG_save();
		return 0;
	}
	
	//glHint (GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
	if(arg_server){
		consoleLog("main: starting as server\n");
		SV_init();
		SV_start(arg_startMap?arg_startMap:"default");
		server_display();
		return 0;
	}else if(arg_connectTo){
		consoleLog("main: starting as client\n");
		try{
			if(arg_spectate)
				CG_spectate(arg_connectTo, "tcpp","default");
			else
				CG_connect(arg_connectTo, "tcpp","default");
		}catch(const char *str){
			show_error(str);
			return 0;
		}
	}else{
		consoleLog("main: starting as single player (client and server)\n");
		SV_init();
		SV_start(arg_startMap?arg_startMap:"default");
		SDL_Delay(500);
		
		try{
			if(arg_spectate)
				CG_spectate("127.0.0.1", "tcpp","default");
			else
				CG_connect("127.0.0.1", "tcpp","default");
		}catch(const char *str){
			show_error(str);
			return 0;
		}
	}
	
	init();
	done = 0;
	Uint32 ot;
	ot=SDL_GetTicks()-20;
	
	gameStart();
	consoleLog("main: start mainloop\n");
	//SDL_AddTimer(30, timer_func, NULL);
	while ( !done ) {

		/* Check for events */
		while ( SDL_PollEvent(&event) ) {
			switch (event.type) {

				case SDL_MOUSEMOTION:
					break;
				case SDL_MOUSEBUTTONDOWN:
					break;
				case SDL_KEYDOWN:
					if(event.key.keysym.sym==SDLK_ESCAPE)
						done=1;
					if(event.key.keysym.sym==SDLK_b){
						bloom=(bloom_method_t)((bloom+1)%min((int)BM_count, cap_glsl?4:3));
						const char *blooms[]={
							"Bloom Effect: None", "Bloom Effect: Basic",
							"Bloom Effect: Overlap", "Bloom Effect: GLSL"
						};
						show_msg(blooms[bloom]);
					}
					if(event.key.keysym.sym==SDLK_f){
						dof=(dof_method_t)((dof+1)%min((int)DM_count, cap_glsl?3:2));
						const char *blooms[]={
							"DoF: None", "DoF: Basic",
							"DoF: GLSL", 
						};
						show_msg(blooms[dof]);
					}
					if(event.key.keysym.sym==SDLK_n){
						if(blur==Blur_glsl_simple && multiSamples!=1)
							blur=(blur_t)-1;
						blur=(blur_t)((blur+1)%min((int)Blur_count, cap_glsl?4:2));
						const char *blurs[]={
							"Motion Blur: None", "Motion Blur: Basic",
							"Motion Blur: GLSL Simple", "Motion Blur: GLSL Enhanced"
						};
						show_msg(blurs[blur]);
					}
					if(event.key.keysym.sym==SDLK_m){
						g_backMirror=!g_backMirror;
						if(g_backMirror)
							show_msg("Back Mirror enabled");
						else
							show_msg("Back Mirror disabled");
					}
					if(event.key.keysym.sym==SDLK_j){
						g_useStereo=!g_useStereo;
						if(g_useStereo)
							show_msg("Stereo enabled");
						else
							show_msg("Stereo disabled");
					}
					if(event.key.keysym.sym==SDLK_h){
						firstPersonView=!firstPersonView;
						
					}
					if(event.key.keysym.sym==SDLK_g){
						map_grass=!map_grass;
						show_msg(map_grass?"Grass enabled":"Grass disabled");
					}
					if(event.key.keysym.sym==SDLK_l){
						if(cap_glsl){
							use_glsl=!use_glsl;
							show_msg(use_glsl?"GLSL enabled":"GLSL disabled");
						}else{
							show_msg("GLSL cannot be enabled");
						}
					}
					if(event.key.keysym.sym==SDLK_t){
						if(sv_running)
							SV_msgStat();
						else
							show_msg("Server is not running");
						//show_fraggedMsg("test");
					}
					if(event.key.keysym.sym==SDLK_BACKSPACE){
						freezed=!freezed;
					}	
					break;
				case SDL_QUIT:
					done = 1;
					exit(0);
					break;
				case SDL_VIDEORESIZE:
					//screen=SDL_SetVideoMode(event.resize.w,event.resize.h, video_bpp, videoflags);
					break;
				case SDL_USEREVENT:
					
					break;
				default:
					break;
			}
		}
		
		framenext(min(.2f, 1.f/fps2));
		ot=SDL_GetTicks();
		if(!freezed)
		render();
		
		//SDL_Delay(40);
	}
	exit(0);
	gameStop();
	/* Clean up the SDL library */
	SDL_Quit();
	
	return(0);
}
