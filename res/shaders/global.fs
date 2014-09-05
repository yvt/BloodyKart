
// global Frag Shader



#if 0

vec3 slowApplyFog(in vec3 color, in vec3 eyePos, in float mieFactor){

	vec3 r;
	vec3 m;
	float cosSun;
	float dist=length(eyePos);

	vec3 rr=vec3(0.0000169, 0.0000223, 0.0000314)*10.; // rayleigh coef
	vec3 mm=vec3(3./gl_Fog.end)*mieFactor; // mie coef
	vec3 gg=vec3(0.99);
	vec3 ee=vec3(100.); // irradiance
	
	cosSun=dot(normalize(eyePos), 
	normalize(gl_LightSource[0].position.xyz));
	
	r=rr*0.954929658551372/16.;
	r*=1.+max(cosSun, 0.)*max(cosSun, 0.);

	m=mm/12.566370614359173;
	m*=(1.-gg)*(1.-gg);
	m*=pow(1.+gg*gg-2.*gg*cosSun, vec3(-1.5));
	
	vec3 lin, fex;

	fex=exp(-dist*(rr+mm));

	lin=(r+m)/(rr+mm);
	lin*=ee;
	lin*=(1.-fex);
	
	color=color*fex+lin*gl_LightSource[0].diffuse.xyz;


	return color;
}

// per-pixel scattering (very heavy)

vec3 applyFog(in vec3 color, in vec3 eyePos, in float mieFactor){
	return slowApplyFog(color, eyePos, mieFactor);
}

#else

// per-vertex scattering (light)

varying vec3 scatFex, scatLin;

vec3 applyFog(in vec3 color, in vec3 eyePos, in float mieFactor){
	return color*scatFex+scatLin;
}

#endif

vec3 applyFog(in vec3 color, in vec3 eyePos){
	return applyFog(color, eyePos, 1.);
}

