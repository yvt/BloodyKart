
varying vec3 Pos;

void prepareFog(in vec3 eyePos, in float mieFactor);

float sphereStick(vec2 v1, vec2 v2){
	vec2 norm; vec2 dir=normalize(v2-v1); 
	norm.xy=vec2(dir.y, -dir.x);
	
	float shift;
	shift=dot(v1, norm);
	shift=clamp(shift*shift, 0., 1.);
	return sqrt(1.-shift);
}

void main()
{
	gl_Position = ftransform();

	
	vec3 ep;
	const float heig=1000.;
	const float atmoScale=0.02;
	const float lastScale=10000.;
	
	ep=normalize(gl_Vertex.xyz);
	
	float dist, dist2;
	vec2 v2=vec2(length(ep.xz), ep.y);
	
	dist=sphereStick(vec2(0., 1.)*(1.-atmoScale),
		vec2(v2.x, 1.+v2.y)*(1.-atmoScale))/(1.-atmoScale);
	
	dist-=sphereStick(vec2(0., 1.),
		vec2(v2.x, 1.+v2.y));
		
	
		
	dist*=lastScale;
	//ep=normalize(ep);
	
	ep=gl_NormalMatrix*ep;
	ep*=dist;
	
	//prepareFog(ep, .01);
	Pos=ep;
}
