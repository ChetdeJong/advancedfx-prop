#ifndef _HLAE_MAINWINDOW_H_
	#define _HLAE_MAINWINDOW_H_

	#include <wx/app.h>
	#include <wx/frame.h>
	#include <wx/menu.h>
	#include <wx/dialog.h>

	// this is not very nice, but we currently lack a better solution:
	//class hlaeMainWindow;
	//class CHlaeBcServer;

	#include "basecomServer.h"

class CAboutDialog : public wxDialog 
{
	public:
		CAboutDialog(wxWindow* parent);
	
};

class hlaeMainWindow : public wxFrame {
private:
	DECLARE_EVENT_TABLE()
	enum {
		hlaeID_SaveLayout = wxID_HIGHEST+1,
		hlaeID_LayoutManager,
		hlaeID_DemoTools,
		hlaeID_Launch
	};
	CHlaeGameWindow* m_HlaeGameWindow;
	wxMenu* m_windowmenu;
	wxMenu*	m_toolbarmenu;
	wxMenu*	m_layoutmenu;
	void CreateMenuBar();
	void OnExit(wxCommandEvent& evt);
	void OnLaunch(wxCommandEvent& evt);
	void OnDemoTools(wxCommandEvent& evt);
	void OnSaveLayout(wxCommandEvent& evt);
	void OnLayoutManager(wxCommandEvent& evt);
	void OnAbout(wxCommandEvent& evt);
	void OnSize(wxSizeEvent& evt);
public:
	hlaeMainWindow();
	~hlaeMainWindow();
	wxMenu* hlaeMainWindow::GetWindowMenu() const;
	wxMenu* hlaeMainWindow::GetToolBarMenuMenu() const;
	wxMenu* hlaeMainWindow::GetLayoutMenu() const;
};

class hlaeApp : public wxApp {
public:
	bool OnInit();
};

DECLARE_APP(hlaeApp)

#endif // _HLAE_MAINWINDOW_H_