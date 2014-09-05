uniform sampler2D tex;
uniform vec2 blurSize;
void main()
{
	vec3 v;
#if 1

	v= texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(0., 0.)).xyz;
	v=max(v, texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(0., 1.)).xyz);
	v=max(v, texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(0., 2.)).xyz);
	v=max(v, texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(0., 3.)).xyz);

	v=max(v, texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(1., 0.)).xyz);
	v=max(v, texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(1., 1.)).xyz);
	v=max(v, texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(1., 2.)).xyz);
	v=max(v, texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(1., 3.)).xyz);

	v=max(v, texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(2., 0.)).xyz);
	v=max(v, texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(2., 1.)).xyz);
	v=max(v, texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(2., 2.)).xyz);
	v=max(v, texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(2., 3.)).xyz);

	v=max(v, texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(3., 0.)).xyz);
	v=max(v, texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(3., 1.)).xyz);
	v=max(v, texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(3., 2.)).xyz);
	v=max(v, texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(3., 3.)).xyz);

#else
	v= texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(0., 0.)).xyz;
	v+=texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(0., 1.)).xyz;
	v+=texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(0., 2.)).xyz;
	v+=texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(0., 3.)).xyz;

	v+=texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(1., 0.)).xyz;
	v+=texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(1., 1.)).xyz;
	v+=texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(1., 2.)).xyz;
	v+=texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(1., 3.)).xyz;

	v+=texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(2., 0.)).xyz;
	v+=texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(2., 1.)).xyz;
	v+=texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(2., 2.)).xyz;
	v+=texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(2., 3.)).xyz;

	v+=texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(3., 0.)).xyz;
	v+=texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(3., 1.)).xyz;
	v+=texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(3., 2.)).xyz;
	v+=texture2D(tex, gl_TexCoord[0].xy+blurSize*vec2(3., 3.)).xyz;
	v/=16.;
#endif
	v=clamp(v, 0., 1024.);
	gl_FragColor=gl_Color*vec4(v, 1.);
}
