uniform sampler2D texDry, texDepth;
void main()
{
	vec4 texs;
	vec2 origin;


	origin=gl_TexCoord[0].xy;
	texs=texture2D(texDry, origin);
	
	float depth;
	depth=texture2D(texDepth, origin).r;
	depth=1./(1.-depth);	
	depth=min(depth, 3000.);

	gl_FragColor = texs;//max(gl_Color * texs*2.5, vec4(0.));
	gl_FragColor=clamp(gl_FragColor, 0., 1024.);
		
	gl_FragColor.a=1.-clamp(depth/gl_Fog.end, 0., 1.);

	if(gl_FragColor.a==0.)
		discard;

}
