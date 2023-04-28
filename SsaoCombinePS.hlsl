
struct VertexToPixel
{
	float4 screenPosition	: SV_POSITION;
	float2 uv				: TEXCOORD0;
};

// Textures
Texture2D SceneColorsNoAmbient	: register(t0);
Texture2D Ambient				: register(t1);
Texture2D SSAOBlur				: register(t2);

// Samplers
SamplerState BasicSampler		: register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
	// Sample textures
	float3 sceneColors = SceneColorsNoAmbient.Sample(BasicSampler, input.uv).rgb;
	float3 ambient = Ambient.Sample(BasicSampler, input.uv).rgb;
	float ao = SSAOBlur.Sample(BasicSampler, input.uv).r;

	// Combine
	return float4(ambient * ao + sceneColors, 1);
}