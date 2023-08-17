
#include "pch.h"
#include "AppLook.h"
#include "resource.h"

#include <afxvisualmanager.h>
#include <afxvisualmanagerofficexp.h>
#include <afxvisualmanagerwindows.h>
#include <afxvisualmanageroffice2003.h>
#include <afxvisualmanagervs2005.h>
#include <afxvisualmanagervs2008.h>
#include <afxvisualmanageroffice2007.h>
#include <afxvisualmanagerwindows7.h>
#include <afxdockingmanager.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_Settings[] = _T("Settings");
	static const TCHAR entry_AppLook[] = _T("AppLook");
}


CAppLook::CAppLook( app::AppLook appLook )
	: CCmdTarget()
	, m_appLook( static_cast<app::AppLook>( AfxGetApp()->GetProfileInt( reg::section_Settings, reg::entry_AppLook, appLook ) ) )
{
	SetAppLook( m_appLook );
}

CAppLook::~CAppLook()
{
}

void CAppLook::Save( void )
{
	AfxGetApp()->WriteProfileInt( reg::section_Settings, reg::entry_AppLook, m_appLook );
}

void CAppLook::SetAppLook( app::AppLook appLook )
{
	m_appLook = appLook;

	switch ( m_appLook )
	{
		case app::Windows_2000:
			CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManager ) );
			break;
		case app::Office_XP:
			CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerOfficeXP ) );
			break;
		case app::Windows_XP:
			CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
			CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerWindows ) );
			break;
		case app::Office_2003:
			CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerOffice2003 ) );
			CDockingManager::SetDockingMode(DT_SMART);
			break;
		case app::VS_2005:
			CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerVS2005 ) );
			CDockingManager::SetDockingMode(DT_SMART);
			break;
		case app::VS_2008:
			CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerVS2008 ) );
			CDockingManager::SetDockingMode( DT_SMART );
			break;
		case app::Windows_7:
			CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerWindows7 ) );
			CDockingManager::SetDockingMode( DT_SMART );
			break;
		default:
			switch ( m_appLook )
			{
				case app::Office_2007_Blue:
					CMFCVisualManagerOffice2007::SetStyle( CMFCVisualManagerOffice2007::Office2007_LunaBlue );
					break;
				case app::Office_2007_Black:
					CMFCVisualManagerOffice2007::SetStyle( CMFCVisualManagerOffice2007::Office2007_ObsidianBlack );
					break;
				case app::Office_2007_Silver:
					CMFCVisualManagerOffice2007::SetStyle( CMFCVisualManagerOffice2007::Office2007_Silver );
					break;
				case app::Office_2007_Aqua:
					CMFCVisualManagerOffice2007::SetStyle( CMFCVisualManagerOffice2007::Office2007_Aqua );
					break;
			}

			CMFCVisualManager::SetDefaultManager( RUNTIME_CLASS( CMFCVisualManagerOffice2007 ) );
			CDockingManager::SetDockingMode( DT_SMART );
	}

	if ( AfxGetMainWnd()->GetSafeHwnd() )
		AfxGetMainWnd()->RedrawWindow( nullptr, nullptr, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE );
}

app::AppLook CAppLook::FromId( UINT cmdId )
{
	switch ( cmdId )
	{
		case ID_VIEW_APPLOOK_WINDOWS_2000:			return app::Windows_2000;
		case ID_VIEW_APPLOOK_OFFICE_XP:				return app::Office_XP;
		case ID_VIEW_APPLOOK_WINDOWS_XP:			return app::Windows_XP;
		case ID_VIEW_APPLOOK_OFFICE_2003:			return app::Office_2003;
		case ID_VIEW_APPLOOK_VS_2005:				return app::VS_2005;
		case ID_VIEW_APPLOOK_VS_2008:				return app::VS_2008;
		case ID_VIEW_APPLOOK_WINDOWS_7:				return app::Windows_7;
		default: ASSERT( false );
		case ID_VIEW_APPLOOK_OFFICE_2007_BLUE:		return app::Office_2007_Blue;
		case ID_VIEW_APPLOOK_OFFICE_2007_BLACK:		return app::Office_2007_Black;
		case ID_VIEW_APPLOOK_OFFICE_2007_SILVER:	return app::Office_2007_Silver;
		case ID_VIEW_APPLOOK_OFFICE_2007_AQUA:		return app::Office_2007_Aqua;
	}
}


// command handlers

BEGIN_MESSAGE_MAP( CAppLook, CCmdTarget )
	ON_COMMAND_RANGE( ID_VIEW_APPLOOK_WINDOWS_2000, ID_VIEW_APPLOOK_WINDOWS_7, OnApplicationLook )
	ON_UPDATE_COMMAND_UI_RANGE( ID_VIEW_APPLOOK_WINDOWS_2000, ID_VIEW_APPLOOK_WINDOWS_7, OnUpdateApplicationLook )
END_MESSAGE_MAP()

void CAppLook::OnApplicationLook( UINT cmdId )
{
	CWaitCursor wait;

	SetAppLook( FromId( cmdId ) );
	Save();
}

void CAppLook::OnUpdateApplicationLook( CCmdUI* pCmdUI )
{
	pCmdUI->SetRadio( m_appLook == FromId( pCmdUI->m_nID ) );
}
