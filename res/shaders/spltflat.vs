varying vec2 texCoord;
varying vec3 normal;
varying vec4 color;
varying float time;
varying vec3 eyePos;
attribute float time2;

void prepareFog(in vec3 eyePos);

void main()
{
	color = gl_Color;
	texCoord = gl_MultiTexCoord0.xy;
	normal = gl_NormalMatrix * gl_Normal;
	
	gl_Position = ftransform();
	eyePos=(gl_ModelViewMatrix*gl_Vertex).xyz;
	time=time2;
	
	prepareFog(eyePos);
}
