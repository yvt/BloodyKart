uniform sampler2D tex;
varying vec2 pix;
void main()
{
	vec4 texs;
	vec2 origin;
	

	origin=gl_TexCoord[0].xy;
	texs=texture2D(tex, origin)*0.375;

	origin=gl_TexCoord[0].xy; origin.x-=1.25*pix.x;
	texs+=texture2D(tex, origin)*0.3125;

	origin=gl_TexCoord[0].xy; origin.x+=1.25*pix.x;
	texs+=texture2D(tex, origin)*0.3125;
	
	gl_FragColor = gl_Color * texs;
	gl_FragColor.a=gl_Color.a;
}
