

void prepareFog(in vec3 eyePos, in float mieFactor);


void main()
{
	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = ftransform();

}
