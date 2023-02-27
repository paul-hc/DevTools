#ifndef BaseApp_h
#define BaseApp_h
#pragma once

#ifndef __AFXWIN_H__
#error "include 'stdafx.h' before including this file for PCH"
#endif


#include "utl/AppTools.h"
#include "AccelTable.h"
#include "ResourcePool.h"


// UTL global trace categories
//DECLARE_AFX_TRACE_CATEGORY( traceThumbs )


template< typename BaseClass = CWinApp >		// could use CWinAppEx base for new MFC app support (ribbons, etc)
class CBaseApp
	: public BaseClass
	, public CAppTools
{
protected:
	CBaseApp( const TCHAR* pAppName = nullptr );
	virtual ~CBaseApp();

	// call just before InitInstance:
	void StoreAppNameSuffix( const std::tstring& appNameSuffix ) { m_appNameSuffix = appNameSuffix; }
	void StoreProfileSuffix( const std::tstring& profileSuffix ) { m_profileSuffix = profileSuffix; }

	bool IsInitAppResources( void ) const { return m_pSharedResources.get() != nullptr; }
	void SetLazyInitAppResources( void ) { m_lazyInitAppResources = true; }			// for extension DLLs: prevent heavy resource initialization when the dll gets registered by regsvr32.exe
public:
	const std::tstring& GetAppNameSuffix( void ) const { return m_appNameSuffix; }

	// IAppTools interface
	virtual bool IsConsoleApp( void ) const;
	virtual bool IsInteractive( void ) const;
	virtual CLogger& GetLogger( void );
	virtual utl::CResourcePool& GetSharedResources( void );
	virtual CImageStore* GetSharedImageStore( void );
	virtual bool LazyInitAppResources( void );
	virtual bool BeepSignal( app::MsgType msgType = app::Info );
	virtual bool ReportError( const std::tstring& message, app::MsgType msgType = app::Error );
	virtual int ReportException( const std::exception& exc );
	virtual int ReportException( const CException* pExc );

	void StoreCmdShow( int cmdShow )
	{
		this->m_nCmdShow = cmdShow;
	}

	void SetInteractive( bool isInteractive = true )
	{
		m_isInteractive = isInteractive;
	}

	void RunUnitTests( void ) { OnRunUnitTests(); }
protected:
	static const TCHAR* AssignStringCopy( const TCHAR*& rpAppString, const std::tstring& value )
	{
		free( (void*)rpAppString );
		return rpAppString = _tcsdup( value.c_str() );
	}
private:
	std::auto_ptr<utl::CResourcePool> m_pSharedResources;		// application shared resources, released on ExitInstance()
	std::auto_ptr<CLogger> m_pLogger;
	std::auto_ptr<CImageStore> m_pSharedImageStore;				// managed lifetime of shared resources (lazy initialized)
	CAccelTable m_appAccel;
	std::tstring m_appNameSuffix, m_profileSuffix;				// could be set to "_v2" when required
	bool m_isInteractive;										// true by default; for apps with a message loop it starts false until application becomes idle for the first time
	bool m_lazyInitAppResources;								// true for extension DLLs: prevent heavy resource initialization when the dll gets registered by regsvr32.exe
protected:
	std::tstring m_appRegistryKeyName;							// by default "Paul Cocoveanu"
protected:
	// overridables
	virtual void OnInitAppResources( void );

	// generated stuff
public:
	virtual BOOL InitInstance( void );
	virtual int ExitInstance( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	virtual BOOL OnIdle( LONG count );
protected:
	afx_msg void OnAppAbout( void );
	afx_msg void OnUpdateAppAbout( CCmdUI* pCmdUI );
	afx_msg void OnRunUnitTests( void );
	afx_msg void OnUpdateRunUnitTests( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


namespace app
{
	void InitUtlBase( void );
	void TrackUnitTestMenu( CWnd* pTargetWnd, const CPoint& screenPos );
	UINT ToMsgBoxFlags( app::MsgType msgType );

	void TraceOsVersion( void );

#ifdef USE_UT
	void RunAllTests( void );
#endif
}


class CEnumTags;


namespace win
{
	enum OsVersion { NotAvailable, Win2K, WinXP, WinVista, Win7, Win8, Win10, WinBeyond };

	const CEnumTags& GetTags_OsVersion( void );

	OsVersion GetOsVersion( void );
	inline bool IsVersionOrGreater( OsVersion refOsVersion ) { return GetOsVersion() >= refOsVersion; }
}


#endif // BaseApp_h
