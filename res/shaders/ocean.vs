varying vec4 eye, eye2;
varying vec3 poss, norm2;
varying float fog;

void prepareFog(in vec3 eyePos);

void main()
{
	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = vec4(gl_Vertex.x/64., gl_Vertex.z/64., 0., 0.)*vec4(1.5, 1., 1., 1.);
	gl_TexCoord[1] = gl_MultiTexCoord0;
	gl_TexCoord[1].x=gl_TexCoord[1].x*20.+.1;
	gl_TexCoord[2].xyz = gl_Normal;
	gl_Position = ftransform();
	poss=gl_Vertex.xyz;
	//norm=gl_NormalMatrix*gl_Normal;
	norm2=gl_Normal;
	eye=gl_ModelViewMatrix*gl_Vertex;
	fog=(length(gl_Position.xyz)-gl_Fog.start)/(gl_Fog.end-gl_Fog.start);
	eye2=(gl_ModelViewMatrixInverse*vec4(0.,0.,0.,1.))-gl_Vertex;
	
	prepareFog(eye.xyz);
}
