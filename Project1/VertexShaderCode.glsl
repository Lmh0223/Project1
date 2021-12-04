#version 430


in layout(location=0) vec3 position;
in layout(location=1) vec2 vertexUV;
in layout(location=2) vec3 normal;

uniform mat4 modelTransformMatrix;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;

out vec2 UV;
out vec3 normalWorld;
out vec3 vertexPositionWorld;

void main()
{
	vec4 v = vec4(position, 1.0);
	vec4 out_position =  projectionMatrix * viewMatrix * modelTransformMatrix * v;
	gl_Position = out_position;
	
	UV = vertexUV;

	vec4 normal_temp = modelTransformMatrix * vec4(normal, 0);
	normalWorld = normal_temp.xyz;

	vertexPositionWorld = out_position.xyz;
}
