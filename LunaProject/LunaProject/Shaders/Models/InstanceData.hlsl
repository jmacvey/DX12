struct InstanceData
{
    float4x4 World;
    float4x4 TexTransform;
    uint MatIndex;
    uint DiffuseMapIndex;
    uint instancePad1;
    uint instancePad2;
};