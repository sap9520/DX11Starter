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
	float3 ambient;

	Light lights[5];
}


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
	input.normal = normalize(input.normal);

	float3 pixColor = ambient * colorTint;

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
		float R = reflect(-dirNormalized, input.normal);
		float spec = pow(saturate(dot(R, V)), specExponent);

		pixColor += (diffuse + spec) * currentLight.Color * colorTint;

		switch (currentLight.Type) {
			case LIGHT_TYPE_DIRECTIONAL:
				pixColor += (diffuse + spec) * currentLight.Color * colorTint;
				break;
			case LIGHT_TYPE_POINT:
				pixColor += ((diffuse + spec) * currentLight.Color * colorTint) 
					* Attenuate(currentLight, input.worldPosition);
				break;
		}
	}


	return float4(pixColor, 1);
}