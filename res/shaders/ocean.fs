uniform sampler2D tex1, tex2, tex3;
uniform sampler2D bg;
uniform vec3 light;
uniform vec4 color;
varying vec4 eye, eye2;
varying vec3 poss, norm2;
uniform float pos;
uniform float ang;
varying float fog;

vec3 applyFog(in vec3 color, in vec3 eyePos);

vec3 oceanMap(vec2 coord){
	return texture2D(tex2, coord).xyz-vec3(.5);
}

void main()
{
	vec3 e=normalize(eye.xyz);
	vec3 r, f;
	vec4 r2, f2;
	vec3 map;
	vec4 col;
	float refs;
	//r=reflect(e, norm);
	
	map=oceanMap(gl_TexCoord[0].xy);
	map+=oceanMap(gl_TexCoord[0].xy+vec2(pos,0.));
	map+=oceanMap(gl_TexCoord[0].xy+vec2(0.,pos));
	
	map+=oceanMap(gl_TexCoord[0].xy*4.)*.5;
	map-=oceanMap(gl_TexCoord[0].xy*4.+pos*vec2(.4,.3))*.5;
	map+=oceanMap(gl_TexCoord[0].xy*4.+pos*vec2(.7,.8))*.5;
	
	map-=oceanMap(gl_TexCoord[0].xy*8.)*.2;
	map+=oceanMap(gl_TexCoord[0].xy*8.+pos*vec2(.4,.3))*.2;
	map-=oceanMap(gl_TexCoord[0].xy*8.+pos*vec2(.7,.8))*.2;
	
	map-=oceanMap(gl_TexCoord[0].xy*32.)*.2;
	map+=oceanMap(gl_TexCoord[0].xy*32.+pos*vec2(.4,.3))*.2;
	map-=oceanMap(gl_TexCoord[0].xy*32.+pos*vec2(.7,.8))*.2;
	

	map*=.26;
	f=sign(map);
	//map=((sqrt(abs(map))))*f;
	map=normalize(vec3(0., 1., 0.)+map*.06);

	col=color;
	//col.xyz*=max(0.,dot(map, light));
	map=((map));
	
	r=reflect(normalize(-eye2.xyz), map);//r+map*.1;
	f=refract(normalize(-eye2.xyz), map, 1./1.5);

	if(f==vec3(0.))
		f=r;


	r.xz=normalize(r.xz)*acos(r.y)/3.141592654*2.;
	r2=texture2D(tex3, vec2(r.x, r.z)*-.5+.5);
	r2.rgb*=2.;
	r2.rgb*=vec3(1.)+pow(max(vec3(0.), r2.rgb-vec3(1.)), vec3(2.))*7.;
	//r2.rgb+=clamp(r2.rgb-1.5, 0., .5)*10.;
	
	f.xz/=f.y/(gl_TexCoord[1].x);

	float dist=length(f);

	f.xz-=poss.xz;
	f.xz*=.3;
	f2=texture2D(tex1, f.xz)*gl_LightSource[0].diffuse;
	

	refs=-dot(map, normalize(-eye2.xyz));
	if(refs<0.)
		refs=0.;
	refs=pow(1.-refs, 3.)+.005;

	gl_FragColor = r2*refs+f2*pow(col,vec4(dist*.3))+vec4(0,0,0,1);
	gl_FragColor.xyz=applyFog(gl_FragColor.xyz, eye.xyz);
	gl_FragColor.a=1.;
}
