#include "stdafx.h"

// Copyright (c) advancedfx.org
//
// Last changes:
// 2017-03-13 dominik.matrixstorm.com
//
// First changes:
// 2009-09-29 dominik.matrixstorm.com


// TODO:
// - Memory for Wrp* is never freed atm

#include <windows.h>

#include <shared/StringTools.h>

#include <shared/detours.h>

#include "addresses.h"
#include "RenderView.h"
#include "SourceInterfaces.h"
#include "WrpVEngineClient.h"
#include "WrpConsole.h"
#include "WrpGlobals.h"
#include "d3d9Hooks.h"
#include "csgo_SndMixTimeScalePatch.h"
#include "csgo_CSkyBoxView.h"
#include "AfxHookSourceInput.h"
#include "AfxClasses.h"
#include "AfxStreams.h"
#include "hlaeFolder.h"
#include "CampathDrawer.h"
#include "csgo_ScaleForm_Hooks.h"
#include "asmClassTools.h"
#include "csgo_Stdshader_dx9_Hooks.h"
#include "AfxShaders.h"
#include "csgo_CViewRender.h"
#include "CommandSystem.h"
#include "csgo_writeWaveConsoleCheck.h"
#include "ClientTools.h"
#include "MatRenderContextHook.h"
//#include "csgo_IPrediction.h"
#include "csgo_MemAlloc.h"
#include "csgo_c_baseanimatingoverlay.h"
#include "MirvPgl.h"

#include <set>
#include <map>
#include <string>

WrpVEngineClient * g_VEngineClient = 0;
SOURCESDK::ICvar_003 * g_Cvar = 0;


void ErrorBox(char const * messageText) {
	MessageBoxA(0, messageText, "Error - AfxHookSource", MB_OK|MB_ICONERROR);
}

void ErrorBox() {
	ErrorBox("Something went wrong.");
}

char const * g_Info_VClient = "NULL";
char const * g_Info_VEngineClient = "NULL";
char const * g_Info_VEngineCvar = "NULL";

void PrintInfo() {
	Tier0_Msg(
		"|" "\n"
		"| AfxHookSource (" __DATE__ " " __TIME__ ")" "\n"
		"| http://advancedfx.org/" "\n"
		"|" "\n"
	);

	Tier0_Msg("| VClient: %s\n", g_Info_VClient);
	Tier0_Msg("| VEngineClient: %s\n", g_Info_VEngineClient);
	Tier0_Msg("| VEngineCvar: %s\n", g_Info_VEngineCvar);
	Tier0_Msg("| GameDirectory: %s\n", g_VEngineClient ? g_VEngineClient->GetGameDirectory() : "n/a");
	Tier0_Msg("| WrpConCommands::GetVEngineCvar007() == 0x%08x\n", WrpConCommands::GetVEngineCvar007());

	Tier0_Msg("|" "\n");
}

SOURCESDK::IClientEngineTools_001 * g_Engine_ClientEngineTools;

class ClientEngineTools : public SOURCESDK::IClientEngineTools_001
{
public:
	// TODO: we should call the destructor of g_Engine_ClientEngineTools on destuction?

	virtual void LevelInitPreEntityAllTools() { g_Engine_ClientEngineTools->LevelInitPreEntityAllTools(); }
	virtual void LevelInitPostEntityAllTools() { g_Engine_ClientEngineTools->LevelInitPostEntityAllTools(); }
	virtual void LevelShutdownPreEntityAllTools() { g_Engine_ClientEngineTools->LevelShutdownPreEntityAllTools(); }
	virtual void LevelShutdownPostEntityAllTools() { g_Engine_ClientEngineTools->LevelShutdownPostEntityAllTools(); }
	virtual void PreRenderAllTools()
	{
		//Tier0_Msg("ClientEngineTools::PreRenderAllTools\n");
		g_Engine_ClientEngineTools->PreRenderAllTools();
	}
	
	virtual void PostRenderAllTools()
	{
		// Warning: This can be called multiple times during a frame (i.e. for skybox view and normal world view)!

		//Tier0_Msg("ClientEngineTools::PostRenderAllTools\n");

		g_CampathDrawer.OnPostRenderAllTools();

		if(g_Hook_VClient_RenderView.IsInstalled())
		{
			g_CommandSystem.Do_Queue_Commands(g_Hook_VClient_RenderView.GetCurTime());
		}

		g_Engine_ClientEngineTools->PostRenderAllTools();
	}

	virtual void PostToolMessage(SOURCESDK::HTOOLHANDLE hEntity, SOURCESDK::KeyValues_something *msg )
	{
		g_ClientTools.OnPostToolMessage((SOURCESDK::CSGO::HTOOLHANDLE)hEntity, (SOURCESDK::CSGO::KeyValues *)msg);
		g_Engine_ClientEngineTools->PostToolMessage(hEntity, msg);
	}
	virtual void AdjustEngineViewport( int& x, int& y, int& width, int& height )
	{
		g_Engine_ClientEngineTools->AdjustEngineViewport(x, y, width, height);

		g_Hook_VClient_RenderView.OnAdjustEngineViewport(x, y, width, height);
	}
	
	virtual bool SetupEngineView(SOURCESDK::Vector &origin, SOURCESDK::QAngle &angles, float &fov )
	{
		//Tier0_Msg("ClientEngineTools::SetupEngineView\n");
		bool bRet = g_Engine_ClientEngineTools->SetupEngineView(origin, angles, fov);

		g_Hook_VClient_RenderView.OnViewOverride(
			origin.x, origin.y, origin.z,
			angles.x, angles.y, angles.z,
			fov
		);

		return bRet;
	}
	
	virtual bool SetupAudioState(SOURCESDK::AudioState_t &audioState ) { return g_Engine_ClientEngineTools->SetupAudioState(audioState); }
	
	virtual void VGui_PreRenderAllTools( int paintMode )
	{
		//Tier0_Msg("ClientEngineTools::VGui_PreRenderAllTools\n");
		g_Engine_ClientEngineTools->VGui_PreRenderAllTools(paintMode);
	}
	
	virtual void VGui_PostRenderAllTools( int paintMode )
	{
		//Tier0_Msg("ClientEngineTools::VGui_PostRenderAllTools\n");
		g_Engine_ClientEngineTools->VGui_PostRenderAllTools(paintMode);
	}

	virtual bool IsThirdPersonCamera() { return g_Engine_ClientEngineTools->IsThirdPersonCamera(); }

