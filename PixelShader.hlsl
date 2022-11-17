#include "ShaderIncludes.hlsli"

#define LIGHT_TYPE_DIRECTIONAL	0
#define LIGHT_TYPE_POINT		1
#define LIGHT_TYPE_SPOT			2
#define MAX_SPECULAR_EXPONENT 256.0f

#define NUM_LIGHTS				1

struct Light {
	int Type;
	float3 Direction;
	float Range;
	float3 Position;
	float Intensity;
	float3 Color;
	float SpotFalloff;
	float3 Padding;
};


cbuffer ExternalData : register(b0)
{
	float4 colorTint;
	float3 cameraPosition;
	float roughness;
	float3 ambient;

	Light lights[5];
}

Texture2D SurfaceTexture : register(t0);
Texture2D SpecularTexture : register(t1);
Texture2D NormalMap : register(t2);
SamplerState BasicSampler : register(s0);

float GetDiffuseLight(float3 normal, float3 dirToLight) {
	return saturate(dot(normal, dirToLight));
}

float Attenuate(Light light, float3 worldPos) {
	float dist = distance(light.Position, worldPos);
	float att = saturate(1.0f - (dist * dist / (light.Range * light.Range)));
	return att * att;
}

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
	float3 surfaceColor = pow(SurfaceTexture.Sample(BasicSampler, input.uv).rgb * colorTint.rgb, 2.2f);
	float3 pixColor = surfaceColor * ambient;

	float specMap = SpecularTexture.Sample(BasicSampler, input.uv).r;
	float3 unpackedNormal = NormalMap.Sample(BasicSampler, input.uv).rgb * 2 - 1;

	input.normal = normalize(input.normal);
	input.tangent = normalize(input.tangent);

	// Gram-Schmidt orthonormalization
	float3 T = normalize(input.tangent - input.normal * dot(input.tangent, input.normal));
	float3 B = cross(T, input.normal);
	float3x3 TBN = float3x3(T, B, input.normal);
	input.normal = mul(unpackedNormal, TBN);

	for (int i = 0; i < NUM_LIGHTS; i++) {
		Light currentLight = lights[i];

		float3 dirNormalized;

		switch (currentLight.Type) {
			case LIGHT_TYPE_DIRECTIONAL:
				dirNormalized = normalize(-currentLight.Direction);
				break;
			case LIGHT_TYPE_POINT:
				dirNormalized = normalize(currentLight.Position - input.worldPosition);
				break;
		}

		float diffuse = GetDiffuseLight(input.normal, dirNormalized);

		float specExponent = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;
		float3 V = normalize(cameraPosition - input.worldPosition);
		float3 R = reflect(-dirNormalized, input.normal);
		float3 spec = pow(saturate(dot(R, V)), specExponent) * specMap;

		spec *= any(diffuse);
		pixColor += (diffuse + spec) * currentLight.Color * (float3)surfaceColor;

		switch (currentLight.Type) {
			case LIGHT_TYPE_DIRECTIONAL:
				pixColor += (diffuse + spec) * currentLight.Color * (float3)surfaceColor;
				break;
			case LIGHT_TYPE_POINT:
				pixColor += ((diffuse + spec) * currentLight.Color * (float3)surfaceColor)
					* Attenuate(currentLight, input.worldPosition);
				break;
		}
	}

	return float4(pow(pixColor, 1.0f / 2.2f), 1);
}