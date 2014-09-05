uniform sampler2D tex, depth;
uniform float depthCenter, depthRangeI;
void main()
{
	gl_FragColor = gl_Color * texture2D(tex, gl_TexCoord[0].xy);
	float dep;
	dep=texture2D(depth, gl_TexCoord[1].xy).r;
	dep=1./(1.-dep);
	dep=abs(dep-depthCenter)*depthRangeI;
	dep=min(dep, 1.);
	gl_FragColor.a=dep;
	//gl_FragColor.rgb=vec3(dep);
}