	virtual bool InToolMode()
	{
		return g_Engine_ClientEngineTools->InToolMode() || g_ClientTools.GetRecording();
	}

} g_ClientEngineTools;

SOURCESDK::CreateInterfaceFn g_AppSystemFactory = 0;

bool isCsgo = false;
bool isV34 = false;

SOURCESDK::IMaterialSystem_csgo * g_MaterialSystem_csgo = 0;

SOURCESDK::IFileSystem_csgo * g_FileSystem_csgo = 0;

SOURCESDK::CSGO::vgui::IPanel * g_pVGuiPanel_csgo = 0;
SOURCESDK::CSGO::vgui::ISurface *g_pVGuiSurface_csgo = 0;

void AfxV34HookWindow(void);

void MySetup(SOURCESDK::CreateInterfaceFn appSystemFactory, WrpGlobals *pGlobals)
{
	static bool bFirstRun = true;

	if(bFirstRun)
	{
		bFirstRun = false;

		void *iface , *iface2;

		g_AppSystemFactory = appSystemFactory;

		if (!isCsgo && (iface = appSystemFactory(VENGINE_CLIENT_INTERFACE_VERSION_015, NULL)))
		{
			// This is not really 100% backward compatible, there is a problem with the CVAR interface or s.th..
			// But the guy that tested it wasn't available for further debugging, so I'll just leave it as
			// it is now. Will crash as soon as i.e. ExecuteCliendCmd is used, due to some crash
			// related to CVAR system.
			
			g_Info_VEngineClient = VENGINE_CLIENT_INTERFACE_VERSION_015;
			g_VEngineClient = new WrpVEngineClient_013((SOURCESDK::IVEngineClient_013 *)iface);
		}
		else
		if(isCsgo && (iface = appSystemFactory(VENGINE_CLIENT_INTERFACE_VERSION_014_CSGO, NULL)))
		{
			g_Info_VEngineClient = VENGINE_CLIENT_INTERFACE_VERSION_014_CSGO " (CS:GO)";
			g_VEngineClient = new WrpVEngineClient_014_csgo((SOURCESDK::IVEngineClient_014_csgo *)iface);
		}
		else
		if(iface = appSystemFactory(VENGINE_CLIENT_INTERFACE_VERSION_013, NULL))
		{
			g_Info_VEngineClient = VENGINE_CLIENT_INTERFACE_VERSION_013;
			g_VEngineClient = new WrpVEngineClient_013((SOURCESDK::IVEngineClient_013 *)iface);
		}
		else if(iface = appSystemFactory(VENGINE_CLIENT_INTERFACE_VERSION_012, NULL)) {
			g_Info_VEngineClient = VENGINE_CLIENT_INTERFACE_VERSION_012;
			g_VEngineClient = new WrpVEngineClient_012((SOURCESDK::IVEngineClient_012 *)iface);
		}
		else {
			ErrorBox("Could not get a supported VEngineClient interface.");
		}

		if((iface = appSystemFactory( CVAR_INTERFACE_VERSION_007, NULL )))
		{
			g_Info_VEngineCvar = CVAR_INTERFACE_VERSION_007;
			WrpConCommands::RegisterCommands((SOURCESDK::ICvar_007 *)iface);
		}
		else if((iface = appSystemFactory( VENGINE_CVAR_INTERFACE_VERSION_004, NULL )))
		{
			g_Info_VEngineCvar = VENGINE_CVAR_INTERFACE_VERSION_004;
			WrpConCommands::RegisterCommands((SOURCESDK::ICvar_004 *)iface);
		}
		else if(
			(iface = appSystemFactory( VENGINE_CVAR_INTERFACE_VERSION_003, NULL ))
			&& (iface2 = appSystemFactory(VENGINE_CLIENT_INTERFACE_VERSION_012, NULL))
		) {
			g_Info_VEngineCvar = VENGINE_CVAR_INTERFACE_VERSION_003 " & " VENGINE_CLIENT_INTERFACE_VERSION_012;
			WrpConCommands::RegisterCommands((SOURCESDK::ICvar_003 *)iface, (SOURCESDK::IVEngineClient_012 *)iface2);
		}
		else {
			ErrorBox("Could not get a supported VEngineCvar interface.");
		}

		if(iface = appSystemFactory(VCLIENTENGINETOOLS_INTERFACE_VERSION_001, NULL))
		{
			g_Engine_ClientEngineTools = (SOURCESDK::IClientEngineTools_001 *)iface;
		}
		else {
			ErrorBox("Could not get a supported VClientEngineTools interface.");
		}

		if(isCsgo)
		{
			if(iface = appSystemFactory(MATERIAL_SYSTEM_INTERFACE_VERSION_CSGO_80, NULL))
			{
				g_MaterialSystem_csgo = (SOURCESDK::IMaterialSystem_csgo *)iface;
				g_AfxStreams.OnMaterialSystem(g_MaterialSystem_csgo);
			}
			else {
				ErrorBox("Could not get a supported VMaterialSystem interface.");
			}

			if(iface = appSystemFactory(FILESYSTEM_INTERFACE_VERSION_CSGO_017, NULL))
			{
				g_FileSystem_csgo = (SOURCESDK::IFileSystem_csgo *)iface;
			}
			else {
				ErrorBox("Could not get a supported VFileSystem interface.");
			}

			if (iface = appSystemFactory(SOURCESDK_CSGO_VGUI_PANEL_INTERFACE_VERSION, NULL))
			{
				g_pVGuiPanel_csgo = (SOURCESDK::CSGO::vgui::IPanel *)iface;
			}
			else {
				ErrorBox("Could not get a supported VGUI_Panel interface.");
			}

			if (iface = appSystemFactory(SOURCESDK_CSGO_VGUI_VGUI_SURFACE_INTERFACE_VERSION, NULL))
			{
				g_pVGuiSurface_csgo = (SOURCESDK::CSGO::vgui::ISurface *)iface;
			}
			else {
				ErrorBox("Could not get a supported VGUI_Surface interface.");
			}

			if(iface = appSystemFactory(SHADERSHADOW_INTERFACE_VERSION_CSGO, NULL))
			{
				g_AfxStreams.OnShaderShadow((SOURCESDK::IShaderShadow_csgo *)iface);
			}
			else {
				ErrorBox("Could not get a supported ShaderShadow interface.");
			}
		}
		
		g_Hook_VClient_RenderView.Install(pGlobals);

		AfxV34HookWindow();

		PrintInfo();
	}
}

