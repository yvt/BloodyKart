uniform sampler2D texSrc;
uniform sampler2D texBlur;
uniform vec2 maxSrc;
varying vec3 factor;
void main()
{
	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
	
	float exposure=1.;
	float maxLum=dot(texture2D(texBlur, maxSrc),
		vec4(0.30, 0.59, 0.11, 0.00));
	maxLum=max(.01, maxLum);
	float Y;
	Y=exposure*(exposure/maxLum+1.)/(exposure+1.);
	factor=vec3(Y);
	
}
