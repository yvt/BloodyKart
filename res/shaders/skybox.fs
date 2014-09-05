uniform sampler2D tex;


void main()
{
	gl_FragColor = gl_Color * texture2D(tex, gl_TexCoord[0].xy);
	gl_FragColor.rgb*=2.;
	//gl_FragColor.rgb*=vec3(1.)+pow(max(vec3(0.), gl_FragColor.rgb-vec3(1.)), vec3(2.))*3.;
	
	gl_FragColor.a=1.;
}