void* AppSystemFactory_ForClient(const char *pName, int *pReturnCode)
{
	if(!strcmp(VCLIENTENGINETOOLS_INTERFACE_VERSION_001, pName))
	{
		return &g_ClientEngineTools;
	}
	//else
	//if(isCsgo && !strcmp(VENGINE_RENDERVIEW_INTERFACE_VERSION_CSGO, pName) && g_AfxVRenderView)
	//{
	//	return g_AfxVRenderView;
	//}
	return g_AppSystemFactory(pName, pReturnCode);
}

void * old_Client_Init;

int __stdcall new_Client_Init(DWORD *this_ptr, SOURCESDK::CreateInterfaceFn appSystemFactory, SOURCESDK::CreateInterfaceFn physicsFactory, SOURCESDK::CGlobalVarsBase *pGlobals) {
	static bool bFirstCall = true;
	int myret;

	if(bFirstCall) {
		bFirstCall = false;

		MySetup(appSystemFactory, new WrpGlobalsOther(pGlobals));
	}

	__asm {
		MOV ecx, pGlobals
		PUSH ecx
		MOV ecx, physicsFactory
		PUSH ecx
		MOV ecx, AppSystemFactory_ForClient
		PUSH ecx

		MOV ecx, this_ptr		
		CALL old_Client_Init
		MOV	myret, eax
	}

	return myret;
}

__declspec(naked) void hook_Client_Init() {
	static unsigned char * tempMem[8];
	__asm {
		POP eax
		MOV tempMem[0], eax
		MOV tempMem[4], ecx

		PUSH ecx
		CALL new_Client_Init

		MOV ecx, tempMem[4]
		PUSH 0
		PUSH eax
		MOV eax, tempMem[0]
		MOV [esp+4], eax
		POP eax

		RET
	}
}


void * HookInterfaceFn(void * iface, int idx, void * fn)
{
	MdtMemBlockInfos mbis;
	void * ret = 0;

	if(iface)
	{
		void **padr =  *(void ***)iface;
		ret = *(padr+idx);
		MdtMemAccessBegin((padr+idx), sizeof(void *), &mbis);
		*padr = fn;
		MdtMemAccessEnd(&mbis);
	}

	return ret;
}

bool g_DebugEnabled = false;

bool g_csgo_FirstFrameAfterNetUpdateEnd = false;

class CAfxBaseClientDll
: public SOURCESDK::IBaseClientDLL_csgo
, public IAfxBaseClientDll
{
public:
	CAfxBaseClientDll(IBaseClientDLL_csgo * parent)
	: m_Parent(parent)
	, m_OnLevelShutdown(0)
	, m_OnView_Render(0)
	{
	}

	~CAfxBaseClientDll()
	{
	}

	//
	// IAfxBaseClientDll:

	virtual IBaseClientDLL_csgo * GetParent()
	{
		return m_Parent;
	}

	virtual void OnLevelShutdown_set(IAfxBaseClientDllLevelShutdown * value)
	{
		m_OnLevelShutdown = value;
	}

	virtual void OnView_Render_set(IAfxBaseClientDllView_Render * value)
	{
		m_OnView_Render = value;
	}

	//
	// IBaseClientDll_csgo:

	virtual int Connect(SOURCESDK::CreateInterfaceFn appSystemFactory, SOURCESDK::CGlobalVarsBase *pGlobals);
	virtual void Disconnect();
	virtual int Init(SOURCESDK::CreateInterfaceFn appSystemFactory, SOURCESDK::CGlobalVarsBase *pGlobals);
	virtual void PostInit();
	virtual void Shutdown(void);
	virtual void LevelInitPreEntity(char const* pMapName);
	virtual void LevelInitPostEntity();
	virtual void LevelShutdown(void);
	virtual void _UNKOWN_008(void);
	virtual void _UNKOWN_009(void);
	virtual void _UNKOWN_010(void);
	virtual void _UNKOWN_011(void);
	virtual void _UNKOWN_012(void);
	virtual void _UNKOWN_013(void);
	virtual void _UNKOWN_014(void);
	virtual void _UNKOWN_015(void);
	virtual void _UNKOWN_016(void);
	virtual void _UNKOWN_017(void);
	virtual void _UNKOWN_018(void);
	virtual void _UNKOWN_019(void);
	virtual void _UNKOWN_020(void);
	virtual void _UNKOWN_021(void);
	virtual void _UNKOWN_022(void);
	virtual void _UNKOWN_023(void);
	virtual void _UNKOWN_024(void);
	virtual void _UNKOWN_025(void);
	virtual void View_Render(SOURCESDK::vrect_t_csgo *rect);
	virtual void RenderView(const SOURCESDK::CViewSetup_csgo &view, int nClearFlags, int whatToDraw);
	virtual void _UNKOWN_028(void);
	virtual void _UNKOWN_029(void);
	virtual void _UNKOWN_030(void);
	virtual void _UNKOWN_031(void);
	virtual void _UNKOWN_032(void);
	virtual void _UNKOWN_033(void);
	virtual void _UNKOWN_034(void);
	virtual void _UNKOWN_035(void);
	virtual void FrameStageNotify(SOURCESDK::CSGO::ClientFrameStage_t curStage);
	virtual void _UNKOWN_037(void);
	virtual void _UNKOWN_038(void);
	virtual void _UNKOWN_039(void);
	virtual void _UNKOWN_040(void);
	virtual void _UNKOWN_041(void);
	virtual void _UNKOWN_042(void);
	virtual void _UNKOWN_043(void);
	virtual void _UNKOWN_044(void);
	virtual void _UNKOWN_045(void);
	virtual void _UNKOWN_046(void);
	virtual void _UNKOWN_047(void);
	virtual void _UNKOWN_048(void);
	virtual void _UNKOWN_049(void);
	virtual void _UNKOWN_050(void);
	virtual void _UNKOWN_051(void);
	virtual void _UNKOWN_052(void);
	virtual void _UNKOWN_053(void);
	virtual void _UNKOWN_054(void);
	virtual void _UNKOWN_055(void);
	virtual void _UNKOWN_056(void);
	virtual void _UNKOWN_057(void);
	virtual void _UNKOWN_058(void);
	virtual void _UNKOWN_059(void);
	virtual void _UNKOWN_060(void);
	virtual void _UNKOWN_061(void);
	virtual void _UNKOWN_062(void);
	virtual void _UNUSED_GetScreenWidth(void);
	virtual void _UNUSED_GetScreenHeight(void);
	virtual void WriteSaveGameScreenshotOfSize(const char *pFilename, int width, int height, bool bCreatePowerOf2Padded = false, bool bWriteVTF = false);
	virtual void _UNKOWN_066(void);
	virtual void _UNKOWN_067(void);
	virtual void _UNKOWN_068(void);
	virtual void _UNKOWN_069(void);
	virtual void _UNKOWN_070(void);
	virtual void _UNKOWN_071(void);
	virtual void _UNKOWN_072(void);
	virtual void _UNKOWN_073(void);
	virtual void _UNKOWN_074(void);
	virtual void _UNKOWN_075(void);
	virtual void _UNKOWN_076(void);
	virtual void _UNKOWN_077(void);
	virtual void _UNKOWN_078(void);
	virtual void _UNKOWN_079(void);
	virtual void _UNKOWN_080(void);
	virtual void _UNKOWN_081(void);
	virtual void _UNKOWN_082(void);
	virtual void _UNKOWN_083(void);
	virtual void _UNKOWN_084(void);
	virtual void _UNKOWN_085(void);
	virtual void _UNKOWN_086(void);
	virtual void _UNKOWN_087(void);
	virtual void _UNKOWN_088(void);
	virtual void _UNKOWN_089(void);
	virtual void _UNKOWN_090(void);
	virtual void _UNKOWN_091(void);
	virtual void _UNKOWN_092(void);
	virtual void _UNKOWN_093(void);
	virtual void _UNKOWN_094(void);
	virtual void _UNKOWN_095(void);
	virtual void _UNKOWN_096(void);
	virtual void _UNKOWN_097(void);
	virtual void _UNKOWN_098(void);
	virtual void _UNKOWN_099(void);
	virtual void _UNKOWN_100(void);
	virtual void _UNKOWN_101(void);
	virtual void _UNKOWN_102(void);
	virtual void _UNKOWN_103(void);
	virtual void _UNKOWN_104(void);
	virtual void _UNKOWN_105(void);
	virtual void _UNKOWN_106(void);
	virtual void _UNKOWN_107(void);
	virtual void _UNKOWN_108(void);
	virtual void _UNKOWN_109(void);
	virtual void _UNKOWN_110(void);

	// and a few more to be save from updates:

	virtual void _UNKOWN_111(void);
	virtual void _UNKOWN_112(void);
	virtual void _UNKOWN_113(void);
	virtual void _UNKOWN_114(void);
	virtual void _UNKOWN_115(void);
	virtual void _UNKOWN_116(void);
	virtual void _UNKOWN_117(void);
	virtual void _UNKOWN_118(void);
	virtual void _UNKOWN_119(void);
	virtual void _UNKOWN_120(void);
	virtual void _UNKOWN_121(void);
	virtual void _UNKOWN_122(void);
	virtual void _UNKOWN_123(void);
	virtual void _UNKOWN_124(void);
	virtual void _UNKOWN_125(void);
	virtual void _UNKOWN_126(void);
	virtual void _UNKOWN_127(void);
	virtual void _UNKOWN_128(void);
	virtual void _UNKOWN_129(void);
	virtual void _UNKOWN_130(void);

private:
	IBaseClientDLL_csgo * m_Parent;
	IAfxBaseClientDllLevelShutdown * m_OnLevelShutdown;
	IAfxBaseClientDllView_Render * m_OnView_Render;
};

