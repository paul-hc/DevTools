
#include "stdafx.h"
#include "WindowPlacement.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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

bool CWindowPlacement::ReadWnd( const CWnd* pWnd )
{
	ASSERT_PTR( pWnd );
	return pWnd->GetWindowPlacement( this ) != FALSE;
}

bool CWindowPlacement::CommitWnd( CWnd* pWnd, bool restoreToMax /*= false*/, bool setMinPos /*= false*/ )
{
	ASSERT_PTR( pWnd->GetSafeHwnd() );
	CWindowPlacement* pThis = const_cast<CWindowPlacement*>( this );

	if ( restoreToMax )
		pThis->flags |= WPF_RESTORETOMAXIMIZED;

	if ( setMinPos )
		pThis->flags |= WPF_SETMINPOSITION;

	// clamp to visible monitor work-area
	if ( ui::IsTopLevel( pWnd->m_hWnd ) )
		ui::EnsureVisibleDesktopRect( (CRect&)rcNormalPosition );
	else
		ui::EnsureVisibleWindowRect( (CRect&)rcNormalPosition, pWnd->GetSafeHwnd() );		// clamp to parent's rect

	return pWnd->SetWindowPlacement( this ) != FALSE;
}

int CWindowPlacement::ChangeMaximizedShowCmd( UINT showCmd )
{
	if ( HasFlag( this->flags, WPF_RESTORETOMAXIMIZED ) || SW_SHOWMAXIMIZED == this->showCmd )
	{	// actually hide now and further show as maximized
		this->showCmd = showCmd;
		return SW_SHOWMAXIMIZED;				// delayed show command (2nd step)
	}
	return this->showCmd;
}

void CWindowPlacement::Stream( CArchive& archive )
{
	if ( archive.IsStoring() )
		archive.Write( (tagWINDOWPLACEMENT*)this, sizeof( WINDOWPLACEMENT ) );
	else
		archive.Read( (tagWINDOWPLACEMENT*)this, sizeof( WINDOWPLACEMENT ) );
}
