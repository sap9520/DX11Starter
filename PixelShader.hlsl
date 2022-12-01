#include "ShaderIncludes.hlsli"

#define LIGHT_TYPE_DIRECTIONAL	0
#define LIGHT_TYPE_POINT		1
#define LIGHT_TYPE_SPOT			2
#define MAX_SPECULAR_EXPONENT 256.0f

#define NUM_LIGHTS				5

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

	Light lights[5];
}

Texture2D Albedo			: register(t0);
Texture2D NormalMap			: register(t1);
Texture2D RoughnessMap		: register(t2);
Texture2D MetalnessMap		: register(t3);
SamplerState BasicSampler	: register(s0);

static const float F0_NON_METAL = 0.04f;
static const float MIN_ROUGHNESS = 0.0000001f;
static const float PI = 3.14159265359f;

float Attenuate(Light light, float3 worldPos) {
	float dist = distance(light.Position, worldPos);
	float att = saturate(1.0f - (dist * dist / (light.Range * light.Range)));
	return att * att;
}

float DiffusePBR(float3 normal, float3 dirToLight) {
	return saturate(dot(normal, dirToLight));
}

float3 DiffuseEnergyConserve(float3 diffuse, float3 specular, float metalness) {
	return diffuse * ((1 - saturate(specular)) * (1 - metalness));
}

float SpecDistribution(float3 n, float3 h, float roughness) {
	float NdotH = saturate(dot(n, h));
	float NdotH2 = NdotH * NdotH;
	float a = roughness * roughness;
	float a2 = max(a * a, MIN_ROUGHNESS);

	float denomToSquare = NdotH2 * (a2 - 1) + 1;

	return a2 / (PI * denomToSquare * denomToSquare);
}

float3 Fresnel(float3 v, float3 h, float3 f0) {
	float VdotH = saturate(dot(v, h));

	return f0 + (1 - f0) * pow(1 - VdotH, 5);
}

float GeometricShadowing(float3 n, float3 v, float roughness) {
	float k = pow(roughness + 1, 2) / 8.0f;
	float NdotV = saturate(dot(n, v));

	return NdotV / (NdotV * (1 - k) + k);
}

float3 MicrofacetBRDF(float3 n, float3 l, float3 v, float roughness, float3 specColor) {
	float3 h = normalize(v + 1);

	float D = SpecDistribution(n, h, roughness);
	float3 F = Fresnel(v, h, specColor);
	float G = GeometricShadowing(n, v, roughness) * GeometricShadowing(n, l, roughness);

	return (D * F * G) / (4 * max(dot(n, v), dot(n, l)));
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
	float3 surfaceColor = pow(Albedo.Sample(BasicSampler, input.uv).rgb * colorTint.rgb, 2.2f);

	float3 unpackedNormal = NormalMap.Sample(BasicSampler, input.uv).rgb * 2 - 1;
	
	float roughness = RoughnessMap.Sample(BasicSampler, input.uv).r;

	float metalness = MetalnessMap.Sample(BasicSampler, input.uv).r;

	// Assume albedo texture has spec color where metalness == 1
	float3 specColor = lerp(F0_NON_METAL.rrr, surfaceColor.rgb, metalness);

	input.normal = normalize(input.normal);
	input.tangent = normalize(input.tangent);

	// Gram-Schmidt orthonormalization
	float3 T = normalize(input.tangent - input.normal * dot(input.tangent, input.normal));
	float3 B = cross(T, input.normal);
	float3x3 TBN = float3x3(T, B, input.normal);
	input.normal = mul(unpackedNormal, TBN);

	float3 total;

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

		float diffuse = DiffusePBR(input.normal, dirNormalized);

		float3 toCam = normalize(cameraPosition - input.worldPosition);
		float3 spec = MicrofacetBRDF(input.normal, dirNormalized, toCam, roughness, specColor);

		float3 balancedDiff = DiffuseEnergyConserve(diffuse, spec, metalness);

		switch (currentLight.Type) {
			case LIGHT_TYPE_DIRECTIONAL:
				total += (balancedDiff * surfaceColor + spec) * currentLight.Intensity * currentLight.Color;
				break;
			case LIGHT_TYPE_POINT:
				total += ((balancedDiff * surfaceColor + spec) * currentLight.Intensity * currentLight.Color)
					* Attenuate(currentLight, input.worldPosition);
				break;
		}
	}

	return float4(pow(total, 1.0f / 2.2f), 1);
}