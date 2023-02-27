
#include "stdafx.h"
#include "WindowPlacement.h"
#include "SystemTray.h"
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR entry_showCmd[] = _T("FrameCmdShow");
	static const TCHAR entry_restoreToMax[] = _T("FrameRestoreToMax");
	static const TCHAR entry_wndPos[] = _T("FrameWndPos");
	static const TCHAR entry_wndSize[] = _T("FrameWndSize");

	static const TCHAR format_posSize[] = _T("(%d, %d)");
}


CWindowPlacement::CWindowPlacement( void )
{
	Reset();
}

void CWindowPlacement::Reset( void )
{
	memset( (tagWINDOWPLACEMENT*)this, 0, sizeof( tagWINDOWPLACEMENT ) );
	this->length = sizeof( WINDOWPLACEMENT );
}

bool CWindowPlacement::IsEmpty( void ) const
{
	return
		0 == ptMinPosition.x && 0 == ptMinPosition.y &&
		0 == ptMaxPosition.x && 0 == ptMaxPosition.y &&
		0 == rcNormalPosition.left && 0 == rcNormalPosition.top && 0 == rcNormalPosition.right && 0 == rcNormalPosition.bottom;
}

void CWindowPlacement::Setup( const CWnd* pWnd, const CRect& normalRect, UINT _showCmd, UINT _flags /*= 0*/ )
{
	this->flags = _flags;
	this->showCmd = _showCmd;
	this->ptMinPosition = CPoint( 0, 0 );
	this->ptMaxPosition = CPoint( -1, -1 );
	this->rcNormalPosition = normalRect;

	EnsureVisibleNormalPosition( pWnd );
}

bool CWindowPlacement::EnsureVisibleNormalPosition( const CWnd* pWnd )
{
	if ( nullptr == pWnd->GetSafeHwnd() || ui::IsTopLevel( pWnd->m_hWnd ) )					// if pWnd is NULL assume top-level window
		return ui::EnsureVisibleDesktopRect( RefNormalPosition() );							// clamp to visible monitor work-area
	else
		return ui::EnsureVisibleWindowRect( RefNormalPosition(), pWnd->GetSafeHwnd() );		// clamp to parent's rect
}

bool CWindowPlacement::ReadWnd( const CWnd* pWnd )
{
	ASSERT_PTR( pWnd );
	if ( !pWnd->GetWindowPlacement( this ) )
		return false;

	if ( CSystemTray::IsMinimizedToTray( pWnd ) )
		showCmd = SW_SHOWMINIMIZED;

	return true;
}

bool CWindowPlacement::CommitWnd( CWnd* pWnd, bool restoreToMax /*= false*/, bool setMinPos /*= false*/ )
{
	ASSERT_PTR( pWnd->GetSafeHwnd() );

	if ( restoreToMax )
		this->flags |= WPF_RESTORETOMAXIMIZED;

	if ( setMinPos )
		this->flags |= WPF_SETMINPOSITION;

	EnsureVisibleNormalPosition( pWnd );

	return pWnd->SetWindowPlacement( this ) != FALSE;
}

void CWindowPlacement::QueryCreateStruct( CREATESTRUCT* pCreateStruct ) const
{
	ASSERT_PTR( pCreateStruct );

	CSize size = ui::GetRectSize( rcNormalPosition );

	pCreateStruct->cx = size.cx;
	pCreateStruct->cy = size.cy;
	pCreateStruct->x = rcNormalPosition.left;
	pCreateStruct->y = rcNormalPosition.top;
}

int CWindowPlacement::ChangeMaximizedShowCmd( UINT _showCmd )
{
	if ( SW_SHOWMAXIMIZED == this->showCmd )
	{	// actually hide now and further show as maximized
		this->showCmd = _showCmd;
		return SW_SHOWMAXIMIZED;				// delayed show command (2nd step)
	}
	return this->showCmd;
}

void CWindowPlacement::RegSave( const TCHAR regSection[] ) const
{
	REQUIRE( !str::IsEmpty( regSection ) );

	CRect windowRect = rcNormalPosition;		// workspace coordinates
	const CPoint& pos = windowRect.TopLeft();
	CSize size = windowRect.Size();

	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileInt( regSection, reg::entry_showCmd, this->showCmd );
	pApp->WriteProfileInt( regSection, reg::entry_restoreToMax, HasFlag( this->flags, WPF_RESTORETOMAXIMIZED ) );
	pApp->WriteProfileString( regSection, reg::entry_wndPos, str::Format( reg::format_posSize, pos.x, pos.y ).c_str() );
	pApp->WriteProfileString( regSection, reg::entry_wndSize, str::Format( reg::format_posSize, size.cx, size.cy ).c_str() );
}

bool CWindowPlacement::RegLoad( const TCHAR regSection[], const CWnd* pWnd )
{
	REQUIRE( !str::IsEmpty( regSection ) );

	CPoint pos( -1, -1 );
	CSize size( -1, -1 );

	CWinApp* pApp = AfxGetApp();
	std::tstring spec;

	spec = pApp->GetProfileString( regSection, reg::entry_wndPos ).GetString();
	if ( _stscanf( spec.c_str(), reg::format_posSize, &pos.x, &pos.y ) != 2 )
		return false;

	spec = pApp->GetProfileString( regSection, reg::entry_wndSize ).GetString();
	if ( _stscanf( spec.c_str(), reg::format_posSize, &size.cx, &size.cy ) != 2 )
		return false;

	CRect normalRect( pos, size );

	Setup( pWnd, normalRect, pApp->GetProfileInt( regSection, reg::entry_showCmd, SW_SHOWNORMAL ) );
	SetFlag( this->flags, WPF_RESTORETOMAXIMIZED, pApp->GetProfileInt( regSection, reg::entry_restoreToMax, FALSE ) != FALSE );

	if ( pWnd != nullptr && AfxGetMainWnd() == pWnd )
		if ( pApp->m_nCmdShow != SW_SHOWNORMAL )
			this->showCmd = pApp->m_nCmdShow;				// override with the command line option
		else
			pApp->m_nCmdShow = this->showCmd;				// use the saved state

	return true;
}

void CWindowPlacement::Stream( CArchive& archive )
{
	if ( archive.IsStoring() )
		archive.Write( (tagWINDOWPLACEMENT*)this, sizeof( WINDOWPLACEMENT ) );
	else
		archive.Read( (tagWINDOWPLACEMENT*)this, sizeof( WINDOWPLACEMENT ) );
}
