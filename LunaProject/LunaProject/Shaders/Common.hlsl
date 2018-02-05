#include "Models/PassSettings.hlsl" // pass Settings in b0
#include "Models/MaterialData.hlsl" 
#include "Models/InstanceData.hlsl"

TextureCube gCubeMap : register(t0);
StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);
StructuredBuffer<InstanceData> gInstanceData : register(t1);
Texture2D gDiffuseMap : register(t2);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);