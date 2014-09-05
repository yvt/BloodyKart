varying vec3 eyePos;

void prepareFog(in vec3 eyePos);

void main()
{
	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();
	
	eyePos=(gl_ModelViewMatrix*gl_Vertex).xyz;

	prepareFog(eyePos);

}
