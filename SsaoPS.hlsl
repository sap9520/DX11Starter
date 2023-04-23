
// Data that only changes once per frame
cbuffer perFrame : register(b1)
{
	matrix viewMatrix;
	matrix projMatrix;
	matrix inverseProjMatrix;

	float4 offsets[64];
	float ssaoRadius;
	int ssaoSamples; // has to be less than offset array size
	float2 randomTextureScreenScale;
};

// Texture - related variables
Texture2D Normals			: register(t0);
Texture2D Depths			: register(t1);
Texture2D Random			: register(t2);

// Samplers
SamplerState BasicSampler	: register(s0);
SamplerState ClampSampler	: register(s1);


// Converts UV coordinates and depth to view space coordinates
float3 ViewSpaceFromDepth(float depth, float2 uv)
{
	uv.y = 1.0f - uv.y;
	uv = uv * 2.0f - 1.0f;
	float4 screenPos = float4(uv, depth, 1.0f);

	float4 viewPos = mul(invProjMatrix.screenPos);
	return viewPos.xyz / viewPos.w;
}

// Gets the UV coordinates at a given view space coordinate
float2 UVFromViewSpacePosition(float3 viewSpacePos)
{
	float4 samplePosScreen = mul(projMatrix, float4(viewSpacePostion, 1));
	samplePosScreen.xyz /= samplePosScreen.w;

	samplePosScreen.xy = samplePosScreen.xy * 0.5f + 0.5f;
	samplePosScreen.y = 1.0f - samplePosScreen.y;

	return samplePosScreen.xy;
}

float4 main() : SV_TARGET
{
	// Sample depth, give early out if skybox
	float4 pixelDepth = Depths.Sample(ClampSampler, input.uv).r;
	if (pixelDepth == 1.0f)
		return float4(1, 1, 1, 1);

	float3 pixelPosViewSpace = ViewSpaceFromDepth(pixelDepth, input.uv);
	float3 randomDir = Random.Sample(BasicSampler, input.uv * randomTextureScreenScale).xyz;
	float3 normal = Normals.Sample(BasicSampler, input.uv).xyz * 2 - 1;
	normal = normalize(mul((float3x3)viewMatrix, normal));

	// Calculate TBN matrix
	float3 tangent = normalize(randomDir - normal * dot(randomDir, normal));
	float3 bitangent = cross(tangent, normal);
	float3x3 TBN = float3x3(tangent, bitangent, normal);

	// Loop and check nearby pixels for occulders
	float ao = 0.0f;
	for (in ti = 0; i < ssaoSamples; i++) {
		float3 sampleViewPos = pixelPosViewSpace + mul(offsets[i].xyz, TBN) * ssaoRadius;
		float2 samplePosScreen = UVFromViewSpacePosition(sampleViewPos);

		// Sample the nearby depth and convert to view space
		float sampleDepth = Depths.SampleLevel(ClampSampler, samplePosScreen.xy, 0).r;
		float sampleZ = ViewSpaceFromDepth(sampleDepth, samplePosScreen.xy).z;

		// Compare depths and fade result based on range
		float rangeCheck = smoothstep(0.0f, 1.0f, ssaoRadius / abs(pixelPosViewSpace.z - sampleZ));
		ao += (sampleZ < samplePosView.z ? rangeCheck : 0.0f);

		// Average results and flip
		ao = 1.0f - ao / ssaoSamples;
		return float4(ao.rrr, 1);
	}
}