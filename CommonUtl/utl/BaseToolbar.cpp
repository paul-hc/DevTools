
#include "stdafx.h"
#include "BaseToolbar.h"
#include "DibDraw.h"
#include "MenuUtilities.h"
#include <afxpriv.h>		// for WM_IDLEUPDATECMDUI

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CBaseToolbar::CBaseToolbar( void )
	: CToolBar()
{
}

CBaseToolbar::~CBaseToolbar()
{
}

bool CBaseToolbar::LoadToolStrip( UINT toolStripId, COLORREF transpColor /*= color::Auto*/ )
{
	ASSERT_PTR( m_hWnd );
	return
		m_strip.LoadStrip( toolStripId, transpColor ) &&
		InitToolbarButtons();
}

bool CBaseToolbar::InitToolbarButtons( void )
{
	ASSERT_PTR( m_hWnd );
	ASSERT( m_strip.IsValid() );

	if ( !SetButtons( &m_strip.m_buttonIds.front(), static_cast< int >( m_strip.m_buttonIds.size() ) ) )
		return false;

	CSize buttonSize = m_strip.m_imageSize + CSize( BtnEdgeWidth, BtnEdgeHeight );
	SetSizes( buttonSize, m_strip.m_imageSize );			// set new sizes of the buttons

	if ( CImageList* pImageList = m_strip.EnsureImageList() )
		GetToolBarCtrl().SetImageList( pImageList );
	return true;
}

void CBaseToolbar::SetCustomDisabledImageList( void )
{
	if ( CImageList* pImageList = m_strip.EnsureImageList() )
	{
		m_pDisabledImageList.reset( new CImageList );
		gdi::MakeDisabledImageList( *m_pDisabledImageList, *pImageList, gdi::DisabledBlendColor, GetSysColor( COLOR_3DLIGHT ), 128 );
		GetToolBarCtrl().SetDisabledImageList( m_pDisabledImageList.get() );
	}
	else
		ASSERT( false );
}

void CBaseToolbar::UpdateCmdUI( void )
{
	if ( m_hWnd != NULL )
		SendMessage( WM_IDLEUPDATECMDUI, (WPARAM)TRUE );
}

void CBaseToolbar::TrackButtonMenu( UINT buttonId, CWnd* pTargetWnd, CMenu* pPopupMenu, ui::PopupAlign popupAlign )
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

BEGIN_MESSAGE_MAP( CBaseToolbar, CToolBar )
END_MESSAGE_MAP()
