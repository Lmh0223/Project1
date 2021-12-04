#version 430


out vec3 outColor;

in vec2 UV; 
in vec3 normalWorld;
in vec3 vertexPositionWorld;

uniform vec3 cameraPosition;

uniform sampler2D textureSampler; 

void main()
{
	vec3 texture = texture(textureSampler, UV).rgb;

	vec3 norm = normalize(normalWorld);
	vec3 viewDir = normalize(cameraPosition - vertexPositionWorld);

	outColor = texture;
}