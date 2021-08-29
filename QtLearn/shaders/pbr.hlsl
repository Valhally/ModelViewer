cbuffer cbPass : register(b0)
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;

    float3 albedo; 
    float metallic; 
    float roughness; 
    float ao; 

    int lightCount;
    float unused;

    float4 lightPos[8]; 

    float4 lightColor[8]; 

    float3 camearaPos; 
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 color : TEXCOORD2;
};

PSInput VSMain(float3 position : POSITION, float3 normal : NORMAL, float3 color : COLOR)
{
    PSInput result;
	result.position = mul(float4(position, 1.0), model);
    result.worldPos = result.position.xyz;
    result.position = mul(result.position, view);
    result.position = mul(result.position, projection);
    result.normal = mul(float4(normal, 0.0), model).xyz;
    result.color = color;
    return result;
}

static const float PI = 3.14159265359;

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0000001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}
// ----------------------------------------------------------------------------

float4 PSMain(PSInput input) : SV_TARGET
{
    input.color += float3(1, 1, 1) * 0.1;
    float3 N = normalize(input.normal);
    float3 V = normalize(camearaPos - input.worldPos);

    float3 F0 = float3(0.4, 0.4, 0.4);
    F0 = lerp(F0, input.color, metallic);

    float3 Lo = float3(0.0, 0.0, 0.0); 
    for (int i = 0; i < lightCount; ++i)
    {
        float3 L = normalize(lightPos[i] - input.worldPos);
        float3 H = normalize(V + L);
        float distance = length(lightPos[i] - input.worldPos);
        //return float4(float3(distance, distance, distance) / 1000, 1.0);
        float attenuation = 1.0 / (distance * distance);
        float3 radiance = lightColor[i] * attenuation;

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

        float3 numerator = NDF * G * F;
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        float3 specular = numerator / max(denominator, 0.001);

        float3 kS = F;
        float3 kD = float3(1.0, 1.0, 1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * input.color / PI + specular) * radiance * NdotL;
    }

    float3 ambient = float3(0.03, 0.03, 0.03) * input.color * ao;

    float3 color = ambient + Lo;

    color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, float3(1.0, 1.0, 1.0) / 2.2);  

    return float4(color, 1.0); 
}