CAfxBaseClientDll * g_AfxBaseClientDll = 0;

__declspec(naked) int CAfxBaseClientDll::Connect(SOURCESDK::CreateInterfaceFn appSystemFactory, SOURCESDK::CGlobalVarsBase *pGlobals)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 0) }

__declspec(naked) void CAfxBaseClientDll::Disconnect()
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 1) }

//__declspec(naked) 
int CAfxBaseClientDll::Init(SOURCESDK::CreateInterfaceFn appSystemFactory, SOURCESDK::CGlobalVarsBase *pGlobals)
{ // NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 2)

	static bool bFirstCall = true;

	if (bFirstCall)
	{
		bFirstCall = false;

		MySetup(appSystemFactory, new WrpGlobalsCsGo(pGlobals));
	}

	int result = m_Parent->Init(AppSystemFactory_ForClient, pGlobals);

	// Add file system search path for our assets:
	if (g_FileSystem_csgo)
	{
		std::string path(GetHlaeFolder());

		path.append("resources\\AfxHookSource\\assets\\csgo");

		g_FileSystem_csgo->AddSearchPath(path.c_str(), "GAME", SOURCESDK::PATH_ADD_TO_TAIL);
	}

#ifdef AFX_MIRV_PGL
	MirvPgl::Init();
#endif	

	CAfxBaseFxStream::AfxStreamsInit();

	return result;
}

__declspec(naked) void CAfxBaseClientDll::PostInit()
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 3) }

 //__declspec(naked) 
void CAfxBaseClientDll::Shutdown(void)
{ // NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 4)

	g_AfxStreams.ShutDown();

#ifdef AFX_MIRV_PGL
	MirvPgl::Shutdown();
#endif	

	m_Parent->Shutdown();
}

//__declspec(naked) 
void CAfxBaseClientDll::LevelInitPreEntity(char const* pMapName)
{ // NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 5)

#ifdef AFX_MIRV_PGL
	MirvPgl::SupplyLevelInit(pMapName);
#endif	

	m_Parent->LevelInitPreEntity(pMapName);
}

__declspec(naked) void CAfxBaseClientDll::LevelInitPostEntity()
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 6) }

//__declspec(naked) 
void CAfxBaseClientDll::LevelShutdown(void)
{ // NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 7)

	if (m_OnLevelShutdown) m_OnLevelShutdown->LevelShutdown(this);

#if AFX_SHADERS_CSGO
	csgo_Stdshader_dx9_Hooks_OnLevelShutdown();
#endif

	g_AfxShaders.ReleaseUnusedShaders();

#ifdef AFX_MIRV_PGL
	MirvPgl::SupplyLevelShutdown();
#endif

	m_Parent->LevelShutdown();
}

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_008(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 8) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_009(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 9) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_010(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 10) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_011(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 11) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_012(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 12) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_013(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 13) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_014(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 14) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_015(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 15) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_016(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 16) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_017(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 17) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_018(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 18) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_019(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 19) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_020(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 20) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_021(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 21) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_022(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 22) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_023(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 23) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_024(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 24) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_025(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 25) }

