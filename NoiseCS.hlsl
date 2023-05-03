#include "Lighting.hlsli"

cbuffer perFrame : register(b0)
{
	float randNum;
}

RWTexture2D<unorm float4> outNoiseTexture		: register(u0);
// SamplerState basicSampler				: register(s0);


[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float r = sin(randNum);
	float g = cos(randNum);
	outNoiseTexture[DTid.xy] = float4(r, g, randNum, 1);
}