float4 PS(VertexOut pin) : SV_Target
{
	#ifdef COLOR
		return gColor;
	#endif

	// Interpolating normal can unnormalize it, so renormalize it.
	pin.NormalW = normalize(pin.NormalW);

	// Vector from point being lit to eye. 
	float3 toEyeW = gEyePosW - pin.PosW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

						 // Indirect lighting.

	float4 texDiffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC);
	float4 diffuseAlbedo = texDiffuseAlbedo * gDiffuseAlbedo;

	#ifdef ALPHA_TEST
		clip(diffuseAlbedo.a - 0.1f);
	#endif

	float4 ambient = gAmbientLight*diffuseAlbedo;

	const float shininess = 1.0f - gRoughness;
	Material mat = { diffuseAlbedo, gFresnelR0, shininess };
	float3 shadowFactor = 1.0f;
	float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
		pin.NormalW, toEyeW, shadowFactor);

	float4 litColor = ambient + directLight;

	#ifdef FOG
		float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
		litColor = lerp(litColor, gFogColor, fogAmount);
	#endif

	// Common convention to take alpha from diffuse material.
	litColor.a = diffuseAlbedo.a;

	return litColor;
}