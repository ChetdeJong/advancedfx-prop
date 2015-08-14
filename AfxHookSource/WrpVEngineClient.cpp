#include "stdafx.h"

// Copyright (c) advancedfx.org
//
// Last changes:
// 2015-08-14 dominik.matrixstorm.com
//
// First changes:
// 2009-10-01 dominik.matrixstorm.com

// Description:
// Wrapper(s) for Source's IVEngineClient Interface

#include "WrpVEngineClient.h"

#include <stdarg.h>

// WrpVEngineClient_012 ///////////////////////////////////////////////////////////

WrpVEngineClient_012::WrpVEngineClient_012(IVEngineClient_012 * iface) {
	m_VEngineClient_012 = iface;
}

void WrpVEngineClient_012::GetScreenSize( int& width, int& height ) {
	m_VEngineClient_012->GetScreenSize(width, height);
}

void WrpVEngineClient_012::ServerCmd( const char *szCmdString, bool bReliable) {
	m_VEngineClient_012->ServerCmd(szCmdString, bReliable);
}

void WrpVEngineClient_012::ClientCmd( const char *szCmdString ) {
	m_VEngineClient_012->ClientCmd(szCmdString);
}

bool WrpVEngineClient_012::Con_IsVisible( void ) {
	return m_VEngineClient_012->Con_IsVisible();
}

void WrpVEngineClient_012::GetViewAngles( QAngle& va ) {
	m_VEngineClient_012->GetViewAngles(va);
}

void WrpVEngineClient_012::SetViewAngles( QAngle& va ) {
	m_VEngineClient_012->SetViewAngles(va);
}

int WrpVEngineClient_012::GetMaxClients( void ) {
	return m_VEngineClient_012->GetMaxClients();
}

bool WrpVEngineClient_012::IsInGame( void ) {
	return m_VEngineClient_012->IsInGame();
}

bool WrpVEngineClient_012::IsConnected( void ) {
	return m_VEngineClient_012->IsConnected();
}

bool WrpVEngineClient_012::IsDrawingLoadingImage( void ) {
	return m_VEngineClient_012->IsDrawingLoadingImage();
}

void WrpVEngineClient_012::Con_NPrintf( int pos, const char *fmt, ... ) {
	va_list argptr;
	va_start(argptr, fmt);
	m_VEngineClient_012->Con_NPrintf(pos, fmt, argptr);
}

const char *  WrpVEngineClient_012::GetGameDirectory( void ) {
	return m_VEngineClient_012->GetGameDirectory();
}

const VMatrix& WrpVEngineClient_012::WorldToScreenMatrix()
{
	return m_VEngineClient_012->WorldToScreenMatrix();
}

const VMatrix& WrpVEngineClient_012::WorldToViewMatrix()
{
	return m_VEngineClient_012->WorldToViewMatrix();
}

char const *  WrpVEngineClient_012::GetLevelName( void ) {
	return m_VEngineClient_012->GetLevelName();
}

void WrpVEngineClient_012::EngineStats_BeginFrame( void ) {
	return m_VEngineClient_012->EngineStats_BeginFrame();
}

void WrpVEngineClient_012::EngineStats_EndFrame( void ) {
	return m_VEngineClient_012->EngineStats_EndFrame();
}

bool WrpVEngineClient_012::IsPlayingDemo( void ) {
	return m_VEngineClient_012->IsPlayingDemo();
}

bool WrpVEngineClient_012::IsRecordingDemo( void ) {
	return m_VEngineClient_012->IsRecordingDemo();
}

bool WrpVEngineClient_012::IsPlayingTimeDemo( void ) {
	return m_VEngineClient_012->IsPlayingTimeDemo();
}

bool WrpVEngineClient_012::IsPaused( void ) {
	return m_VEngineClient_012->IsPaused();
}

bool WrpVEngineClient_012::IsTakingScreenshot( void ) {
	return m_VEngineClient_012->IsTakingScreenshot();
}
bool WrpVEngineClient_012::IsHLTV( void ) {
	return m_VEngineClient_012->IsHLTV();
}

bool WrpVEngineClient_012::IsLevelMainMenuBackground( void ) {
	return m_VEngineClient_012->IsLevelMainMenuBackground();
}

void WrpVEngineClient_012::GetMainMenuBackgroundName( char *dest, int destlen ) {
	return m_VEngineClient_012->GetMainMenuBackgroundName(dest, destlen);
}

