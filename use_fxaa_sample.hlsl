	float4 ret = tex.Sample(smp,input.uv);
	FxaaTex InputFXAATex = { smp, tex };
	float3 aa=FxaaPixelShader(
		input.uv,							// FxaaFloat2 pos,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),		// FxaaFloat4 fxaaConsolePosPos,
		InputFXAATex,							// FxaaTex tex,
		InputFXAATex,							// FxaaTex fxaaConsole360TexExpBiasNegOne,
		InputFXAATex,							// FxaaTex fxaaConsole360TexExpBiasNegTwo,
		float2(dx,dy),							// FxaaFloat2 fxaaQualityRcpFrame,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),		// FxaaFloat4 fxaaConsoleRcpFrameOpt,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),		// FxaaFloat4 fxaaConsoleRcpFrameOpt2,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),		// FxaaFloat4 fxaaConsole360RcpFrameOpt2,
		0.75f,									// FxaaFloat fxaaQualitySubpix,
		0.166f,									// FxaaFloat fxaaQualityEdgeThreshold,
		0.0833f,								// FxaaFloat fxaaQualityEdgeThresholdMin,
		0.0f,									// FxaaFloat fxaaConsoleEdgeSharpness,
		0.0f,									// FxaaFloat fxaaConsoleEdgeThreshold,
		0.0f,									// FxaaFloat fxaaConsoleEdgeThresholdMin,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f)		// FxaaFloat fxaaConsole360ConstDir,
	).rgb;