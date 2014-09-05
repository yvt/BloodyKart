varying vec4 texCoord;
varying vec3 tangentU;
varying vec3 tangentV;
varying vec3 normal;
varying vec3 eyePos;
//varying vec2 giUV;
//varying float shadowVal;
varying vec3 dynLight;
//varying vec3 dynLightSpec;
//varying vec3 parMap;
attribute vec3 tU, tV;
attribute vec3 gIvt;
//attribute float sV;

void prepareFog(in vec3 eyePos);

struct lightRet{
	vec3 dif, spec;
};

lightRet calcLight(gl_LightSourceParameters p, vec3 bNormal, float specPower){
	//gl_LightSourceParameters p=gl_LightSource[n];
	const float eps=.001;
	lightRet ret;
	ret.dif=ret.spec=vec3(0.);
	
	if(p.diffuse.r>eps || p.diffuse.g>eps || p.diffuse.b>eps){
		float d, att;
		vec3 resBrightness;
		vec3 dif, dir;	
		dif=eyePos-p.position.xyz;
		dir=normalize(dif);
		d=dot(dir, p.spotDirection);
		att=p.constantAttenuation;
		att+=length(dif)*p.linearAttenuation;
		att+=length(dif)*length(dif)*p.quadraticAttenuation;
		att=1./att;
		if(p.spotExponent<eps){
			// Point Light
			resBrightness=vec3(max(0.,
				-dot(bNormal, dir)));
		}else if(d>=0.0){
			// Spot Light
			resBrightness=vec3(max(0.,
				-dot(bNormal, dir)));
			resBrightness*=pow(d, p.spotExponent);
			//resBrightness=vec3(1.);att=1.;
		}else{
			// Spot Light, but out of range
			resBrightness=vec3(0.);
		}

		resBrightness*=p.diffuse.xyz;
		resBrightness*=att;
		
		ret.dif=resBrightness;

		// Specular

		ret.spec=resBrightness*pow(max(0., 
		dot(-reflect(-normalize(eyePos),bNormal),
		dir)), specPower);
	}
	return ret;
}

void main()
{
	gl_FrontColor = gl_Color;
	texCoord.xy = gl_MultiTexCoord0.xy;
	texCoord.zw = gl_MultiTexCoord2.xy;
	tangentU = tU;
	tangentV = tV;
	normal=gl_Normal;

	tangentU=gl_NormalMatrix*tangentU;
	tangentV=gl_NormalMatrix*tangentV;
	tangentU=-tangentU;
	tangentV=-tangentV;
	normal=gl_NormalMatrix*normal;
	//gI=gIvt;
	//shadowVal=sV;
	
	gl_Position = ftransform();

	eyePos=(gl_ModelViewMatrix*gl_Vertex).xyz;
	
	// calc DynLight
	
	vec3 dif=vec3(0);
	vec3 spec=vec3(0);
	float specPower=max(.00001,gl_FrontMaterial.shininess);
	
	{
		lightRet ret;
		ret=calcLight(gl_LightSource[2], normal, specPower);
		dif+=ret.dif; spec+=ret.spec;
		ret=calcLight(gl_LightSource[3], normal, specPower);
		dif+=ret.dif; spec+=ret.spec;
		ret=calcLight(gl_LightSource[4], normal, specPower);
		dif+=ret.dif; spec+=ret.spec;
		ret=calcLight(gl_LightSource[5], normal, specPower);
		dif+=ret.dif; spec+=ret.spec;
		ret=calcLight(gl_LightSource[6], normal, specPower);
		dif+=ret.dif; spec+=ret.spec;
		ret=calcLight(gl_LightSource[7], normal, specPower);
		dif+=ret.dif; spec+=ret.spec;
	}
	
	dynLight=dif;
	//dynLightSpec=spec;

	prepareFog(eyePos);
	
	/*
	parMap.x=dot(normal, eyePos);
	parMap.y=dot(normal, tangentU);
	parMap.z=dot(normal, tangentV);*/

	//gl_Position.xyz+=tangentV;
}
