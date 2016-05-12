#include "Vertex.hlsli"

// Global variables

cbuffer CBFrame : register(b0)
{
	matrix g_mWorld;
	matrix g_mProj;
	float g_zoffset;
	float g_maxDiscardAlpha;
	float g_minOutputAlpha;
};

Texture2D g_textures[8] : register(t0);
SamplerState g_sampler : register(s0);

// Functions

float4 SampleSpriteTexture(float2 tex, uint ntex)
{
	switch (ntex)
	{
		case 0: return g_textures[0].Sample(g_sampler, tex);
		case 1: return g_textures[1].Sample(g_sampler, tex);
		case 2: return g_textures[2].Sample(g_sampler, tex);
		case 3: return g_textures[3].Sample(g_sampler, tex);
		case 4: return g_textures[4].Sample(g_sampler, tex);
		case 5: return g_textures[5].Sample(g_sampler, tex);
		case 6: return g_textures[6].Sample(g_sampler, tex);
		case 7: return g_textures[7].Sample(g_sampler, tex);
		default: return (float4)0;
	}
}

// Sprite rendering

SpritePixel SpriteVS(SpriteVertex input)
{
	SpritePixel output;

	float4 pos = float4(input.pos.x, input.pos.y, input.pos.z + g_zoffset, 1);
	output.pos = mul(pos, g_mWorld);
	output.pos = mul(output.pos, g_mProj);
	output.color = input.color;
	output.tex = input.tex;
	output.ntex = input.ntex;

	return output;
}

float4 SpritePS(SpritePixel input) : SV_TARGET
{
	float4 color = input.color * SampleSpriteTexture(input.tex, input.ntex);

	if (color.a < g_maxDiscardAlpha)
	{
		discard;
	}

	color.a = max(color.a, g_minOutputAlpha);
	return color;
}

// Multi-Sprite rendering

MultiSpritePixel MultiSpriteVS(MultiSpriteVertex input)
{
	MultiSpritePixel output;

	float4 pos = float4(input.pos.x, input.pos.y, input.pos.z + g_zoffset, 1);
	output.pos = mul(pos, g_mWorld);
	output.pos = mul(output.pos, g_mProj);
	output.color0 = input.color0;
	output.color1 = input.color1;
	output.color2 = input.color2;
	output.color3 = input.color3;
	output.tex0 = input.tex0;
	output.tex1 = input.tex1;
	output.tex2 = input.tex2;
	output.tex3 = input.tex3;
	output.ntex = input.ntex;

	return output;
}

float4 MultiSpritePS(MultiSpritePixel input) : SV_TARGET
{
	float4 color0 = input.color0 * SampleSpriteTexture(input.tex0, input.ntex.x);
	float4 color1 = input.color1 * SampleSpriteTexture(input.tex1, input.ntex.y);
	float4 color2 = input.color2 * SampleSpriteTexture(input.tex2, input.ntex.z);
	float4 color3 = input.color3 * SampleSpriteTexture(input.tex3, input.ntex.w);

	color0 = lerp(color0, float4(color1.rgb, 1), color1.aaaa);
	color0 = lerp(color0, float4(color2.rgb, 1), color2.aaaa);
	color0 = lerp(color0, float4(color3.rgb, 1), color3.aaaa);

	if (color0.a < g_maxDiscardAlpha)
	{
		discard;
	}

	color0.a = max(color0.a, g_minOutputAlpha);
	return color0;
}

// Line art rendering

LineArtPixel LineArtVS(LineArtVertex input)
{
	LineArtPixel output = (LineArtPixel)0;

	float4 pos = float4(input.pos.x, input.pos.y, input.pos.z + g_zoffset, 1);
	output.pos = mul(pos, g_mWorld);
	output.pos = mul(output.pos, g_mProj);
	output.color = input.color;

	return output;
}

float4 LineArtPS(LineArtPixel input) : SV_TARGET
{
	float4 color = input.color;

	if (color.a < g_maxDiscardAlpha)
	{
		discard;
	}

	color.a = max(color.a, g_minOutputAlpha);
	return color;
}

// Font/alpha rendering

float4 SpriteAlphaPS(SpritePixel input) : SV_TARGET
{
	float4 color = input.color * float4(1, 1, 1, SampleSpriteTexture(input.tex, input.ntex).a);

	if (color.a < g_maxDiscardAlpha)
	{
		discard;
	}

	color.a = max(color.a, g_minOutputAlpha);
	return color;
}
