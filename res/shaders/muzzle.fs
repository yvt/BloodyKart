uniform sampler2D tex;
varying vec3 eyePos;

vec3 applyFog(in vec3 color, in vec3 eyePos);

void main()
{
	gl_FragColor = gl_Color * texture2D(tex, gl_TexCoord[0].xy);

	gl_FragColor.rgb*=4.;

	gl_FragColor.rgb=applyFog(gl_FragColor.rgb, eyePos);

}
