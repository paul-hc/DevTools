
#include "StdAfx.h"
#include "WindowPlacement.h"
#include "utl/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CWindowPlacement::CWindowPlacement( void )
{
	memset( (tagWINDOWPLACEMENT*)this, 0, sizeof( tagWINDOWPLACEMENT ) );
	this->length = sizeof( WINDOWPLACEMENT );
}

CWindowPlacement::~CWindowPlacement()
{
}

bool CWindowPlacement::GetPlacement( const CWnd* pWnd )
{
	ASSERT_PTR( pWnd );
	return pWnd->GetWindowPlacement( this ) != FALSE;
}

bool CWindowPlacement::SetPlacement( CWnd* pWnd, bool restoreToMax /*= false*/, bool setMinPos /*= false*/ ) const
{
	ASSERT_PTR( pWnd );
	CWindowPlacement* pThis = const_cast< CWindowPlacement* >( this );

	if ( restoreToMax )
		pThis->flags |= WPF_RESTORETOMAXIMIZED;

	if ( setMinPos )
		pThis->flags |= WPF_SETMINPOSITION;

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
