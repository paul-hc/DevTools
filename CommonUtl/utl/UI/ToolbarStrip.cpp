
#include "pch.h"
#include "ToolbarStrip.h"
#include "DibDraw.h"
#include "MenuUtilities.h"
#include <afxpriv.h>		// for WM_IDLEUPDATECMDUI

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CToolbarStrip::CToolbarStrip( gdi::DisabledStyle disabledStyle /*= gdi::Dis_FadeGray*/ )
	: CToolBar()
	, m_enableUnhandledCmds( false )
	, m_disabledStyle( disabledStyle )
{
}

CToolbarStrip::~CToolbarStrip()
{
}

bool CToolbarStrip::LoadToolStrip( UINT toolStripId, COLORREF transpColor /*= CLR_NONE*/ )
{
	ASSERT_PTR( m_hWnd );

	return
		m_strip.LoadToolbar( toolStripId, transpColor ) &&
		InitToolbarButtons();
}

bool CToolbarStrip::InitToolbarButtons( void )
{
	ASSERT_PTR( m_hWnd );
	ASSERT( m_strip.HasButtons() );

	if ( !SetButtons( ARRAY_SPAN_V( m_strip.GetButtonIds() ) ) )
		return false;

	CSize buttonSize = m_strip.GetImageSize() + CSize( BtnEdgeWidth, BtnEdgeHeight );
	SetSizes( buttonSize, m_strip.GetImageSize() );			// set new sizes of the buttons

	if ( CImageList* pImageList = m_strip.EnsureImageList() )
		GetToolBarCtrl().SetImageList( pImageList );

	if ( m_disabledStyle != gdi::Dis_MfcStd )
		SetCustomDisabledImageList();

	return true;
}

CSize CToolbarStrip::GetIdealBarSize( void ) const
{
	CSize idealBarSize;
	GetToolBarCtrl().GetMaxSize( &idealBarSize );
	return idealBarSize;
}

void CToolbarStrip::SetDisabledStyle( gdi::DisabledStyle disabledStyle )
{
	m_disabledStyle = disabledStyle;

	if ( m_strip.HasButtons() && m_disabledStyle != gdi::Dis_MfcStd )
		SetCustomDisabledImageList();
}

void CToolbarStrip::SetCustomDisabledImageList( void )
{
	if ( CImageList* pImageList = m_strip.EnsureImageList() )
	{
		m_pDisabledImageList.reset( new CImageList() );

		if ( gdi::MakeDisabledImageList( m_pDisabledImageList.get(), *pImageList, m_disabledStyle ) )
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
	if ( m_hWnd != nullptr )
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
