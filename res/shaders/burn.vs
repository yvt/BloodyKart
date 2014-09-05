varying vec4 texCoord; // vec2 coord, vec2 fireFrom.xy
varying vec4 normal;   // vec3 normal, float fireFrom.z
varying vec3 uTangent;
varying vec3 vTangent;
varying vec4 color;
varying vec4 eyePos;   // vec3 eyePos, float firePower
//varying float time;
//varying vec3 fireFrom;
//varying vec3 fireColor;
//attribute float time2;


void prepareFog(in vec3 eyePos);

void main()
{
	color = gl_Color;
	texCoord.xy = gl_MultiTexCoord0.xy;
	uTangent = gl_NormalMatrix * gl_MultiTexCoord1.xyz;
	vTangent = gl_NormalMatrix * gl_MultiTexCoord2.xyz;
	vec3 fireFrom=(gl_ModelViewMatrix*
		vec4(gl_MultiTexCoord3.xyz, 1.)).xyz;
	texCoord.zw=fireFrom.xy;
	normal.w=fireFrom.z;
	//fireColor=gl_MultiTexCoord4.xyz;
	normal.xyz = gl_NormalMatrix * gl_Normal;
	
	gl_Position = ftransform();

	eyePos.xyz=(gl_ModelViewMatrix*gl_Vertex).xyz;
	eyePos.w=max(0., gl_MultiTexCoord4.x);	

	//time=time2;
	
	prepareFog(eyePos.xyz);
}
