
uniform sampler2D giTex;
varying vec2 texCoord, giCoord;
varying vec4 color;
varying vec3 normal;
varying vec3 diffuse;
varying vec3 eyePos;

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
	color = gl_Color;
	texCoord = gl_MultiTexCoord0.xy;
	giCoord = gl_MultiTexCoord1.xy;
	normal = gl_NormalMatrix*gl_Normal;
		gl_Position = ftransform();
	diffuse=gl_LightSource[0].diffuse.xyz*
	max(0., dot(normal, normalize(
	gl_LightSource[0].position.xyz)));
	
	eyePos=(gl_ModelViewMatrix*gl_Vertex).xyz;
	
	vec3 dif=vec3(0);
	vec3 spec=vec3(0);
	vec4 gi=texture2D(giTex, giCoord);;
	
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
	
	gi*=2.;
	diffuse*=(smoothstep(0., 1., gi.a));	
	diffuse+=dif;
	diffuse+=gi.xyz;
	
	prepareFog(eyePos);
	
}
