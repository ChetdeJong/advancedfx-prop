#include "stdafx.h"

// Copyright (c) advancedfx.org
//
// Last changes:
// 2015-01-20 dominik.matrixstorm.com
//
// First changes:
// 2015-01-20 dominik.matrixstorm.com

#include "csgo_SndMixTimeScalePatch.h"

#include "addresses.h"

#include <shared/detours.h>

bool csgo_SndMixTimeScalePatch_enable = false;
float csgo_SndMixTimeScalePatch_value = 1.0f;

float csgo_SndMixTimeScalePatch_dummy;

typedef void (*csgo_Host_RunFrame_t) (void);

DWORD detourded_csgo_SndMixTimeScalePatch = 0;

__declspec(naked) void touring_csgo_SndMixTimeScalePatch(void)
{
	__asm
	{
		CMP csgo_SndMixTimeScalePatch_enable, 0
		JZ __Continue

		FSTP csgo_SndMixTimeScalePatch_dummy
		FLD csgo_SndMixTimeScalePatch_value

		__Continue:
		JMP [detourded_csgo_SndMixTimeScalePatch]
	}
}

bool Hook_csgo_SndMixTimeScalePatch(void)
{
	static bool firstRun = true;
	static bool firstResult = false;

	if(!firstRun) return firstResult;
	firstRun = false;

	if(AFXADDR_GET(csgo_snd_mix_timescale_patch))
	{
		detourded_csgo_SndMixTimeScalePatch = (DWORD)DetourApply((BYTE *)AFXADDR_GET(csgo_snd_mix_timescale_patch), (BYTE *)touring_csgo_SndMixTimeScalePatch, (int)AFXADDR_GET(csgo_snd_mix_timescale_patch_DSZ));
	
		firstResult = true;
	}

	return firstResult;
}
