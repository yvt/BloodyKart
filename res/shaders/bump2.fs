// vertex lighting(for dynlight) bumpmapping

uniform sampler2D tex, giTex, bump;
varying vec4 texCoord;
varying vec3 tangentU;
varying vec3 tangentV;
varying vec3 normal;
varying vec3 eyePos;
//varying vec2 giUV;
//varying float shadowVal;
varying vec3 dynLight;
//varying vec3 dynLightSpec;
uniform bool bumped;
//uniform bool isNormalMap;
//const float scale=1.;
uniform bool gIed;
uniform bool texed;

uniform vec2 bumpSizeInv;


vec3 applyFog(in vec3 color, in vec3 eyePos);


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
	
	vec3 nMap;
	if(bumped){
		nMap=texture2D(bump, newCoord.xy).xyz*2.-1.;
		nMap=normalize(nMap);
		bNormal*=(nMap.z);
		bNormal+=(tangentU)*nMap.x;
		bNormal+=(tangentV)*nMap.y;
	}else{
		nMap=vec3(0., 0., 1.);
	}
	bNormal=normalize(bNormal);

	// Shading

	vec3 dif;
	vec4 gis=texture2D(giTex, texCoord.zw);
	vec3 spec;	
	float specPower=gl_FrontMaterial.shininess;

	// Directional Light (Sun)
	
	dif=gl_LightSource[0].diffuse.xyz* 
		max(0.,dot(bNormal,normalize( gl_LightSource[0].position.xyz)));

	// Global Illumination
/*
	if(!gIed){
		// Vertex Shadow and hemisphere lighting
		//dif*=smoothstep(0., 1., shadowVal);
		spec=dif*pow(max(0., 
			dot(-reflect(-normalize(eyePos),bNormal),
			normalize( gl_LightSource[0].position.xyz))), specPower);
		//dif+=gl_LightSource[1].diffuse.xyz* 
		//	(dot(bNormal,normalize( gl_LightSource[1].position.xyz))*.5+.5);
		dif+=gl_LightModel.ambient.xyz* 
			(bNormal.y*.5+.5);
	}else{*/
		// Global Illumination Map
		//gis=gis*gis;
		gis*=2.;
		dif*=smoothstep(0., 1., gis.a);//smoothstep(0., 1., gis.a*4.);
		spec=dif*pow(max(0., 
			dot(-reflect(-normalize(eyePos),bNormal),
			normalize( gl_LightSource[0].position.xyz))), specPower);
		dif.xyz+=gis.xyz*(nMap.z*.5+.5);
	//}

	

	// Spot/Point Lights

	dif.xyz+=dynLight;
	spec.xyz+=dynLight;
	

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
