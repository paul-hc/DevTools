
#include "pch.h"
#include "ColorPickerButton.h"
#include "ColorRepository.h"
#include "Image_fwd.h"
#include "utl/Algorithms.h"
#include <math.h>

#include <afxcolorpopupmenu.h>
#include <afxcolorbar.h>
#include <afxtoolbarmenubutton.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CColorPickerButton implementation

CColorPickerButton::CColorPickerButton( void )
	: CMFCColorButton()
	, m_pTableGroup( new CColorTableGroup( *CColorRepository::Instance() ) )
	, m_hPopup( nullptr )
{
}

CColorPickerButton::~CColorPickerButton()
{
}

void CColorPickerButton::StoreColors( const std::vector<COLORREF>& colors )
{
	m_Colors.SetSize( colors.size() );
	utl::Copy( colors.begin(), colors.end(), m_Colors.GetData() );
}

void CColorPickerButton::SetHalftoneColors( size_t size /*= 256*/ )
{
	std::vector<COLORREF> halftoneColors;
	CHalftoneColorTable::MakeColorTable( halftoneColors, size );
	StoreColors( halftoneColors );
}

void CColorPickerButton::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const override
{
	cmdId, pTooltip;

	if ( m_pTableGroup.get() != nullptr )
		rText = m_pTableGroup->FormatColorMatch( GetColor() );
}


// base overrides:

void CColorPickerButton::OnShowColorPopup( void ) override
{
	if ( -1 == m_nColumns )		// auto-layout for columns?
	{
		size_t colorCount = m_Colors.GetSize();

		if ( 16 == colorCount )
			m_nColumns = 8;
		else if ( colorCount >= 64 )
			m_nColumns = static_cast<int>( sqrt( static_cast<double>( colorCount ) ) );
	}

	__super::OnShowColorPopup();

	CMFCPopupMenuBar* pPopupMenuBar = m_pPopup->GetMenuBar();
	CMFCColorBar* pColorBar = checked_static_cast<CMFCColorBar*>( pPopupMenuBar );
	pColorBar = pColorBar;

	pColorBar->InsertSeparator();
	if ( m_hPopup != nullptr )
	{
		//CMFCToolBarMenuButton* pPopulItem = new CMFCToolBarMenuButton( 333, m_dbgPopup, -1, _T("<debug-popup>") );
		CMFCToolBarMenuButton popupItem( 333, m_hPopup, -1, _T("<debug-popup>") );
		pColorBar->InsertButton( popupItem );
	}
}


// CMenuPickerButton implementation

CMenuPickerButton::CMenuPickerButton( void )
	: CMFCMenuButton()
{
}

CMenuPickerButton::~CMenuPickerButton()
{
}


// message handlers
BEGIN_MESSAGE_MAP(CMenuPickerButton, CMFCMenuButton)
	ON_WM_INITMENUPOPUP()
END_MESSAGE_MAP()

void CMenuPickerButton::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	__super::OnInitMenuPopup( pPopupMenu, index, isSysMenu );

	GetParent()->SendMessage( WM_INITMENUPOPUP, (WPARAM)pPopupMenu->GetSafeHmenu(), MAKELPARAM( index, isSysMenu ) );
}
