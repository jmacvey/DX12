struct InstanceData
{
    float4x4 World;
    float4x4 TexTransform;
    uint matIndex;
    uint instancePad0;
    uint instancePad1;
    uint instancePad2;
};