//__declspec(naked) 
void CAfxBaseClientDll::View_Render(SOURCESDK::vrect_t_csgo *rect)
{ // NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 26)

	//Tier0_Msg("---- View_Render ----\n");

	bool rectNull = rect->width == 0 || rect->height == 0;

	if (g_MaterialSystem_csgo && !rectNull)
	{

		if (m_OnView_Render)
		{
			m_OnView_Render->View_Render(this, rect);
		}
		else
		{
			m_Parent->View_Render(rect);
		}
	}
	else
	{
		m_Parent->View_Render(rect);
	}
}

__declspec(naked) void CAfxBaseClientDll::RenderView(const SOURCESDK::CViewSetup_csgo &view, int nClearFlags, int whatToDraw)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 27) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_028(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 28) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_029(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 29) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_030(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 30) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_031(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 31) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_032(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 32) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_033(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 33) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_034(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 34) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_035(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 35) }

//__declspec(naked)
void CAfxBaseClientDll::FrameStageNotify(SOURCESDK::CSGO::ClientFrameStage_t curStage)
{ // NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 36)

	static bool firstFrameAfterNetUpdateEnd = false;

	switch (curStage)
	{
	case SOURCESDK::CSGO::FRAME_START:
#ifdef AFX_MIRV_PGL
		MirvPgl::CheckStartedAndRestoreIfDown();
		MirvPgl::ExecuteQueuedCommands();
#endif
		break;
	case SOURCESDK::CSGO::FRAME_NET_UPDATE_START:
		break;
	case SOURCESDK::CSGO::FRAME_NET_UPDATE_END:
		firstFrameAfterNetUpdateEnd = true;
		break;
	case SOURCESDK::CSGO::FRAME_RENDER_START:
		g_csgo_FirstFrameAfterNetUpdateEnd = firstFrameAfterNetUpdateEnd;
		firstFrameAfterNetUpdateEnd = false;
		break;
	}

	m_Parent->FrameStageNotify(curStage);

	switch (curStage)
	{
	case SOURCESDK::CSGO::FRAME_RENDER_END:
		break;
	}
}

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_037(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 37) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_038(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 38) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_039(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 39) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_040(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 40) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_041(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 41) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_042(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 42) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_043(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 43) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_044(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 44) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_045(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 45) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_046(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 46) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_047(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 47) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_048(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 48) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_049(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 49) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_050(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 50) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_051(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 51) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_052(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 52) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_053(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 53) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_054(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 54) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_055(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 55) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_056(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 56) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_057(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 57) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_058(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 58) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_059(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 59) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_060(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 60) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_061(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 61) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_062(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 62) }

__declspec(naked) void CAfxBaseClientDll::_UNUSED_GetScreenWidth(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 63) }

__declspec(naked) void CAfxBaseClientDll::_UNUSED_GetScreenHeight(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 64) }

__declspec(naked) void CAfxBaseClientDll::WriteSaveGameScreenshotOfSize(const char *pFilename, int width, int height, bool bCreatePowerOf2Padded, bool bWriteVTF)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 65) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_066(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 66) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_067(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 67) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_068(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 68) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_069(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 69) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_070(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 70) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_071(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 71) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_072(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 72) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_073(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 73) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_074(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 74) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_075(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 75) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_076(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 76) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_077(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 77) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_078(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 78) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_079(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 79) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_080(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 80) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_081(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 81) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_082(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 82) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_083(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 83) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_084(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 84) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_085(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 85) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_086(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 86) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_087(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 87) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_088(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 88) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_089(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 89) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_090(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 90) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_091(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 91) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_092(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 92) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_093(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 93) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_094(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 94) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_095(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 95) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_096(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 96) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_097(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 97) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_098(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 98) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_099(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 99) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_100(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 100) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_101(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 101) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_102(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 102) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_103(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 103) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_104(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 104) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_105(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 105) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_106(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 106) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_107(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 107) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_108(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 108) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_109(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 109) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_110(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 110) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_111(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 111) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_112(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 112) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_113(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 113) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_114(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 114) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_115(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 115) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_116(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 116) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_117(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 117) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_118(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 118) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_119(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 119) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_120(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 120) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_121(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 121) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_122(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 122) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_123(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 123) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_124(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 124) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_125(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 125) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_126(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 126) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_127(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 127) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_128(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 128) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_129(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 129) }

__declspec(naked) void CAfxBaseClientDll::_UNKOWN_130(void)
{ NAKED_JMP_CLASSMEMBERIFACE_FN(CAfxBaseClientDll, m_Parent, 130) }

void HookClientDllInterface_011_Init(void * iface)
{
	old_Client_Init = HookInterfaceFn(iface, 0, (void *)hook_Client_Init);
}

SOURCESDK::IClientEntityList_csgo * SOURCESDK::g_Entitylist_csgo = 0;

SOURCESDK::CreateInterfaceFn old_Client_CreateInterface = 0;

