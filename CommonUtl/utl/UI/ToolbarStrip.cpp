
#include "stdafx.h"
#include "ToolbarStrip.h"
#include "DibDraw.h"
#include "MenuUtilities.h"
#include <afxpriv.h>		// for WM_IDLEUPDATECMDUI

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CToolbarStrip::CToolbarStrip( void )
	: CToolBar()
	, m_enableUnhandledCmds( false )
{
}

CToolbarStrip::~CToolbarStrip()
{
}

bool CToolbarStrip::LoadToolStrip( UINT toolStripId, COLORREF transpColor /*= color::Auto*/ )
{
	ASSERT_PTR( m_hWnd );
	return
		m_strip.LoadStrip( toolStripId, transpColor ) &&
		InitToolbarButtons();
}

bool CToolbarStrip::InitToolbarButtons( void )
{
	ASSERT_PTR( m_hWnd );
	ASSERT( m_strip.IsValid() );

	if ( !SetButtons( ARRAY_PAIR_V( m_strip.m_buttonIds ) ) )
		return false;

	CSize buttonSize = m_strip.m_imageSize + CSize( BtnEdgeWidth, BtnEdgeHeight );
	SetSizes( buttonSize, m_strip.m_imageSize );			// set new sizes of the buttons

	if ( CImageList* pImageList = m_strip.EnsureImageList() )
		GetToolBarCtrl().SetImageList( pImageList );
	return true;
}

void CToolbarStrip::SetCustomDisabledImageList( gdi::DisabledStyle style /*= gdi::DisabledGrayOut*/ )
{
	if ( CImageList* pImageList = m_strip.EnsureImageList() )
	{
		m_pDisabledImageList.reset( new CImageList );
		if ( gdi::MakeDisabledImageList( *m_pDisabledImageList, *pImageList, style, GetSysColor( COLOR_BTNFACE ), 128 ) )	// was COLOR_3DLIGHT, 128
			GetToolBarCtrl().SetDisabledImageList( m_pDisabledImageList.get() );
		else
		{
			ASSERT( false );
			m_pDisabledImageList.reset();
		}
	}
	else
		ASSERT( false );
}

void CToolbarStrip::UpdateCmdUI( void )
{
	if ( m_hWnd != NULL )
		SendMessage( WM_IDLEUPDATECMDUI, (WPARAM)!m_enableUnhandledCmds );
}

void CToolbarStrip::TrackButtonMenu( UINT buttonId, CWnd* pTargetWnd, CMenu* pPopupMenu, ui::PopupAlign popupAlign )
{
	ASSERT_PTR( GetSafeHwnd() );
	ASSERT_PTR( pPopupMenu->GetSafeHmenu() );

	CToolBarCtrl& rToolBarCtrl = GetToolBarCtrl();
	rToolBarCtrl.PressButton( buttonId, TRUE );

	CRect screenRect;
	rToolBarCtrl.GetRect( buttonId, &screenRect );
	rToolBarCtrl.ClientToScreen( &screenRect );

	ui::TrackPopupMenuAlign( *pPopupMenu, pTargetWnd, screenRect, popupAlign, TPM_LEFTBUTTON );
	rToolBarCtrl.PressButton( buttonId, FALSE );
}


// message handlers

BEGIN_MESSAGE_MAP( CToolbarStrip, CToolBar )
END_MESSAGE_MAP()
