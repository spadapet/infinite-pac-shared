// Shared vertex structures

struct VertexPN
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
};

struct VertexPNA
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float weight : WEIGHT;
};

struct VertexPNC
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float4 diffuse : COLOR;
};

struct VertexPNCT
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float4 diffuse : COLOR;
	float2 tex : TEXCOORD;
};

struct VertexPNT
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 tex : TEXCOORD;
};

struct VertexPNTB
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 tex : TEXCOORD;
	uint bones : INDEX;
	float4 weights : WEIGHT;
};

struct ColorPCSMT
{
	float4 pos : SV_POSITION;
	float4 diffuse : COLOR0;
	float4 specular : COLOR1;
	nointerpolation float4 ambient : COLOR2;
	float2 tex : TEXCOORD;
};

// --------------------------------------------------------------------------

struct LineArtVertex
{
	float3 pos : POSITION;
	float4 color : COLOR;
};

struct LineArtPixel
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
};

// --------------------------------------------------------------------------

struct SpriteVertex
{
	float3 pos : POSITION;
	float4 color : COLOR;
	float2 tex : TEXCOORD;
	uint ntex : TEXINDEX;
};

struct SpritePixel
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
	float2 tex : TEXCOORD;
	uint ntex : TEXINDEX;
};

// --------------------------------------------------------------------------

struct MultiSpriteVertex
{
	float3 pos : POSITION;
	float4 color0 : COLOR0;
	float4 color1 : COLOR1;
	float4 color2 : COLOR2;
	float4 color3 : COLOR3;
	float2 tex0 : TEXCOORD0;
	float2 tex1 : TEXCOORD1;
	float2 tex2 : TEXCOORD2;
	float2 tex3 : TEXCOORD3;
	uint4 ntex : TEXINDEX;
};

struct MultiSpritePixel
{
	float4 pos : SV_POSITION;
	float4 color0 : COLOR0;
	float4 color1 : COLOR1;
	float4 color2 : COLOR2;
	float4 color3 : COLOR3;
	float2 tex0 : TEXCOORD0;
	float2 tex1 : TEXCOORD1;
	float2 tex2 : TEXCOORD2;
	float2 tex3 : TEXCOORD3;
	uint4 ntex : TEXINDEX;
};
