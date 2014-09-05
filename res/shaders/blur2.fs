varying vec3 blurFrom;
varying vec3 blurTo; 
uniform sampler2D screen;
uniform sampler2D depthTex;
uniform vec2 texSize;
varying vec2 texCoord;
void main()
{
	const int steps=8;
	const float step=50./float(steps);
	const float unmovTor=.2;
	const float unmovTorFar=.09;
	//const float strength=.2;
	vec4 total = vec4(0);
	float d=0.;
	int n;
	float baseZ=1.-texture2D(depthTex, (texCoord*.5+.5)*texSize).x;
	float scale=baseZ*(1.-clamp((baseZ-unmovTorFar)/(unmovTor-unmovTorFar), 0., 1.));
	scale=clamp(scale, 0., 1./50.);
	for(n=0;n<steps;n++){
		float per=float(n)*step*scale;
		vec3 coord=mix(blurTo, blurFrom, per);
		coord.xy*=coord.z;
		//if(abs(coord.x)<1. && abs(coord.y)<1.){
		coord.xy=(coord.xy*.5+.5)*texSize;
		total+=texture2D(screen, coord.xy)*step(0.1, coord.z);
			//d+=1.;
		//}
		
	}
	total/=max(total.w, .0001);
	gl_FragColor=vec4(total.xyz, 1.);
	//gl_FragColor.xyz=vec3(scale);
	
	
	//gl_FragColor = gl_Color * texture2D(tex, gl_TexCoord[0].xy);
}
