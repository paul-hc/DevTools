#ifndef BaseApp_h
#define BaseApp_h
#pragma once

#ifndef __AFXWIN_H__
#error "include 'stdafx.h' before including this file for PCH"
#endif


#include "utl/AppTools.h"
#include "AccelTable.h"
#include "ResourcePool.h"


class CImageStore;


// UTL global trace categories
//DECLARE_AFX_TRACE_CATEGORY( traceThumbs )


template< typename BaseClass = CWinApp >		// could use CWinAppEx base for new MFC app support (ribbons, etc)
class CBaseApp : public BaseClass, public CAppTools
{
protected:
	CBaseApp( const TCHAR* pAppName = NULL );
	virtual ~CBaseApp();

	// call just before InitInstance:
	void StoreAppNameSuffix( const std::tstring& appNameSuffix ) { m_appNameSuffix = appNameSuffix; }
	void StoreProfileSuffix( const std::tstring& profileSuffix ) { m_profileSuffix = profileSuffix; }
public:
	const std::tstring& GetAppNameSuffix( void ) const { return m_appNameSuffix; }

	// IAppTools interface
	virtual utl::CResourcePool& GetSharedResources( void );
	virtual CLogger& GetLogger( void );
	virtual bool BeepSignal( app::MsgType msgType = app::Info );
	virtual bool ReportError( const std::tstring& message, app::MsgType msgType = app::Error );
	virtual int ReportException( const std::exception& exc );
	virtual int ReportException( const CException* pExc );

	void RunUnitTests( void ) { OnRunUnitTests(); }
protected:
	static const TCHAR* AssignStringCopy( const TCHAR*& rpAppString, const std::tstring& value )
	{
		free( (void*)rpAppString );
		return rpAppString = _tcsdup( value.c_str() );
	}
private:
	std::auto_ptr< utl::CResourcePool > m_pSharedResources;		// application shared resources, released on ExitInstance()
	std::auto_ptr< CLogger > m_pLogger;
	std::auto_ptr< CImageStore > m_pImageStore;					// control the lifetime of shared resources
	CAccelTable m_appAccel;
	std::tstring m_appNameSuffix, m_profileSuffix;				// could be set to "_v2" when required
protected:
	std::tstring m_appRegistryKeyName;							// by default "Paul Cocoveanu"
protected:
	// generated overrides
	public:
	virtual BOOL InitInstance( void );
	virtual int ExitInstance( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	// generated message map
	afx_msg void OnAppAbout( void );
	afx_msg void OnUpdateAppAbout( CCmdUI* pCmdUI );
	afx_msg void OnRunUnitTests( void );
	afx_msg void OnUpdateRunUnitTests( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


namespace app
{
	void TrackUnitTestMenu( CWnd* pTargetWnd, const CPoint& screenPos );
	UINT ToMsgBoxFlags( app::MsgType msgType );

	void TraceOsVersion( void );
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
