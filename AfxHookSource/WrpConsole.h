#pragma once

// Copyright (c) advancedfx.org
//
// Last changes:
// 2016-02-16 dominik.matrixstorm.com
//
// First changes:
// 2009-10-02 dominik.matrixstorm.com

// Description:
// Wrapper(s) for Source engine ConCommands and ConVars.

#include "SourceInterfaces.h"

#include <string>


// IWrpCommandArgs /////////////////////////////////////////////////////////////

class IWrpCommandArgs abstract
{
public:
	/// <summary> returns the count of passed arguments </summary>
	virtual int ArgC() abstract = 0;

	/// <summary> returns the i-th argument, where 0 is being the first one </summary>
	virtual char const * ArgV(int i) abstract = 0;
};
typedef void (*WrpCommandCallback)(IWrpCommandArgs * args);

// CSubWrpCommandArgs //////////////////////////////////////////////////////////

class CSubWrpCommandArgs
: public IWrpCommandArgs
{
public:
	CSubWrpCommandArgs(IWrpCommandArgs * commandArgs, int offset);

	/// <summary> returns the count of passed arguments </summary>
	virtual int ArgC();

	/// <summary> returns the i-th argument, where 0 is being the first one </summary>
	virtual char const * ArgV(int i);

private:
	int m_Offset;
	IWrpCommandArgs *m_CommandArgs;
	std::string m_Temp;
};

// WrpConCommand ///////////////////////////////////////////////////////////////

class WrpConCommand
{
public:
	WrpConCommand(char const * name, WrpCommandCallback callback, char const * helpString = 0);
	virtual ~WrpConCommand();

	WrpCommandCallback GetCallback();
	char const * GetHelpString();
	char const * GetName();

private:
	WrpCommandCallback m_Callback;
	char * m_HelpString;
	char * m_Name;
};


// WrpConCommandsRegistrar_003 /////////////////////////////////////////////////

class WrpConCommandsRegistrar_003 :
	public IConCommandBaseAccessor_003
{
public:
	virtual bool RegisterConCommandBase(ConCommandBase_003 *pVar);
};

// WrpConCommandsRegistrar_004 /////////////////////////////////////////////////

class WrpConCommandsRegistrar_004 :
	public IConCommandBaseAccessor_004
{
public:
	virtual bool RegisterConCommandBase(ConCommandBase_004 *pVar);
};

// WrpConCommandsRegistrar_007 /////////////////////////////////////////////////

class WrpConCommandsRegistrar_007 :
	public IConCommandBaseAccessor_007
{
public:
	virtual bool RegisterConCommandBase(ConCommandBase_007 *pVar);
};


// WrpConCommands //////////////////////////////////////////////////////////////

struct WrpConCommandsListEntry {
	WrpConCommand * Command;
	WrpConCommandsListEntry * Next;
};

class WrpConCommands
{
public:
	/// <remarks> only valid when Registered with ICvar_003 </remarks>
	static IVEngineClient_012 * GetVEngineClient_012();
	
	/// <remarks> only valid when Registered with ICvar_007 </remarks>
	static ICvar_007 * GetVEngineCvar007();

	static void RegisterCommands(ICvar_003 * cvarIface, IVEngineClient_012 * vEngineClientInterface);
	static void RegisterCommands(ICvar_004 * cvarIface);
	static void RegisterCommands(ICvar_007 * cvarIface);

	static void WrpConCommand_Register(WrpConCommand * cmd);
	static void WrpConCommand_Unregister(WrpConCommand * cmd);

	static bool WrpConCommandsRegistrar_003_Register(ConCommandBase_003 *pVar);
	static bool WrpConCommandsRegistrar_004_Register(ConCommandBase_004 *pVar);
	static bool WrpConCommandsRegistrar_007_Register(ConCommandBase_007 *pVar);

private:
	static ICvar_003 * m_CvarIface_003;
	static ICvar_004 * m_CvarIface_004;
	static ICvar_007 * m_CvarIface_007;
	static WrpConCommandsListEntry * m_CommandListRoot;
	static IVEngineClient_012 * m_VEngineClient_012;
};


#define CON_COMMAND( name, description ) \
   static void name(IWrpCommandArgs * args); \
   static WrpConCommand name##_command( #name, name, description ); \
   static void name(IWrpCommandArgs * args)

#define CON_COMMAND_F( name, description, flags ) \
   static void name(IWrpCommandArgs * args); \
   static WrpConCommand name##_command( #name, name, description, flags ); \
   static void name(IWrpCommandArgs * args)

////////////////////////////////////////////////////////////////////////////////

/// <remaks>Currently only supports CS:GO!</remarks>
class WrpConVarRef
{
public:
	WrpConVarRef(char const * pName);

	float GetFloat(void) const;
	int GetInt(void) const;

	void SetValue(float value);

	/// <remarks>This guarantees the int and float m_Value members to be set immediatelly. However Changecallback and string versions might still be triggered later.</remarks>
	void SetValueFastHack(float value);

	/// <summary>This sets the values directly, without calling callbacks, this might have unwanted side-effects!</summary>
	void SetDirectHack(float value);

private:
	ConVar_007 * m_pConVar007;

};
