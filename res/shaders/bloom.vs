varying vec2 pix;
void main()
{
	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_TextureMatrix[0]*gl_MultiTexCoord0;
	gl_Position = ftransform();
	pix.x=gl_TextureMatrix[0][0][0];
	pix.y=gl_TextureMatrix[0][1][1];
}
