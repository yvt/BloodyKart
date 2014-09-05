varying vec4 texCoord;
varying vec3 tangentU;
varying vec3 tangentV;
varying vec3 normal;
varying vec3 eyePos;
//varying vec2 giUV;

//varying vec3 parMap;
attribute vec3 tU, tV;
//attribute vec3 gIvt;

void prepareFog(in vec3 eyePos);

void main()
{
	gl_FrontColor = gl_Color;
	texCoord.xy = gl_MultiTexCoord0.xy;
	texCoord.zw = gl_MultiTexCoord2.xy;
	tangentU = tU;
	tangentV = tV;
	normal=gl_Normal;

	tangentU=gl_NormalMatrix*tangentU;
	tangentV=gl_NormalMatrix*tangentV;
	tangentU=-tangentU;
	tangentV=-tangentV;
	normal=gl_NormalMatrix*normal;
	//gI=gIvt;
	
	
	

	gl_Position = ftransform();

	eyePos=(gl_ModelViewMatrix*gl_Vertex).xyz;
	/*
	parMap.x=dot(normal, eyePos);
	parMap.y=dot(normal, tangentU);
	parMap.z=dot(normal, tangentV);*/
	
	prepareFog(eyePos);

	//gl_Position.xyz+=tangentV;
}
