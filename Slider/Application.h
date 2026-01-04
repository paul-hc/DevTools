#ifndef Slider_h
#define Slider_h
#pragma once

#include "Application_fwd.h"
#include "utl/UI/BaseApp.h"
#include <afxwinappex.h>


class CMainFrame;
class CThumbnailer;
class CScopedGdiPlusInit;
class CScopedPumpMessage;


namespace app
{
	enum ForceFlags
	{
		FullScreen			= BIT_FLAG( 0 ),
		DocMaximize			= BIT_FLAG( 1 ),
		ShowToolbar			= BIT_FLAG( 2 ),
		ShowStatusBar		= BIT_FLAG( 3 ),

		// handled by CApplication
		RegImgAdditionalExt	= BIT_FLAG( 8 )
	};
}


class CApplication : public CBaseApp<CWinAppEx>
{
	typedef CBaseApp<CWinAppEx> TBaseClass;
public:
	CApplication( void );
	virtual ~CApplication();

	CMainFrame* GetMainFrame( void ) const { return safe_ptr( m_pMainFrame ); }
	CLogger& GetEventLogger( void ) const { ASSERT_PTR( m_pEventLogger.get() ); return *m_pEventLogger; }
	CThumbnailer* GetThumbnailer( void ) const { return safe_ptr( m_pThumbnailer.get() ); }

	bool HasForceMask( int maskFlag ) const { return HasFlag( m_forceMask, maskFlag ); }
	bool HasForceFlag( int forceFlag ) const { ASSERT( HasForceMask( forceFlag ) ); return HasFlag( m_forceFlags, forceFlag ); }

	// operations
	bool OpenQueuedAlbum( void );

	void SetStatusBarMessage( const TCHAR* pMessage );

	void UpdateAllViews( UpdateViewHint hint = Hint_ViewUpdate, CDocument* pSenderDoc = nullptr, CView* pSenderView = nullptr );
private:
	void InitGlobals( void );

	enum RunFlags
	{
		ShowHelp	= BIT_FLAG( 0 ),
		RunTests	= BIT_FLAG( 8 ),
		SkipUiTests	= BIT_FLAG( 9 )
	};

	bool HandleAppTests( void );
private:
	class CCmdLineInfo : public CCommandLineInfo
	{
	public:
		CCmdLineInfo( CApplication* pApp );

		void SetForceFlag( int forceFlag, bool on );
		void ParseAppSwitches( void );

		// base overrides
		virtual void ParseParam( const TCHAR* pParam, BOOL isFlag, BOOL isLast );

		static std::tstring GetHelpMsg( void );
	private:
		bool ParseSwitch( const TCHAR* pSwitch );
	private:
		CApplication* m_pApp;
	};

	friend class CCmdLineInfo;
private:
	std::auto_ptr<CScopedGdiPlusInit> m_pGdiPlusInit;
	std::auto_ptr<CThumbnailer> m_pThumbnailer;
	std::auto_ptr<CLogger> m_pEventLogger;
	CAccelTable m_sharedAccel;

	CMainFrame* m_pMainFrame;
private:
	int m_runFlags;
	int m_forceMask;
	int m_forceFlags;
	std::vector<fs::CPath> m_queuedAlbumFilePaths;
	static std::tstring s_cmdLineHelpMsg;

	// generated stuff
public:
	virtual BOOL InitInstance( void );
	virtual int ExitInstance( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	virtual BOOL OnDDECommand( LPTSTR pCommand );
private:
	afx_msg void OnFileOpenAlbumFolder( void );
	afx_msg void OnClearTempEmbeddedClones( void );

	DECLARE_MESSAGE_MAP()
};


extern CApplication theApp;


namespace app
{
	inline CApplication* GetApp( void ) { return &theApp; }
	inline CMainFrame* GetMainFrame( void ) { return theApp.GetMainFrame(); }
	inline CThumbnailer* GetThumbnailer( void ) { return theApp.GetThumbnailer(); }

	ui::CIssueStore& GetIssueStore( void );

	fs::TDirPath GetActiveDirPath( void );


	class CInteractiveMode : public ui::CInteractiveMode
	{
	public:
		static ui::IUserReport& Instance( void );

		virtual int ReportError( CException* pExc, UINT mbType = MB_OK );
	};


	class CScopedUserReport
	{
	public:
		CScopedUserReport( ui::IUserReport& rReport ) : m_pOldUserReport( s_pUserReport ) { ASSERT_PTR( m_pOldUserReport ); s_pUserReport = &rReport; }
		~CScopedUserReport() { s_pUserReport = m_pOldUserReport; ASSERT_PTR( s_pUserReport ); }

		static ui::IUserReport& GetReport( void ) { ASSERT_PTR( s_pUserReport ); return *s_pUserReport; }
	private:
		ui::IUserReport* m_pOldUserReport;
		static ui::IUserReport* s_pUserReport;
	};

} //namespace app


#endif // Slider_h
