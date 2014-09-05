uniform sampler2D texSrc;
uniform sampler2D texBlur;
varying vec3 factor;
void main()
{
	gl_FragColor = texture2D(texSrc, gl_TexCoord[0].xy)*vec4(factor, 1.);
	gl_FragColor=clamp(gl_FragColor, 0., 1.);
	//gl_FragColor.rgb=pow(gl_FragColor.rgb, vec3(1.4));
	//gl_FragColor.xyz=1./factor;
	//gl_FragColor.xyz+=vec3(.1);
	gl_FragColor.a=1.;
}
