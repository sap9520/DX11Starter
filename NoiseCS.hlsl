#include "Lighting.hlsli"

cbuffer perFrame : register(b0)
{
	float2 resolution;
	float time;
}

RWTexture2D<unorm float4> outNoiseTexture		: register(u0);

// https://thebookofshaders.com/12/
float2 random(float2 uv)
{
	float2 output = dot(uv.xy, float2(12.9898, 78.233));
	output = sin(output * 43758.5453123);
	return float2(frac(output));
}

float3 cellularNoise(float2 uv, float numCols, float numRows)
{
	float2 pos = uv / resolution;
	pos.x *= resolution.x / resolution.y;
	float3 color = float3(0, 0, 0);

	// Apply scale
	pos *= 3;

	// Create tiles
	float2 i = floor(pos);
	float2 f = frac(pos);

	float minDist = 1;

	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			float2 neighbor = float2(x, y);
			float2 randPoint = random(i * neighbor);

			// Move point
			randPoint = 0.5f + 0.5f * sin(time + 6.2831 * randPoint);

			float2 diff = neighbor + randPoint - f;
			float currentDist = length(diff);
			
			// minDist = min(minDist, currentDist);
			minDist = currentDist;
		}
	}

	color += minDist;
	color += 1 - step(0.02, minDist);
	// color.r += step(0.98, f.x) + step(0.98, f.y);

	return color;
}

[numthreads(8, 8, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float3 noise = cellularNoise(DTid.xy, 5, 5);

	outNoiseTexture[DTid.xy] = float4(noise.x, noise.y, noise.z, 1);
}