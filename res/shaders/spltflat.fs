uniform sampler2D tex;
varying vec2 texCoord;
varying vec3 normal;
varying vec4 color;
varying vec3 eyePos;

varying float time;

vec3 applyFog(in vec3 color, in vec3 eyePos);

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
		dif=p.position.xyz-eyePos;
		dir=normalize(dif);
		d=dot(-dir, p.spotDirection);
		att=p.constantAttenuation;
		att+=length(dif)*p.linearAttenuation;
		att+=length(dif)*length(dif)*p.quadraticAttenuation;
		att=1./att;
		
		resBrightness=vec3(max(0.,
			dot(bNormal, dir)));
		resBrightness*=pow(abs(d), p.spotExponent);
		

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
	const float texWidth=512.;
	const float texHeight=256.;
	vec3 bNormal;
	float aa1, aa2, aa3, aa4;

	aa1=texture2D(tex, texCoord).a;

	if(aa1<.01)
		discard;


	bNormal=normal;

	bNormal=normalize(bNormal);

	// diffuse

	vec3 dif;
	dif=(gl_LightModel.ambient.xyz+gl_LightSource[0].ambient.xyz);
	dif+=gl_LightSource[0].diffuse.xyz* 
		abs(dot(bNormal,normalize( gl_LightSource[0].position.xyz))*.5+.5);
	//dif+=gl_LightSource[1].diffuse.xyz* 
	//	max(0.,dot(bNormal,normalize( gl_LightSource[1].position.xyz))*.5+.5);
		
	{
		lightRet ret;
		ret=calcLight(gl_LightSource[2], bNormal, 1.);
		dif+=ret.dif; 
		ret=calcLight(gl_LightSource[3], bNormal, 1.);
		dif+=ret.dif; 
		ret=calcLight(gl_LightSource[4], bNormal, 1.);
		dif+=ret.dif;
		//ret=calcLight(gl_LightSource[5], bNormal, 1.);
		//dif+=ret.dif;
		//ret=calcLight(gl_LightSource[6], bNormal, 1.);
		//dif+=ret.dif;
		//ret=calcLight(gl_LightSource[7], bNormal, 1.);
		//dif+=ret.dif;
	}

	gl_FragColor = texture2D(tex, texCoord.xy);
	//gl_FragColor.a=pow(gl_FragColor.a, time*10.+1.);
	gl_FragColor*=color;
	

	gl_FragColor.xyz*=dif;

/*
	// specular

	float spec;
	spec=max(0., 
		dot(-reflect(-normalize(eyePos),bNormal),
		normalize( gl_LightSource[0].position.xyz)));
	spec=pow(spec, 40.);
	
	
	gl_FragColor.xyz+=spec*vec3(3.)*gl_FrontLightProduct[0].specular.xyz;*/
	gl_FragColor.xyz=applyFog(gl_FragColor.xyz, eyePos);
}
