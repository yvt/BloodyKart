varying vec3 Pos;
vec3 slowApplyFog(vec3 color, vec3 eyePos, float mieFactor);

void main()
{
	gl_FragColor.a=1.;
	gl_FragColor.rgb=slowApplyFog(vec3(0.), Pos.xyz, .01);
}