bool WrpVEngineClient_012::IsInEditMode( void ) {
	return m_VEngineClient_012->IsInEditMode();
}

float WrpVEngineClient_012::GetScreenAspectRatio() {
	return m_VEngineClient_012->GetScreenAspectRatio();
}

unsigned int WrpVEngineClient_012::GetEngineBuildNumber() {
	return m_VEngineClient_012->GetEngineBuildNumber();
}

const char * WrpVEngineClient_012::GetProductVersionString() {
	return m_VEngineClient_012->GetProductVersionString();
}

bool WrpVEngineClient_012::IsHammerRunning( ) const {
	return m_VEngineClient_012->IsHammerRunning();
}

void WrpVEngineClient_012::ExecuteClientCmd( const char *szCmdString ) {
	m_VEngineClient_012->ExecuteClientCmd(szCmdString);
}

void WrpVEngineClient_012::ClientCmd_Unrestricted( const char *szCmdString )
{
	// In 012 this is unrestricted:
	ClientCmd(szCmdString);
}

WrpVEngineClientDemoInfoEx * WrpVEngineClient_012::GetDemoInfoEx(void)
{
	return 0;
}

// WrpVEngineClient_013 ///////////////////////////////////////////////////////////

WrpVEngineClient_013::WrpVEngineClient_013(IVEngineClient_013 * iface) {
	m_VEngineClient_013 = iface;
}

void WrpVEngineClient_013::GetScreenSize( int& width, int& height ) {
	m_VEngineClient_013->GetScreenSize(width, height);
}

void WrpVEngineClient_013::ServerCmd( const char *szCmdString, bool bReliable) {
	m_VEngineClient_013->ServerCmd(szCmdString, bReliable);
}

void WrpVEngineClient_013::ClientCmd( const char *szCmdString ) {
	m_VEngineClient_013->ClientCmd(szCmdString);
}

bool WrpVEngineClient_013::Con_IsVisible( void ) {
	return m_VEngineClient_013->Con_IsVisible();
}

void WrpVEngineClient_013::GetViewAngles( QAngle& va ) {
	m_VEngineClient_013->GetViewAngles(va);
}

void WrpVEngineClient_013::SetViewAngles( QAngle& va ) {
	m_VEngineClient_013->SetViewAngles(va);
}

int WrpVEngineClient_013::GetMaxClients( void ) {
	return m_VEngineClient_013->GetMaxClients();
}

bool WrpVEngineClient_013::IsInGame( void ) {
	return m_VEngineClient_013->IsInGame();
}

bool WrpVEngineClient_013::IsConnected( void ) {
	return m_VEngineClient_013->IsConnected();
}

bool WrpVEngineClient_013::IsDrawingLoadingImage( void ) {
	return m_VEngineClient_013->IsDrawingLoadingImage();
}

void WrpVEngineClient_013::Con_NPrintf( int pos, const char *fmt, ... ) {
	va_list argptr;
	va_start(argptr, fmt);
	m_VEngineClient_013->Con_NPrintf(pos, fmt, argptr);
}

const char *  WrpVEngineClient_013::GetGameDirectory( void ) {
	return m_VEngineClient_013->GetGameDirectory();
}

const VMatrix& WrpVEngineClient_013::WorldToScreenMatrix()
{
	return m_VEngineClient_013->WorldToScreenMatrix();
}

const VMatrix& WrpVEngineClient_013::WorldToViewMatrix()
{
	return m_VEngineClient_013->WorldToViewMatrix();
}


char const *  WrpVEngineClient_013::GetLevelName( void ) {
	return m_VEngineClient_013->GetLevelName();
}

void WrpVEngineClient_013::EngineStats_BeginFrame( void ) {
	return m_VEngineClient_013->EngineStats_BeginFrame();
}

void WrpVEngineClient_013::EngineStats_EndFrame( void ) {
	return m_VEngineClient_013->EngineStats_EndFrame();
}

bool WrpVEngineClient_013::IsPlayingDemo( void ) {
	return m_VEngineClient_013->IsPlayingDemo();
}

bool WrpVEngineClient_013::IsRecordingDemo( void ) {
	return m_VEngineClient_013->IsRecordingDemo();
}

bool WrpVEngineClient_013::IsPlayingTimeDemo( void ) {
	return m_VEngineClient_013->IsPlayingTimeDemo();
}

int WrpVEngineClient_013::GetDemoRecordingTick( void )
{
	return m_VEngineClient_013->GetDemoRecordingTick();
}

