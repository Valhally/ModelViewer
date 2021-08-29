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
    float3 color : TEXCOORD1;
};

PSInput VSMain(float3 position : POSITION, float3 normal : NORMAL, float3 color : COLOR)
{
    PSInput result;
 
    result.position = float4(position, 1.0);
    result.worldPos = result.position.xyz;
    result.position = mul(result.position, view);
    result.position = mul(result.position, projection);
    result.color = normal;

    return result;
}

static const float maxDistance = 20000;
static const float disT = 50000;
static const float3 backgroundColor = float3(55, 56, 59) / 255;
static const float tmp = 200.0 / 255.0;

float4 PSMain(PSInput input) : SV_TARGET
{
	float dis = length(input.worldPos);
    if (input.color.x > tmp || input.color.y > tmp || input.color.z > tmp)
    {
        if (dis > disT)
            discard;
        else
			return float4(input.color, 1.0);
    }

    float t1 = dis / maxDistance;
    if (t1 > 1.0)
        t1 = 1;
    t1 = 1.0 - t1;
    return float4(input.color * t1 + backgroundColor * (1 - t1), 1.0); 
}