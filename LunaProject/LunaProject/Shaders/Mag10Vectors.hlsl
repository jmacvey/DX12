#define N 64

// 3D buffer of vectors
StructuredBuffer<float3> gVectors : register(t0);
RWStructuredBuffer<float> gMagnitudes : register(u0);


// num threads per group is 64
[numthreads(N, 1, 1)]
void Mag10VectorsCS( uint3 dispatchThreadID : SV_DispatchThreadID )
{
    gMagnitudes[dispatchThreadID.x] = (length(gVectors[dispatchThreadID.x]));
}