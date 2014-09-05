uniform sampler2D tex;
varying vec4 texCoord; // vec2 coord, vec2 fireFrom.xy
varying vec4 normal;   // vec3 normal, float fireFrom.z
varying vec3 uTangent; // vec3 uTangent
varying vec3 vTangent; 
varying vec4 color;
varying vec4 eyePos;   // vec3 eyePos, float firePower
//varying vec3 fireFrom;
uniform vec3 fireColor;
//varying float time;

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
		dif=p.position.xyz-eyePos.xyz;
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
		dot(-reflect(-normalize(eyePos.xyz),bNormal),
		dir)), specPower);
	}
	return ret;
}

void main()
{
	const float texWidth=32.;
	const float texHeight=32.;
	vec3 bNormal;
	float aa1, aa2, aa3, aa4;
	vec3 fireFrom=vec3(texCoord.zw, normal.w);

	aa1=texture2D(tex, texCoord.xy-vec2(.5/texWidth, 0.)).a;

	if(aa1<.01)
		discard;

	aa2=texture2D(tex, texCoord.xy+vec2(.5/texWidth, 0.)).a;
	aa3=texture2D(tex, texCoord.xy-vec2(0., .5/texHeight)).a;
	aa4=texture2D(tex, texCoord.xy+vec2(0., .5/texHeight)).a;

	bNormal.x=aa2 - aa1;
	bNormal.y=aa4 - aa3;
	bNormal.z=.05;
	bNormal=normalize(bNormal);

	bNormal=normal.xyz
		  +(uTangent)*bNormal.x
		  +(vTangent)*bNormal.y;

	bNormal=normalize(bNormal);

	// diffuse

	vec3 dif;
	dif=(gl_LightModel.ambient.xyz+gl_LightSource[0].ambient.xyz);
	dif+=gl_LightSource[0].diffuse.xyz* 
		max(0.,dot(bNormal,normalize( gl_LightSource[0].position.xyz))*.5+.5);
	dif+=gl_LightSource[0].diffuse.xyz* 
		max(0.,(1.-color.a)*.6-dot(bNormal,normalize( gl_LightSource[0].position.xyz)))*pow(1.-aa1*color.a, 2.);
		
	// fire
	dif+=fireColor* 
		max(0.,dot(bNormal,normalize(fireFrom-eyePos.xyz))*.5+.5)*9.*eyePos.w;
	dif+=fireColor*pow(1.-aa1*color.a, 2.)*2.*eyePos.w;
/*	dif+=gl_LightSource[1].diffuse.xyz* 
		max(0.,dot(bNormal,normalize( gl_LightSource[1].position.xyz))*.5+.5);
*/

	{
		lightRet ret;
		ret=calcLight(gl_LightSource[2], bNormal, 1.);
		dif+=ret.dif; 
		ret=calcLight(gl_LightSource[3], bNormal, 1.);
		dif+=ret.dif; 
		ret=calcLight(gl_LightSource[4], bNormal, 1.);
		dif+=ret.dif;
		ret=calcLight(gl_LightSource[5], bNormal, 1.);
		dif+=ret.dif;
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
	gl_FragColor.xyz=applyFog(gl_FragColor.xyz, eyePos.xyz);
}
