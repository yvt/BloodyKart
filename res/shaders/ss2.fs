uniform sampler2D tex, texDepth;
uniform vec2 unit;
void main()
{
	gl_FragColor=vec4(0., 0., 0., 1.);
	gl_FragColor.rgb += gl_Color.a * texture2D(tex, 
		gl_TexCoord[0].xy+unit*vec2(0., 0.)).rgb;
	gl_FragColor.rgb += gl_Color.a * texture2D(tex, 
		gl_TexCoord[0].xy+unit*vec2(1., 0.)).rgb;
	gl_FragColor.rgb += gl_Color.a * texture2D(tex, 
		gl_TexCoord[0].xy+unit*vec2(0., 1.)).rgb;
	gl_FragColor.rgb += gl_Color.a * texture2D(tex, 
		gl_TexCoord[0].xy+unit*vec2(1., 1.)).rgb;
	gl_FragDepth=0.;
	gl_FragDepth+=gl_Color.a*texture2D(texDepth, 
		gl_TexCoord[0].xy+unit*vec2(0., 0.)).r;
	gl_FragDepth+=gl_Color.a*texture2D(texDepth, 
		gl_TexCoord[0].xy+unit*vec2(1., 0.)).r;
	gl_FragDepth+=gl_Color.a*texture2D(texDepth, 
		gl_TexCoord[0].xy+unit*vec2(0., 1.)).r;
	gl_FragDepth+=gl_Color.a*texture2D(texDepth, 
		gl_TexCoord[0].xy+unit*vec2(1., 1.)).r;
	
	//gl_FragColor=vec4(1.);
	//gl_FragDepth=gl_FragCoord.w;
}
