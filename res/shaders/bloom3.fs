uniform sampler2D tex;
varying vec2 pix;
void main()
{
	vec4 texs;
	vec2 origin;
	vec2 dif1=(vec2(0.577350269189626, 0.288675134594813)*6.*pix);
	vec2 dif2=(vec2(0.288675134594813, -0.577350269189626)*6.*pix);
	const float scale1=.05;
	const float scale2=.03;
	const float scal=1.5;
	float n;
	float factor;
	float total=0.;

	origin=gl_TexCoord[0].xy;
	factor=scal*.13;
	texs=texture2D(tex, origin)*scal*.13;

	total=factor;

	for(n=-12.;n<=12.;n+=1.){
		origin=gl_TexCoord[0].xy; origin+=dif1*n;
		factor=scal*scale1*pow(1.-abs(n)/13., 4.);
		texs+=texture2D(tex, origin)*factor;
		total+=factor;
	}

	for(n=-3.;n<=3.;n+=1.){
		origin=gl_TexCoord[0].xy; origin+=dif2*n;
		factor=scal*scale2*pow(1.-abs(n)/4., 4.);
		texs+=texture2D(tex, origin)*factor;
		total+=factor;
	}
	
	gl_FragColor = (texs/total);//max(gl_Color * texs*2.5, vec4(0.));
	gl_FragColor.a=gl_Color.a;
}