void* new_Client_CreateInterface(const char *pName, int *pReturnCode)
{
	static bool bFirstCall = true;

	void * pRet = old_Client_CreateInterface(pName, pReturnCode);

	if(bFirstCall)
	{
		bFirstCall = false;

		void * iface = NULL;
		
		if(!isCsgo)
		{
			if (iface = old_Client_CreateInterface(CLIENT_DLL_INTERFACE_VERSION_018, NULL)) {
				g_Info_VClient = CLIENT_DLL_INTERFACE_VERSION_018;
				HookClientDllInterface_011_Init(iface);
			}
			else
			if(iface = old_Client_CreateInterface(CLIENT_DLL_INTERFACE_VERSION_017, NULL)) {
				g_Info_VClient = CLIENT_DLL_INTERFACE_VERSION_017;
				HookClientDllInterface_011_Init(iface);
			}
			else if(iface = old_Client_CreateInterface(CLIENT_DLL_INTERFACE_VERSION_016, NULL)) {
				g_Info_VClient = CLIENT_DLL_INTERFACE_VERSION_016;
				HookClientDllInterface_011_Init(iface);
			}
			else if(iface = old_Client_CreateInterface(CLIENT_DLL_INTERFACE_VERSION_015, NULL)) {
				g_Info_VClient = CLIENT_DLL_INTERFACE_VERSION_015;
				HookClientDllInterface_011_Init(iface);
			}
			else if(iface = old_Client_CreateInterface(CLIENT_DLL_INTERFACE_VERSION_013, NULL)) {
				g_Info_VClient = CLIENT_DLL_INTERFACE_VERSION_013;
				HookClientDllInterface_011_Init(iface);
			}
			else if(iface = old_Client_CreateInterface(CLIENT_DLL_INTERFACE_VERSION_012, NULL)) {
				g_Info_VClient = CLIENT_DLL_INTERFACE_VERSION_012;
				HookClientDllInterface_011_Init(iface);
			}
			else if(iface = old_Client_CreateInterface(CLIENT_DLL_INTERFACE_VERSION_011, NULL)) {
				g_Info_VClient = CLIENT_DLL_INTERFACE_VERSION_011;
				HookClientDllInterface_011_Init(iface);
			}
			else
			{
				ErrorBox("Could not get a supported VClient interface.");
			}
		}
		else
		{
			// isCsgo.

			SOURCESDK::g_Entitylist_csgo = (SOURCESDK::IClientEntityList_csgo *)old_Client_CreateInterface(VCLIENTENTITYLIST_INTERFACE_VERSION_CSGO, NULL);

			g_ClientTools.SetClientTools((SOURCESDK::CSGO::IClientTools *)old_Client_CreateInterface(SOURCESDK_CSGO_VCLIENTTOOLS_INTERFACE_VERSION, NULL));
		}
	}

	if(isCsgo)
	{
		if(!g_AfxBaseClientDll && !strcmp(pName, CLIENT_DLL_INTERFACE_VERSION_CSGO_018))
		{
			g_Info_VClient = CLIENT_DLL_INTERFACE_VERSION_CSGO_018 " (CS:GO)";
			g_AfxBaseClientDll = new CAfxBaseClientDll((SOURCESDK::IBaseClientDLL_csgo *)pRet);
			g_AfxStreams.OnAfxBaseClientDll(g_AfxBaseClientDll);

			pRet = g_AfxBaseClientDll;
		}
		/*
		else if (!g_AfxCsgoPrediction && !strcmp(SOURCESDK_CSGO_VCLIENT_PREDICTION_INTERFACE_VERSION, pName))
		{
			g_AfxCsgoPrediction = new CAfxCsgoPrediction((SOURCESDK::CSGO::IPrediction *)pRet);

			pRet = g_AfxCsgoPrediction;
		}
		*/
	}

	return pRet;
}


HMODULE g_H_ClientDll = 0;

FARPROC WINAPI new_Engine_GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	FARPROC nResult;
	nResult = GetProcAddress(hModule, lpProcName);

	if (!nResult)
		return nResult;

	if (HIWORD(lpProcName))
	{
		if (!lstrcmp(lpProcName, "GetProcAddress"))
			return (FARPROC) &new_Engine_GetProcAddress;

		if (
			hModule == g_H_ClientDll
			&& !lstrcmp(lpProcName, "CreateInterface")
		) {
			old_Client_CreateInterface = (SOURCESDK::CreateInterfaceFn)nResult;
			return (FARPROC) &new_Client_CreateInterface;
		}

	}

	return nResult;
}

WNDPROC g_NextWindProc;
static bool g_afxWindowProcSet = false;

LRESULT CALLBACK new_Afx_WindowProc(
	__in HWND hwnd,
	__in UINT uMsg,
	__in WPARAM wParam,
	__in LPARAM lParam
)
{
	switch(uMsg)
	{
	case WM_ACTIVATE:
		g_AfxHookSourceInput.Supply_Focus(LOWORD(wParam) != 0);
		break;
	case WM_CHAR:
		if(g_AfxHookSourceInput.Supply_CharEvent(wParam, lParam))
			return 0;
		break;
	case WM_KEYDOWN:
		if(g_AfxHookSourceInput.Supply_KeyEvent(AfxHookSourceInput::KS_DOWN, wParam, lParam))
			return 0;
		break;
	case WM_KEYUP:
		if(g_AfxHookSourceInput.Supply_KeyEvent(AfxHookSourceInput::KS_UP,wParam, lParam))
			return 0;
		break;
	case WM_INPUT:
		{
			HRAWINPUT hRawInput = (HRAWINPUT)lParam;
			RAWINPUT inp;
			UINT size = sizeof(inp);

			GetRawInputData(hRawInput, RID_INPUT, &inp, &size, sizeof(RAWINPUTHEADER));

			if(inp.header.dwType == RIM_TYPEMOUSE)
			{
				RAWMOUSE * rawmouse = &inp.data.mouse;
				LONG dX, dY;

				if((rawmouse->usFlags & 0x01) == MOUSE_MOVE_RELATIVE)
				{
					dX = rawmouse->lLastX;
					dY = rawmouse->lLastY;
				}
				else
				{
					static bool initial = true;
					static LONG lastX = 0;
					static LONG lastY = 0;

					if(initial)
					{
						initial = false;
						lastX = rawmouse->lLastX;
						lastY = rawmouse->lLastY;
					}

					dX = rawmouse->lLastX -lastX;
					dY = rawmouse->lLastY -lastY;

					lastX = rawmouse->lLastX;
					lastY = rawmouse->lLastY;
				}

				if(g_AfxHookSourceInput.Supply_RawMouseMotion(dX,dY))
					return 0;
			}
		}
		break;
	}

	return CallWindowProcW(g_NextWindProc, hwnd, uMsg, wParam, lParam);
}


// TODO: this is risky, actually we should track the hWnd maybe.
LONG WINAPI new_GetWindowLongW(
	__in HWND hWnd,
	__in int nIndex
)
{
	if(nIndex == GWL_WNDPROC)
	{
		if(g_afxWindowProcSet)
		{
			return (LONG)g_NextWindProc;
		}
	}

	return GetWindowLongW(hWnd, nIndex);
}

// TODO: this is risky, actually we should track the hWnd maybe.
LONG WINAPI new_SetWindowLongW(
	__in HWND hWnd,
	__in int nIndex,
	__in LONG dwNewLong
)
{
	if(nIndex == GWL_WNDPROC)
	{
		LONG lResult = SetWindowLongW(hWnd, nIndex, (LONG)new_Afx_WindowProc);

		if(!g_afxWindowProcSet)
		{
			g_afxWindowProcSet = true;
		}
		else
		{
			lResult = (LONG)g_NextWindProc;
		}

		g_NextWindProc = (WNDPROC)dwNewLong;

		return lResult;
	}

	return SetWindowLongW(hWnd, nIndex, dwNewLong);
}

BOOL WINAPI new_GetCursorPos(
	__out LPPOINT lpPoint
)
{
	BOOL result = GetCursorPos(lpPoint);

	g_AfxHookSourceInput.Supply_GetCursorPos(lpPoint);

	return result;
}

