varying vec2 texCoord;
varying vec3 normal;
varying vec3 uTangent;
varying vec3 vTangent;
varying vec4 color;
varying vec3 eyePos;
varying float time;
attribute float time2;

void prepareFog(in vec3 eyePos);

void main()
{
	color = gl_Color;
	texCoord = gl_MultiTexCoord0.xy;
	uTangent = gl_NormalMatrix * gl_MultiTexCoord1.xyz;
	vTangent = gl_NormalMatrix * gl_MultiTexCoord2.xyz;
	normal = gl_NormalMatrix * gl_Normal;
	
	gl_Position = ftransform();

	eyePos=(gl_ModelViewMatrix*gl_Vertex).xyz;
	time=time2;
	
	prepareFog(eyePos);
}
