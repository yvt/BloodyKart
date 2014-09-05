varying vec3 blurFrom;
varying vec3 blurTo; 
varying vec2 texCoord;
uniform vec2 texSize;
uniform mat4 oldMatrixInverse;
uniform mat4 curMatrixInverse;
uniform mat4 curMatrix, oldMatrix;
vec3 calcPos(mat4 invMat){
	vec4 v;
	v.xy=gl_MultiTexCoord0.xy;
	v.z=1.; v.w=1.;
	v=invMat*v;
	v= oldMatrix*v;
	return v.xyz;
}
void main()
{
	gl_FrontColor = gl_Color;
	gl_Position = ftransform();
	
	blurFrom=calcPos(curMatrixInverse);
	blurTo=calcPos(oldMatrixInverse);

	texCoord=gl_MultiTexCoord0.xy;
	//blurFrom=blurTo;
}
