// GPU Implementation of Waves

cbuffer cbSettings : register(b0)
{
    float gK1;
    float gK2;
    float gK3;
    float magnitude;

    int2 gDisturbIndex;
};

RWTexture2D<float> gPrevSolution : register(u0);
RWTexture2D<float> gCurrSolution : register(u1);
RWTexture2D<float> gOutput : register(u2);

#define N 16

[numthreads(N, N, 1)]
void WavesCS( uint3 dispatchThreadID : SV_DispatchThreadID )
{
    gOutput[dispatchThreadID.xy] = gK1 * gPrevSolution[dispatchThreadID.xy].r +
        gK2 * gCurrSolution[dispatchThreadID.xy].r +
        gK3 * (
            gCurrSolution[int2(dispatchThreadID.x + 1, dispatchThreadID.y)].r +
            gCurrSolution[int2(dispatchThreadID.x - 1, dispatchThreadID.y)].r +
            gCurrSolution[int2(dispatchThreadID.x, dispatchThreadID.y + 1)].r +
            gCurrSolution[int2(dispatchThreadID.x, dispatchThreadID.y - 1)].r);
}

//void Waves::Disturb(int i, int j, float magnitude)
//{
//	// Don't disturb boundaries.
//    assert(i > 1 && i < mNumRows - 2);
//    assert(j > 1 && j < mNumCols - 2);

//    float halfMag = 0.5f * magnitude;

//	// Disturb the ijth vertex height and its neighbors.
//    mCurrSolution[i * mNumCols + j].y += magnitude;
//    mCurrSolution[i * mNumCols + j + 1].y += halfMag;
//    mCurrSolution[i * mNumCols + j - 1].y += halfMag;
//    mCurrSolution[(i + 1) * mNumCols + j].y += halfMag;
//    mCurrSolution[(i - 1) * mNumCols + j].y += halfMag;
//}

[numthreads(1, 1, 1)]
void DisturbCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    int x = gDisturbIndex.x;
    int y = gDisturbIndex.y;

    float halfMag = 0.5f * magnitude;
    gOutput[int2(x, y)] += magnitude;
    gOutput[int2(x + 1, y)] += halfMag;
    gOutput[int2(x - 1, y)] += halfMag;
    gOutput[int2(x, y + 1)] += halfMag;
    gOutput[int2(x, y - 1)] += halfMag;
}