BOOL WINAPI new_SetCursorPos(
	__in int X,
	__in int Y
)
{
	g_AfxHookSourceInput.Supply_SetCursorPos(X,Y);

	return SetCursorPos(X,Y);
}

void AfxV34HookWindow(void)
{
	if(isV34)
	{
		HWND hWnd = FindWindowW(L"Valve001",NULL);

		if(!hWnd)
			ErrorBox("AfxV34HookWindow: FindWindowW failed.");
		else
			g_NextWindProc = (WNDPROC)SetWindowLongW(hWnd,GWL_WNDPROC,(LONG)new_Afx_WindowProc);
	}
}

FARPROC WINAPI new_shaderapidx9_GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
	FARPROC nResult;
	nResult = GetProcAddress(hModule, lpProcName);

	if (!nResult)
		return nResult; // This can happen on Windows XP for Direct3DCreateEx9

	if (HIWORD(lpProcName))
	{
		if (
			!lstrcmp(lpProcName, "Direct3DCreate9Ex")
		) {
			old_Direct3DCreate9Ex = (Direct3DCreate9Ex_t)nResult;
			return (FARPROC) &new_Direct3DCreate9Ex;
		}
	}

	return nResult;
}

HMODULE WINAPI new_LoadLibraryA(LPCSTR lpLibFileName);
HMODULE WINAPI new_LoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);

extern HMODULE g_H_EngineDll;

void CommonHooks()
{
	static bool bFirstRun = true;
	static bool bFirstTier0 = true;

	// do not use messageboxes here, there is some friggin hooking going on in between by the
	// Source engine.

	if (bFirstRun)
	{
		bFirstRun = false;

		// detect if we are csgo:

		char filePath[MAX_PATH] = { 0 };
		GetModuleFileName(0, filePath, MAX_PATH);

		if (StringEndsWith(filePath, "csgo.exe"))
		{
			isCsgo = true;
		}

		if (wcsstr(GetCommandLineW(), L"-afxV34"))
		{
			isV34 = true;
		}

		//ScriptEngine_StartUp();
	}

	if (bFirstTier0)
	{
		HMODULE hTier0;
		if (hTier0 = GetModuleHandleA("tier0.dll"))
		{
			bFirstTier0 = false;

			Tier0_Msg = (Tier0MsgFn)GetProcAddress(hTier0, "Msg");
			Tier0_DMsg = (Tier0DMsgFn)GetProcAddress(hTier0, "DMsg");
			Tier0_Warning = (Tier0MsgFn)GetProcAddress(hTier0, "Warning");
			Tier0_DWarning = (Tier0DMsgFn)GetProcAddress(hTier0, "DWarning");
			Tier0_Log = (Tier0MsgFn)GetProcAddress(hTier0, "Log");
			Tier0_DLog = (Tier0DMsgFn)GetProcAddress(hTier0, "DLog");
			Tier0_Error = (Tier0MsgFn)GetProcAddress(hTier0, "Error");
			Tier0_ConMsg = (Tier0MsgFn)GetProcAddress(hTier0, "ConMsg");
			Tier0_ConWarning = (Tier0MsgFn)GetProcAddress(hTier0, "ConWarning");
			Tier0_ConLog = (Tier0MsgFn)GetProcAddress(hTier0, "ConLog");

			if (isV34)
			{
				InterceptDllCall(hTier0, "USER32.dll", "GetCursorPos", (DWORD)&new_GetCursorPos);
				InterceptDllCall(hTier0, "USER32.dll", "SetCursorPos", (DWORD)&new_SetCursorPos);
			}

			//if (!Hook_csgo_MemAlloc())
			//{
			//	ErrorBox("Error: Hook_csgo_MemAlloc failed. This can cause mayor bugs!");
			//}
		}
	}
}

