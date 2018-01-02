// weights come in unnormaled: exp(-x^2 / (2*sigma_d^2)

cbuffer cbSettings : register(b0)
{
    int gBlurRadius;
    float twoSigmaRSquared;
    float w0;
    float w1;
    float w2;
    float w3;
    float w4;
    float w5;
    float w6;
    float w7;
    float w8;
    float w9;
    float w10;
};

static const int gMaxBlurRadius = 5;

Texture2D gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);

#define N 256
#define CacheSize (N + 2*gMaxBlurRadius)

groupshared float4 gCache[CacheSize];

// For all horizontal texture coordinate x across each y coordinate:
// 1. for tex coordinatei in [x - gMaxBlurRadius, x + gMaxBlurRadius]
// 2. sample the coordinatefor (x, y) and (i, y), and calculate f_r(i)
// where f_r(i) = exp(- |I(i) - I(x)|^2) / (2*sigma_r*sigma_r) )

// 3. Multiply f_r(i) by corresponding spatial weight w_i to produce term w*(i)
// 4.1: add allw*(i) for coordinate to produce normalizing factor W_p
// 4.2: multiply allw*(i) by corresponding image intensityI(i,y) and sum
// them to form I*
// 5. assign to image texelI(x, y) the value I*/W_p

// With spatial weights, we still need intensity weight
// requires sampling the texture at given point x_k
// for [i-blurRadius, i+gBlurRadius]
// then apply the gaussian function 

[numthreads(N, 1, 1)]
void HorzBlurCS(int3 groupThreadID: SV_GroupThreadID,
    int3 dispatchThreadID : SV_DispatchThreadID)
{
    float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };

	if (groupThreadID.x < gBlurRadius) {

		// Clamp out of bound samples that occur at image borders
		int x = max(dispatchThreadID.x - gBlurRadius, 0);
		gCache[groupThreadID.x] = gInput[int2(x, dispatchThreadID.y)];
	}

	if (groupThreadID.x >= N - gBlurRadius) {
		
		// Clamp out of bound samples that occur at image border
		int x = min(dispatchThreadID.x + gBlurRadius, gInput.Length.x - 1);
		gCache[groupThreadID.x + 2 * gBlurRadius] = gInput[int2(x, dispatchThreadID.y)];
	}

	// clamp out of bound samples that occur at image border
	gCache[groupThreadID.x + gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy - 1)];

    GroupMemoryBarrierWithGroupSync();

    float4 blurColor = float4(0, 0, 0, 0);
    float w_p = 0.0f;
    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        int k = groupThreadID.x + gBlurRadius + i;
        float f_r = exp(-pow(length(gCache[groupThreadID.x] - gCache[k]), 2) / twoSigmaRSquared);
        float w_i = f_r * weights[i + gBlurRadius];
        w_p += w_i;
        blurColor += w_i * gCache[k];
    }

    blurColor /= w_p; // <- normalize weights
    gOutput[dispatchThreadID.xy] = blurColor;
}

[numthreads(1, N, 1)]
void VertBlurCS(int3 groupThreadID : SV_GroupThreadID,
    int3 dispatchThreadID : SV_DispatchThreadID)
{
    float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };

	// clamp top
    if (groupThreadID.y < gBlurRadius)
    {
        int y = max(dispatchThreadID.y - gBlurRadius, 0);
        gCache[groupThreadID.y] = gInput[int2(dispatchThreadID.x, y)];
    }

	// clamp bottom
    if (groupThreadID.y >= N - gBlurRadius)
    {
        int y = min(dispatchThreadID.y + gBlurRadius, gInput.Length.y - 1);
        gCache[groupThreadID.y + 2 * gBlurRadius] = gInput[int2(dispatchThreadID.x, y)];
    }

	// clamp boundary
    gCache[groupThreadID.y + gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy - 1)];

    GroupMemoryBarrierWithGroupSync();

    float w_p = 0.0f;
    float4 blurColor = float4(0, 0, 0, 0);

    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        int k = groupThreadID.y + gBlurRadius + i;
        float f_r = exp(-pow(length(gCache[groupThreadID.y] - gCache[k]), 2) / twoSigmaRSquared);
        float w_i = f_r * weights[i + gBlurRadius];
        w_p += w_i;
        blurColor += w_i * gCache[k];
    }
    blurColor /= w_p;
    gOutput[dispatchThreadID.xy] = blurColor;
}