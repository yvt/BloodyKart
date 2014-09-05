uniform sampler2D tex;

varying vec2 texCoord, giCoord;
varying vec3 normal;
varying vec4 color;
varying vec3 diffuse;
varying vec3 eyePos;

vec3 applyFog(in vec3 color, in vec3 eyePos);

void main()
{
	vec4 texColor=texture2D(tex, texCoord);
	if(texColor.a<.6)
		discard;

	gl_FragColor = color * texColor;

	vec3 dif;


	dif=diffuse;
	

	gl_FragColor.xyz *= dif;
	
	// fog

	gl_FragColor.xyz=applyFog(gl_FragColor.xyz, eyePos);

}