int	WrpVEngineClient_013::GetDemoPlaybackTick( void )
{
	return m_VEngineClient_013->GetDemoPlaybackTick();
}

int	WrpVEngineClient_013::GetDemoPlaybackStartTick( void )
{
	return m_VEngineClient_013->GetDemoPlaybackStartTick();
}

float WrpVEngineClient_013::GetDemoPlaybackTimeScale( void )
{
	return m_VEngineClient_013->GetDemoPlaybackTimeScale();
}

int WrpVEngineClient_013::GetDemoPlaybackTotalTicks( void )
{
	return m_VEngineClient_013->GetDemoPlaybackTotalTicks();
}

bool WrpVEngineClient_013::IsPaused( void ) {
	return m_VEngineClient_013->IsPaused();
}

bool WrpVEngineClient_013::IsTakingScreenshot( void ) {
	return m_VEngineClient_013->IsTakingScreenshot();
}
bool WrpVEngineClient_013::IsHLTV( void ) {
	return m_VEngineClient_013->IsHLTV();
}

bool WrpVEngineClient_013::IsLevelMainMenuBackground( void ) {
	return m_VEngineClient_013->IsLevelMainMenuBackground();
}

void WrpVEngineClient_013::GetMainMenuBackgroundName( char *dest, int destlen ) {
	return m_VEngineClient_013->GetMainMenuBackgroundName(dest, destlen);
}

bool WrpVEngineClient_013::IsInEditMode( void ) {
	return m_VEngineClient_013->IsInEditMode();
}

float WrpVEngineClient_013::GetScreenAspectRatio() {
	return m_VEngineClient_013->GetScreenAspectRatio();
}

unsigned int WrpVEngineClient_013::GetEngineBuildNumber() {
	return m_VEngineClient_013->GetEngineBuildNumber();
}

const char * WrpVEngineClient_013::GetProductVersionString() {
	return m_VEngineClient_013->GetProductVersionString();
}

bool WrpVEngineClient_013::IsHammerRunning( ) const {
	return m_VEngineClient_013->IsHammerRunning();
}

void WrpVEngineClient_013::ExecuteClientCmd( const char *szCmdString ) {
	m_VEngineClient_013->ExecuteClientCmd(szCmdString);
}

void WrpVEngineClient_013::ClientCmd_Unrestricted( const char *szCmdString ) {
	m_VEngineClient_013->ClientCmd_Unrestricted(szCmdString);
}

WrpVEngineClientDemoInfoEx * WrpVEngineClient_013::GetDemoInfoEx(void)
{
	return this;
}

// WrpVEngineClient_013_csgo ///////////////////////////////////////////////////////////

WrpVEngineClient_013_csgo::WrpVEngineClient_013_csgo(IVEngineClient_013_csgo * iface) {
	m_VEngineClient_013 = iface;
}

void WrpVEngineClient_013_csgo::GetScreenSize( int& width, int& height ) {
	m_VEngineClient_013->GetScreenSize(width, height);
}

void WrpVEngineClient_013_csgo::ServerCmd( const char *szCmdString, bool bReliable) {
	m_VEngineClient_013->ServerCmd(szCmdString, bReliable);
}

void WrpVEngineClient_013_csgo::ClientCmd( const char *szCmdString ) {
	m_VEngineClient_013->ClientCmd(szCmdString);
}

bool WrpVEngineClient_013_csgo::Con_IsVisible( void ) {
	return m_VEngineClient_013->Con_IsVisible();
}

void WrpVEngineClient_013_csgo::GetViewAngles( QAngle& va ) {
	m_VEngineClient_013->GetViewAngles(va);
}

void WrpVEngineClient_013_csgo::SetViewAngles( QAngle& va ) {
	m_VEngineClient_013->SetViewAngles(va);
}

int WrpVEngineClient_013_csgo::GetMaxClients( void ) {
	return m_VEngineClient_013->GetMaxClients();
}

bool WrpVEngineClient_013_csgo::IsInGame( void ) {
	return m_VEngineClient_013->IsInGame();
}

bool WrpVEngineClient_013_csgo::IsConnected( void ) {
	return m_VEngineClient_013->IsConnected();
}

bool WrpVEngineClient_013_csgo::IsDrawingLoadingImage( void ) {
	return m_VEngineClient_013->IsDrawingLoadingImage();
}

