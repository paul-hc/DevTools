#ifndef Application_h
#define Application_h
#pragma once

#include "utl/Logger.h"
#include "utl/UI/BaseApp.h"
#include "Application_fwd.h"


class CAppCmdService;
class CCommandModel;
class CSystemTray;
class CTrayIcon;
namespace svc { interface ICommandService; }


class CApplication : public CBaseApp<CWinApp>
{
public:
	CApplication( void );
	virtual ~CApplication();

	svc::ICommandService* GetCommandService( void ) const;
	const CCommandModel* GetCommandModel( void ) const;
	CTrayIcon* GetMessageTrayIcon( void );				// lazy initialization

	// advanced debugging:
	bool UseInputSourcePaths( void ) const { return !m_inputSourcePaths.empty(); }
	const std::vector<fs::CPath>& GetInputSourcePaths( void ) const { return m_inputSourcePaths; }
	bool CheckReplaceSourcePaths( std::vector<fs::CPath>& rOutSourcePaths, const TCHAR* pClientTag );

	using CBaseApp<CWinApp>::IsInitAppResources;		// make it public for debugging
private:
	bool LoadInputFileListPaths( void );
private:
	std::auto_ptr<CAppCmdService> m_pCmdSvc;
	std::auto_ptr<CSystemTray> m_pSystemTray;			// system-tray shared popup window: hidden by default, used to display balloon notifications

	// advanced debugging:
	fs::CPath m_inputFileListPath;						// stored manually in registry
	std::vector<fs::CPath> m_inputSourcePaths;			// forces this DLL to use filepaths from the input file list (replacing the HDROP selection source from Explorer)

	// generated stuff
public:
    virtual BOOL InitInstance( void ) override;
    virtual int ExitInstance( void ) override;
	virtual void OnInitAppResources( void ) override;
protected:
	DECLARE_MESSAGE_MAP()
};


struct CScopedMainWnd
{
	CScopedMainWnd( HWND hWnd );
	~CScopedMainWnd();

	bool InEffect( void ) const { return m_inEffect; }

	static bool HasValidParentOwner( void ) { return ::IsWindow( s_pParentOwner->GetSafeHwnd() ) != FALSE; }
	static CWnd* GetParentOwnerWnd( void ) { return s_pParentOwner != nullptr ? s_pParentOwner : AfxGetMainWnd(); }		// during DLL registration there is no parent owner
private:
	CWnd* m_pOldMainWnd;
	bool m_inEffect;
	static CWnd* s_pParentOwner;
};


namespace app
{
	CApplication* GetApp( void );
	inline CWnd* GetMainWnd( void ) { return CScopedMainWnd::GetParentOwnerWnd(); }
}


#endif // Application_h