void LibraryHooksA(HMODULE hModule, LPCSTR lpLibFileName)
{
	static bool bFirstClient = true;
	static bool bFirstEngine = true;
	static bool bFirstInputsystem = true;
	//static bool bFirstGameOverlayRenderer = true;
	static bool bFirstfilesystem_stdio = true;
	static bool bFirstShaderapidx9 = true;
	static bool bFirstMaterialsystem = true;
	static bool bFirstScaleformui = true;
	static bool bFirstStdshader_dx9 = true;

	CommonHooks();

	if(!hModule || !lpLibFileName)
		return;

#if 0
	static FILE *f1=NULL;

	if( !f1 ) f1=fopen("hlae_log_LibraryHooksA.txt","wb");
	fprintf(f1,"%s\n", lpLibFileName);
	fflush(f1);
#endif

	if(bFirstfilesystem_stdio && StringEndsWith( lpLibFileName, "filesystem_steam.dll")) // v34
	{
		bFirstfilesystem_stdio = false;
		
		InterceptDllCall(hModule, "Kernel32.dll", "LoadLibraryExA", (DWORD) &new_LoadLibraryExA);
		InterceptDllCall(hModule, "Kernel32.dll", "LoadLibraryA", (DWORD) &new_LoadLibraryA);
	}
	else
	if(bFirstfilesystem_stdio && StringEndsWith( lpLibFileName, "filesystem_stdio.dll"))
	{
		bFirstfilesystem_stdio = false;
		
		InterceptDllCall(hModule, "Kernel32.dll", "LoadLibraryExA", (DWORD) &new_LoadLibraryExA);
		InterceptDllCall(hModule, "Kernel32.dll", "LoadLibraryA", (DWORD) &new_LoadLibraryA);
	}
	else
	if(bFirstEngine && StringEndsWith( lpLibFileName, "engine.dll"))
	{
		bFirstEngine = false;

		g_H_EngineDll = hModule;

		Addresses_InitEngineDll((AfxAddr)hModule, isCsgo);

		InterceptDllCall(hModule, "Kernel32.dll", "GetProcAddress", (DWORD) &new_Engine_GetProcAddress);
		InterceptDllCall(hModule, "Kernel32.dll", "LoadLibraryExA", (DWORD) &new_LoadLibraryExA);
		InterceptDllCall(hModule, "Kernel32.dll", "LoadLibraryA", (DWORD) &new_LoadLibraryA);

		// actually this is not required, since engine.dll calls first and thus is lower in the chain:
		InterceptDllCall(hModule, "USER32.dll", "GetWindowLongW", (DWORD) &new_GetWindowLongW);
		InterceptDllCall(hModule, "USER32.dll", "SetWindowLongW", (DWORD) &new_SetWindowLongW);

		if(isV34)
		{
			InterceptDllCall(hModule, "USER32.dll", "GetCursorPos", (DWORD) &new_GetCursorPos);
			InterceptDllCall(hModule, "USER32.dll", "SetCursorPos", (DWORD) &new_SetCursorPos);
		}

		// Init the hook early, so we don't run into issues with threading:
		Hook_csgo_SndMixTimeScalePatch();
	}
	else
	if(bFirstInputsystem && StringEndsWith( lpLibFileName, "inputsystem.dll"))
	{
		bFirstInputsystem = false;

		InterceptDllCall(hModule, "USER32.dll", "GetWindowLongW", (DWORD) &new_GetWindowLongW);
		InterceptDllCall(hModule, "USER32.dll", "SetWindowLongW", (DWORD) &new_SetWindowLongW);

		InterceptDllCall(hModule, "USER32.dll", "GetCursorPos", (DWORD) &new_GetCursorPos);
		InterceptDllCall(hModule, "USER32.dll", "SetCursorPos", (DWORD) &new_SetCursorPos);
	}
	else
	if(bFirstMaterialsystem && StringEndsWith( lpLibFileName, "materialsystem.dll"))
	{
		bFirstMaterialsystem = false;
		
		InterceptDllCall(hModule, "Kernel32.dll", "LoadLibraryExA", (DWORD) &new_LoadLibraryExA);
		InterceptDllCall(hModule, "Kernel32.dll", "LoadLibraryA", (DWORD) &new_LoadLibraryA);
	}
	else
	if(bFirstShaderapidx9 && StringEndsWith( lpLibFileName, "shaderapidx9.dll"))
	{
		bFirstShaderapidx9 = false;

		InterceptDllCall(hModule, "kernel32.dll", "GetProcAddress", (DWORD) &new_shaderapidx9_GetProcAddress);

		old_Direct3DCreate9 = (Direct3DCreate9_t)InterceptDllCall(hModule, "d3d9.dll", "Direct3DCreate9", (DWORD) &new_Direct3DCreate9);
	}
	else
	if(bFirstClient && StringEndsWith( lpLibFileName, "client.dll"))
	{
		bFirstClient = false;

		g_H_ClientDll = hModule;

		Addresses_InitClientDll((AfxAddr)g_H_ClientDll, isCsgo);

		//
		// Install early hooks:

		csgo_CSkyBoxView_Draw_Install();
		csgo_CViewRender_Install();
		Hook_csgo_writeWaveConsoleCheck();
		Hook_C_BaseEntity_ToolRecordEnties();
		Hook_csgo_PlayerAnimStateFix();
	}
	else
	if(bFirstScaleformui && StringEndsWith( lpLibFileName, "scaleformui.dll"))
	{
		bFirstScaleformui = false;

		Addresses_InitScaleformuiDll((AfxAddr)hModule, isCsgo);

		//
		// Install early hooks:

		csgo_ScaleFormDll_Hooks_Init();
	}
	else
	if(bFirstStdshader_dx9 && StringEndsWith( lpLibFileName, "stdshader_dx9.dll"))
	{
		bFirstStdshader_dx9 = false;

		//Addresses_InitStdshader_dx9Dll((AfxAddr)hModule, isCsgo);

		//
		// Install early hooks:

		//csgo_Stdshader_dx9_Hooks_Init();
	}
}

void LibraryHooksW(HMODULE hModule, LPCWSTR lpLibFileName)
{
	static bool bFirstLauncher = true;

	CommonHooks();

	if (!hModule || !lpLibFileName)
		return;

#if 0
	static FILE *f1 = NULL;

	if (!f1) f1 = fopen("hlae_log_LibraryHooksW.txt", "wb");
	fwprintf(f1, L"%s\n", lpLibFileName);
	fflush(f1);
#endif

	if (bFirstLauncher && StringEndsWithW(lpLibFileName, L"launcher.dll"))
	{
		bFirstLauncher = false;

		InterceptDllCall(hModule, "Kernel32.dll", "LoadLibraryExA", (DWORD)&new_LoadLibraryExA);
		InterceptDllCall(hModule, "Kernel32.dll", "LoadLibraryA", (DWORD)&new_LoadLibraryA);
	}
}


// i.e. called by Counter-Strike Source
HMODULE WINAPI new_LoadLibraryA( LPCSTR lpLibFileName ) {
	HMODULE hRet = LoadLibraryA(lpLibFileName);

	LibraryHooksA(hRet, lpLibFileName);

	return hRet;
}


// i.e. called by Portal First Slice
HMODULE WINAPI new_LoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
	HMODULE hRet = LoadLibraryExA(lpLibFileName, hFile, dwFlags);

	LibraryHooksA(hRet, lpLibFileName);

	return hRet;
}

// i.e. called by CS:GO since 5/4/2017:
HMODULE WINAPI new_LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
	HMODULE hRet = LoadLibraryExW(lpLibFileName, hFile, dwFlags);

	LibraryHooksW(hRet, lpLibFileName);

	return hRet;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	switch (fdwReason) 
	{ 
		case DLL_PROCESS_ATTACH:
		{
#if 1
			MessageBox(0,"DLL_PROCESS_ATTACH","MDT_DEBUG",MB_OK);
#endif

			bool bLoadLibraryExA = 0 != InterceptDllCall(GetModuleHandle(NULL), "Kernel32.dll", "LoadLibraryExA", (DWORD)&new_LoadLibraryExA);
			bool bLoadLibraryA = 0 != InterceptDllCall(GetModuleHandle(NULL), "Kernel32.dll", "LoadLibraryA", (DWORD)&new_LoadLibraryA);
			bool bLoadLibraryExW = 0 != InterceptDllCall(GetModuleHandle(NULL), "Kernel32.dll", "LoadLibraryExW", (DWORD)&new_LoadLibraryExW);

			if (!(bLoadLibraryExA || bLoadLibraryA || bLoadLibraryExW))
				ErrorBox();

			//
			// Remember we are not on the main program thread here,
			// instead we are on our own thread, so don't run
			// things here that would have problems with that.
			//

			break;
		}
		case DLL_PROCESS_DETACH:
		{
			// actually this gets called now.

			MatRenderContextHook_Shutdown();

			if(g_AfxBaseClientDll) { delete g_AfxBaseClientDll; g_AfxBaseClientDll = 0; }

#ifdef _DEBUG
			_CrtDumpMemoryLeaks();
#endif

			break;
		}
		case DLL_THREAD_ATTACH:
		{
			break;
		}
		case DLL_THREAD_DETACH:
		{
			break;
		}
	}
	return TRUE;
}
