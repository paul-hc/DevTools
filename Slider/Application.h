#ifndef Slider_h
#define Slider_h
#pragma once

#include "utl/UI/BaseApp.h"
#include "Application_fwd.h"


class CMainFrame;
class CThumbnailer;
class CScopedGdiPlusInit;
class CScopedPumpMessage;


namespace app
{
	class CCommandLineInfo;

	enum ForceFlags
	{
		FullScreen			= BIT_FLAG( 0 ),
		DocMaximize			= BIT_FLAG( 1 ),
		ShowToolbar			= BIT_FLAG( 2 ),
		ShowStatusBar		= BIT_FLAG( 3 ),

		// handled by CApplication
		RegAdditionalExt	= BIT_FLAG( 8 )
	};
}


class CApplication : public CBaseApp< CWinApp >
{
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

	void UpdateAllViews( UpdateViewHint hint = Hint_ViewUpdate, CDocument* pSenderDoc = NULL, CView* pSenderView = NULL );
private:
	enum RunFlags
	{
		ShowHelp			= BIT_FLAG( 0 ),
		RunTests			= BIT_FLAG( 8 ),
		SkipUiTests			= BIT_FLAG( 9 )
	};

	bool HandleAppTests( void );
private:
	class CCmdLineInfo : public CCommandLineInfo
	{
	public:
		CCmdLineInfo( CApplication* pApp ) : m_pApp( pApp ) { ASSERT_PTR( m_pApp ); }

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
	std::auto_ptr< CScopedGdiPlusInit > m_pGdiPlusInit;
	std::auto_ptr< CThumbnailer > m_pThumbnailer;
	std::auto_ptr< CLogger > m_pEventLogger;
	CAccelTable m_sharedAccel;

	CMainFrame* m_pMainFrame;
private:
	int m_runFlags;
	int m_forceMask;
	int m_forceFlags;
	std::vector< std::tstring > m_queuedAlbumFilePaths;
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

	bool MoveFiles( const std::vector< std::tstring >& filesToMove, CWnd* pParentWnd = AfxGetMainWnd() );
	bool DeleteFiles( const std::vector< std::tstring >& filesToDelete, bool allowUndo = true );

	fs::CPath GetActiveDirPath( void );


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


	// shared app progress bar support

	class CScopedProgress : private utl::noncopyable
	{
	public:
		enum AutoClearDelay
		{
			ACD_Immediate		= 0,
			ACD_DefaultDelay	= 500,
			ACD_NoClear			= INT_MAX
		};

		CScopedProgress( int autoClearDelay = 250 );
		CScopedProgress( int valueMin, int count, int stepCount, const TCHAR* pCaption = NULL, int autoClearDelay = 250 );
		~CScopedProgress();

		bool IsActive( void ) const;

		void Begin( int valueMin, int count, int stepCount, const TCHAR* pCaption = NULL );
		void End( int clearDelay = ACD_NoClear );	// ACD_NoClear here it means use m_autoClearDelay value!

		void SetStep( int step ) { ASSERT( IsActive() ); m_pSharedProgressBar->SetStep( step ); }
		void StepIt( void );

		void SetStepDivider( UINT pbStepDivider ) { m_pbStepDivider = pbStepDivider; }

		int GetPos( void ) const { ASSERT( IsActive() ); return m_pSharedProgressBar->GetPos(); }
		void SetPos( int value );
		void OffsetPos( int by ) { ASSERT( IsActive() ); m_pSharedProgressBar->OffsetPos( by ); }

		void GotoBegin( void );
		void GotoEnd( void );
	private:
		CProgressCtrl* m_pSharedProgressBar;
		int m_autoClearDelay;			// remove delay for the progress bar in mili-secs, 0 for immediate or ACD_NoClear for no reset
		UINT m_pbStepIndex;
		UINT m_pbStepDivider;
		std::auto_ptr< CScopedPumpMessage > m_pMessagePump;
	};

} //namespace app


#endif // Slider_h
