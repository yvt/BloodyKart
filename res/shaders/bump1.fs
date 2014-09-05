uniform sampler2D tex, giTex, bump;
varying vec4 texCoord;
varying vec3 tangentU;
varying vec3 tangentV;
varying vec3 normal;
varying vec3 eyePos;
//varying vec2 giUV;

uniform bool bumped;
//uniform bool isNormalMap;
//const float scale=1.;
uniform bool gIed;
uniform bool texed;

uniform vec2 bumpSizeInv;

uniform vec3 ldV00;
uniform vec3 ldV10;
uniform vec3 ldV11;
uniform vec3 ldV12;

vec3 applyFog(in vec3 color, in vec3 eyePos);

struct lightRet{
	vec3 dif, spec;
};

#define calcLight(p, bNormal, specPower, ret) \
{\
	const float eps=.001;\
	ret.dif=ret.spec=vec3(0.);\
	\
	if(p.diffuse.r>eps || p.diffuse.g>eps || p.diffuse.b>eps){\
		float d, att;\
		vec3 resBrightness;\
		vec3 dif, dir;	\
		dif=p.position.xyz-eyePos;\
		dir=normalize(dif);\
		d=dot(-dir, p.spotDirection);\
		att=p.constantAttenuation;\
		att+=length(dif)*p.linearAttenuation;\
		att+=length(dif)*length(dif)*p.quadraticAttenuation;\
		att=1./att;\
		\
		resBrightness=vec3(max(0.,\
			dot(bNormal, dir)));\
		resBrightness*=pow(max(d, 0.), p.spotExponent);\
		\
\
		resBrightness*=p.diffuse.xyz;\
		resBrightness*=att;\
		\
		ret.dif=resBrightness;\
\
\
		ret.spec=resBrightness*pow(max(0., \
		dot(-reflect(-normalize(eyePos),bNormal),\
		dir)), specPower);\
	}\
}\

/*
void calcLight(gl_LightSourceParameters p, vec3 bNormal, float specPower, out lightRet ret){
	//gl_LightSourceParameters p=gl_LightSource[n];
	const float eps=.001;
	//lightRet ret;
	ret.dif=ret.spec=vec3(0.);
	
	if(p.diffuse.r>eps || p.diffuse.g>eps || p.diffuse.b>eps){
		float d, att;
		vec3 resBrightness;
		vec3 dif, dir;	
		dif=p.position.xyz-eyePos;
		dir=normalize(dif);
		d=dot(-dir, p.spotDirection);
		att=p.constantAttenuation;
		att+=length(dif)*p.linearAttenuation;
		att+=length(dif)*length(dif)*p.quadraticAttenuation;
		att=1./att;
		
		resBrightness=vec3(max(0.,
			dot(bNormal, dir)));
		resBrightness*=pow(max(d, 0.), p.spotExponent);
		

		resBrightness*=p.diffuse.xyz;
		resBrightness*=att;
		
		ret.dif=resBrightness;

		// Specular

		ret.spec=resBrightness*pow(max(0., 
		dot(-reflect(-normalize(eyePos),bNormal),
		dir)), specPower);
	}
	//return ret;
}*/

void main()
{
	
	vec3 bNormal;
	vec4 texCol;
	
	vec2 newCoord;
	
	newCoord=texCoord.xy;

	
	texCol=texture2D(tex, newCoord.xy);
	if(texCol.a<.01)
		discard;
	if(gl_Color.a<.01)
		discard;

	bNormal=(normal);
	gl_FragColor.a=1.;
	if(bumped){
		vec3 nMap;
			
		nMap=texture2D(bump, newCoord.xy).xyz*2.-1.;
		//nMap.xy*=scale;
			
		
		//nMap=vec3(0,0,1);
		nMap=normalize(nMap);
		bNormal*=(nMap.z);
		bNormal+=(tangentU)*nMap.x;
		bNormal+=(tangentV)*nMap.y;

	}else{
		//bNormal=-bNormal;

	}
	bNormal=normalize(bNormal);

	// Shading

	vec3 dif;
	//vec4 gis=texture2D(giTex, texCoord.zw);
	vec3 spec;	
	float specPower=gl_FrontMaterial.shininess;

	// Directional Light (Sun)
	
	dif=gl_LightSource[0].diffuse.xyz* 
		max(0.,dot(bNormal,normalize( gl_LightSource[0].position.xyz)));

	// Global Illumination

	//if(!gIed){
		//  hemisphere lighting
		spec=dif*pow(max(0., 
			dot(-reflect(-normalize(eyePos),bNormal),
			normalize( gl_LightSource[0].position.xyz))), specPower);
		//dif+=gl_LightSource[1].diffuse.xyz* 
		///	(dot(bNormal,normalize( gl_LightSource[1].position.xyz))*.5+.5);
		dif+=gl_LightModel.ambient.xyz* 
			(bNormal.y*.5+.5);
		/*
	}else{
		// Global Illumination Map
		dif*=smoothstep(0., 1., gis.a*4.);
		spec=dif*pow(max(0., 
			dot(-reflect(-normalize(eyePos),bNormal),
			normalize( gl_LightSource[0].position.xyz))), specPower);
		dif.xyz+=gis.xyz*4.;
	}*/

	

	// Spot/Point Lights

	{
		lightRet ret;
		calcLight(gl_LightSource[2], bNormal, specPower, ret);
		dif+=ret.dif; spec+=ret.spec;
		calcLight(gl_LightSource[3], bNormal, specPower, ret);
		dif+=ret.dif; spec+=ret.spec;
		calcLight(gl_LightSource[4], bNormal, specPower, ret);
		dif+=ret.dif; spec+=ret.spec;
		calcLight(gl_LightSource[5], bNormal, specPower, ret);
		dif+=ret.dif; spec+=ret.spec;
		calcLight(gl_LightSource[6], bNormal, specPower, ret);
		dif+=ret.dif; spec+=ret.spec;
		//ret=calcLight(gl_LightSource[7], bNormal, specPower);
		//dif+=ret.dif; spec+=ret.spec;
	}

	// lightData
	dif.xyz+=ldV00;
	dif.xyz+=ldV10*bNormal.x;
	dif.xyz+=ldV11*bNormal.y;
	dif.xyz+=ldV12*bNormal.z;

 	// Diffuse
	
	gl_FragColor = (gl_Color);
	if(texed)
		gl_FragColor*=texCol;

	gl_FragColor.xyz*=dif;

	// Specular
	
	gl_FragColor.xyz+=spec*gl_FrontMaterial.specular.xyz;

	// Fog / Light Scattering

	gl_FragColor.xyz=applyFog(gl_FragColor.xyz, eyePos);

}



