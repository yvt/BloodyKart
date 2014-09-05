varying vec3 blurFrom;
varying vec3 blurTo; 
uniform sampler2D screen;
uniform vec2 texSize;
void main()
{
	const int steps=8;
	const float step=1./float(steps);
	//const float strength=.2;
	vec4 total = vec4(0);
	float d=0.;
	int n;
	for(n=0;n<steps;n++){
		float per=float(n)*step;
		vec3 coord=mix(blurFrom, blurTo, per);
		coord.xy*=coord.z;
		//if(abs(coord.x)<1. && abs(coord.y)<1.){
			coord.xy=(coord.xy*.5+.5)*texSize;
			total+=texture2D(screen, coord.xy);
			//d+=1.;
		//}
		
	}
	total/=max(total.w, .0001);
	gl_FragColor=vec4(total.xyz, 1.);
	//gl_FragColor = gl_Color * texture2D(tex, gl_TexCoord[0].xy);
}
