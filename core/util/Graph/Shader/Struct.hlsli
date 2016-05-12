// Shared data structures

struct Light
{
	float4 wpos; // only use xyz
	float4 diffuse;
	float4 specular;
	float4 ambient;
	float4 atten; // only use xyz
};
