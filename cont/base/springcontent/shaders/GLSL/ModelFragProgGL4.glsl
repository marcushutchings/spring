#version 430 core

layout(binding = 0) uniform sampler2D tex1;
layout(binding = 1) uniform sampler2D tex2;

in Data {
	vec4 uvCoord;
	vec4 teamCol;
};

out vec4 fragColor;
void main(void)
{
	vec4 modelColor = texture(tex1, uvCoord.xy);
	vec4 modelProp  = texture(tex2, uvCoord.xy);

	modelColor.rgb = mix(modelColor.rgb, teamCol.rgb, modelColor.a);

	fragColor = vec4(modelColor.rgb, modelProp.a);
}

