#include "stdafx.h"

// Copyright (c) advancedfx.org
//
// Last changes:
// 2016-11-01 dominik.matrixstorm.com
//
// First changes:
// 2014-11-26 dominik.matrixstorm.com

#include "csgo_CViewRender.h"

#include "SourceInterfaces.h"
#include "AfxStreams.h"
#include "addresses.h"
#include <shared/detours.h>

void * detoured_csgo_CViewRender_RenderView_AfterVGui_DrawHud;

void DoOnDrawingHud(void)
{
	g_AfxStreams.OnDrawingHud();
}

void __declspec(naked) touring_csgo_CViewRender_RenderView_AfterVGui_DrawHud(void)
{
	__asm ; it's safe to do a call without preauctions here, because a call would happen right after anyways
	__asm call DoOnDrawingHud

	__asm jmp detoured_csgo_CViewRender_RenderView_AfterVGui_DrawHud
}

typedef void (__stdcall * CCSViewRender_RenderView_t)(DWORD * this_ptr, const SOURCESDK::CViewSetup_csgo &view, const SOURCESDK::CViewSetup_csgo &hudViewSetup, int nClearFlags, int whatToDraw);

CCSViewRender_RenderView_t detoured_CCSViewRender_RenderView;

float g_csgo_OldSmokeOverlayAlphaFactor;
float g_csgo_AfxSmokeOverlayAlphaMod = 1.0f;

void _stdcall touring_CCSViewRender_RenderView(DWORD * this_ptr, const SOURCESDK::CViewSetup_csgo &view, const SOURCESDK::CViewSetup_csgo &hudViewSetup, int nClearFlags, int whatToDraw)
{
	float * smokeOverlayAlphaFactor = (float *)((const char *)this_ptr + 0x588);

	g_csgo_OldSmokeOverlayAlphaFactor = *smokeOverlayAlphaFactor;

	g_csgo_AfxSmokeOverlayAlphaMod = g_AfxStreams.OnRenderSmokeOverlayAlphaMod();

	if(g_csgo_AfxSmokeOverlayAlphaMod < 1.0f)
		*smokeOverlayAlphaFactor = 0.0f; // force drawing world

	return detoured_CCSViewRender_RenderView(this_ptr, view, hudViewSetup, nClearFlags, whatToDraw);
}

void * detoured_csgo_CViewRender_RenderSmokeOverlay_OnLoadOldAlpha;

void __declspec(naked) touring_csgo_CViewRender_RenderSmokeOverlay_OnLoadOldAlpha(void)
{
	// restore old value:
	__asm push eax
	__asm mov eax, g_csgo_OldSmokeOverlayAlphaFactor
	__asm mov dword ptr[edx + 588h], eax
	__asm pop eax

	__asm jmp detoured_csgo_CViewRender_RenderSmokeOverlay_OnLoadOldAlpha
}

void * detoured_csgo_CViewRender_RenderSmokeOverlay_OnLoadAlphaBeforeDraw;

void __declspec(naked) touring_csgo_CViewRender_RenderSmokeOverlay_OnLoadAlphaBeforeDraw(void)
{
	// store new old value:
	__asm push eax
	__asm mov eax, dword ptr[edx + 588h]
	__asm mov g_csgo_OldSmokeOverlayAlphaFactor, eax
	__asm pop eax

	// calculate target value:
	__asm movss xmm1, dword ptr[edx + 588h]
	__asm mulss xmm1, g_csgo_AfxSmokeOverlayAlphaMod
	__asm movss dword ptr [edx + 588h], xmm1

	__asm jmp detoured_csgo_CViewRender_RenderSmokeOverlay_OnLoadAlphaBeforeDraw
}

void * detoured_csgo_CViewRender_RenderSmokeOverlay_OnBeforeExitFunc;

void __declspec(naked) touring_csgo_CViewRender_RenderSmokeOverlay_OnBeforeExitFunc(void)
{
	// restore old value:
	__asm push eax
	__asm mov eax, ebp
	__asm and eax, 0FFFFFFF0h
	__asm sub eax, 48h
	__asm sub eax, 0x4
	__asm sub eax, 0x4
	__asm mov edx, [eax + 18h] ; this ptr should be here
	__asm mov eax, g_csgo_OldSmokeOverlayAlphaFactor
	__asm mov dword ptr[edx + 588h], eax
	__asm pop eax

	__asm jmp detoured_csgo_CViewRender_RenderSmokeOverlay_OnBeforeExitFunc
}

bool csgo_CViewRender_Install(void)
{
	static bool firstResult = true;
	static bool firstRun = true;
	if(!firstRun) return firstResult;
	firstRun = false;

	if(AFXADDR_GET(csgo_CViewRender_RenderView_AfterVGui_DrawHud))
	{
		detoured_csgo_CViewRender_RenderView_AfterVGui_DrawHud = (void *)DetourApply((BYTE *)AFXADDR_GET(csgo_CViewRender_RenderView_AfterVGui_DrawHud), (BYTE *)touring_csgo_CViewRender_RenderView_AfterVGui_DrawHud, 0x5);
		
		// update original call offset:
		DWORD * pCalladdr = (DWORD *)((BYTE *)detoured_csgo_CViewRender_RenderView_AfterVGui_DrawHud +0x1);
		*pCalladdr = *pCalladdr -((DWORD)detoured_csgo_CViewRender_RenderView_AfterVGui_DrawHud -AFXADDR_GET(csgo_CViewRender_RenderView_AfterVGui_DrawHud));
	}
	else
		firstResult = false;

	if (AFXADDR_GET(csgo_CCSViewRender_RenderSmokeOverlay_OnLoadOldAlpha)
		&& AFXADDR_GET(csgo_CCSViewRender_RenderView)
		&& AFXADDR_GET(csgo_CCSViewRender_RenderSmokeOverlay_OnLoadAlphaBeforeDraw)
		&& AFXADDR_GET(csgo_CCSViewRender_RenderSmokeOverlay_OnBeforeExitFunc))
	{
		detoured_csgo_CViewRender_RenderSmokeOverlay_OnLoadOldAlpha = (void *)DetourApply((BYTE *)AFXADDR_GET(csgo_CCSViewRender_RenderSmokeOverlay_OnLoadOldAlpha), (BYTE *)touring_csgo_CViewRender_RenderSmokeOverlay_OnLoadOldAlpha, 8);
		detoured_CCSViewRender_RenderView = (CCSViewRender_RenderView_t)DetourClassFunc((BYTE *)AFXADDR_GET(csgo_CCSViewRender_RenderView), (BYTE *)touring_CCSViewRender_RenderView, (int)AFXADDR_GET(csgo_CCSViewRender_RenderView_DSZ));
		detoured_csgo_CViewRender_RenderSmokeOverlay_OnLoadAlphaBeforeDraw = (void *)DetourApply((BYTE *)AFXADDR_GET(csgo_CCSViewRender_RenderSmokeOverlay_OnLoadAlphaBeforeDraw), (BYTE *)touring_csgo_CViewRender_RenderSmokeOverlay_OnLoadAlphaBeforeDraw, 8);
		detoured_csgo_CViewRender_RenderSmokeOverlay_OnBeforeExitFunc = (void *)DetourApply((BYTE *)AFXADDR_GET(csgo_CCSViewRender_RenderSmokeOverlay_OnBeforeExitFunc), (BYTE *)touring_csgo_CViewRender_RenderSmokeOverlay_OnBeforeExitFunc, 8);
	}
	else
		firstResult = false;

	return firstResult;
}