void WrpVEngineClient_013_csgo::Con_NPrintf( int pos, const char *fmt, ... ) {
	va_list argptr;
	va_start(argptr, fmt);
	m_VEngineClient_013->Con_NPrintf(pos, fmt, argptr);
}

const char *  WrpVEngineClient_013_csgo::GetGameDirectory( void ) {
	return m_VEngineClient_013->GetGameDirectory();
}

const VMatrix& WrpVEngineClient_013_csgo::WorldToScreenMatrix()
{
	return m_VEngineClient_013->WorldToScreenMatrix();
}

const VMatrix& WrpVEngineClient_013_csgo::WorldToViewMatrix()
{
	return m_VEngineClient_013->WorldToViewMatrix();
}


char const *  WrpVEngineClient_013_csgo::GetLevelName( void ) {
	return m_VEngineClient_013->GetLevelName();
}

void WrpVEngineClient_013_csgo::EngineStats_BeginFrame( void ) {
	return m_VEngineClient_013->EngineStats_BeginFrame();
}

void WrpVEngineClient_013_csgo::EngineStats_EndFrame( void ) {
	return m_VEngineClient_013->EngineStats_EndFrame();
}

bool WrpVEngineClient_013_csgo::IsPlayingDemo( void ) {
	return m_VEngineClient_013->IsPlayingDemo();
}

bool WrpVEngineClient_013_csgo::IsRecordingDemo( void ) {
	return m_VEngineClient_013->IsRecordingDemo();
}

bool WrpVEngineClient_013_csgo::IsPlayingTimeDemo( void ) {
	return m_VEngineClient_013->IsPlayingTimeDemo();
}

int WrpVEngineClient_013_csgo::GetDemoRecordingTick( void )
{
	return m_VEngineClient_013->GetDemoRecordingTick();
}

int	WrpVEngineClient_013_csgo::GetDemoPlaybackTick( void )
{
	return m_VEngineClient_013->GetDemoPlaybackTick();
}

int	WrpVEngineClient_013_csgo::GetDemoPlaybackStartTick( void )
{
	return m_VEngineClient_013->GetDemoPlaybackStartTick();
}

float WrpVEngineClient_013_csgo::GetDemoPlaybackTimeScale( void )
{
	return m_VEngineClient_013->GetDemoPlaybackTimeScale();
}

int WrpVEngineClient_013_csgo::GetDemoPlaybackTotalTicks( void )
{
	return m_VEngineClient_013->GetDemoPlaybackTotalTicks();
}

bool WrpVEngineClient_013_csgo::IsPaused( void ) {
	return m_VEngineClient_013->IsPaused();
}

bool WrpVEngineClient_013_csgo::IsTakingScreenshot( void ) {
	return m_VEngineClient_013->IsTakingScreenshot();
}
bool WrpVEngineClient_013_csgo::IsHLTV( void ) {
	return m_VEngineClient_013->IsHLTV();
}

bool WrpVEngineClient_013_csgo::IsLevelMainMenuBackground( void ) {
	return m_VEngineClient_013->IsLevelMainMenuBackground();
}

void WrpVEngineClient_013_csgo::GetMainMenuBackgroundName( char *dest, int destlen ) {
	return m_VEngineClient_013->GetMainMenuBackgroundName(dest, destlen);
}

bool WrpVEngineClient_013_csgo::IsInEditMode( void ) {
	return m_VEngineClient_013->IsInEditMode();
}

float WrpVEngineClient_013_csgo::GetScreenAspectRatio() {
	return m_VEngineClient_013->GetScreenAspectRatio();
}

unsigned int WrpVEngineClient_013_csgo::GetEngineBuildNumber() {
	return m_VEngineClient_013->GetEngineBuildNumber();
}

const char * WrpVEngineClient_013_csgo::GetProductVersionString() {
	return m_VEngineClient_013->GetProductVersionString();
}

bool WrpVEngineClient_013_csgo::IsHammerRunning( ) const {
	return m_VEngineClient_013->IsHammerRunning();
}

void WrpVEngineClient_013_csgo::ExecuteClientCmd( const char *szCmdString ) {
	m_VEngineClient_013->ExecuteClientCmd(szCmdString);
}

void WrpVEngineClient_013_csgo::ClientCmd_Unrestricted( const char *szCmdString ) {
	m_VEngineClient_013->ClientCmd_Unrestricted(szCmdString);
}

WrpVEngineClientDemoInfoEx * WrpVEngineClient_013_csgo::GetDemoInfoEx(void)
{
	return this;
}
