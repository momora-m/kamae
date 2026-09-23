// Lit cube. Compiled at runtime with D3DCompile (vs_5_0 / ps_5_0).
// Matrices are row-major so they match DirectXMath's XMFLOAT4X4 layout.
// Transform order is mul(position, matrix): row vector on the left.

cbuffer Frame : register(b0)
{
    row_major float4x4 world;
    row_major float4x4 viewProjection;
    float4 lightDirection;
    float4 lightColor;
    float4 ambientColor;
    float4 cameraPosition;
    float4 albedo;
};

struct VsInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct VsOutput
{
    float4 position : SV_Position;
    float3 worldNormal : NORMAL;
    float3 worldPosition : TEXCOORD0;
};

VsOutput VSMain(VsInput input)
{
    VsOutput output;
    float4 worldPosition = mul(float4(input.position, 1.0f), world);
    output.worldPosition = worldPosition.xyz;
    output.position = mul(worldPosition, viewProjection);
    output.worldNormal = mul(input.normal, (float3x3)world);
    return output;
}

float4 PSMain(VsOutput input) : SV_Target
{
    float3 normal = normalize(input.worldNormal);
    float3 toLight = normalize(lightDirection.xyz);
    float diffuse = saturate(dot(normal, toLight));

    float3 toCamera = cameraPosition.xyz - input.worldPosition;
    float3 halfVector = normalize(toCamera + toLight);
    float specular = pow(saturate(dot(normal, halfVector)), 48.0f);

    float3 color = albedo.rgb * (ambientColor.rgb + lightColor.rgb * diffuse);
    color += lightColor.rgb * specular * 0.35f;
    return float4(saturate(color), 1.0f);
}
