
#include "stdafx.h"
#include "EnumSplitButton.h"
#include "EnumTags.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CEnumSplitButton::CEnumSplitButton( const CEnumTags* pEnumTags )
	: CPopupSplitButton()
	, m_pEnumTags( NULL )
	, m_selValue( 0 )
{
	SetEnumTags( pEnumTags );
}

CEnumSplitButton::~CEnumSplitButton()
{
}

void CEnumSplitButton::SetEnumTags( const CEnumTags* pEnumTags )
{
	m_pEnumTags = pEnumTags;
	if ( NULL == m_pEnumTags )
		return;

	SetTargetWnd( this );
	m_popupMenu.CreatePopupMenu();

	ASSERT( !m_pEnumTags->GetUiTags().empty() );
	unsigned int count = static_cast< UINT >( m_pEnumTags->GetUiTags().size() );

	for ( unsigned int i = 0; i != count; ++i )
		m_popupMenu.AppendMenu( MF_STRING, IdFirstCommand + i, m_pEnumTags->FormatUi( i ).c_str() );

	m_popupMenu.CheckMenuRadioItem( 0, count - 1, m_selValue, MF_BYPOSITION );
}

void CEnumSplitButton::SetSelValue( int selValue )
{
	ASSERT( m_pEnumTags != NULL && !m_pEnumTags->GetUiTags().empty() );
	unsigned int count = static_cast< UINT >( m_pEnumTags->GetUiTags().size() );

	ASSERT( selValue >= 0 && selValue < static_cast< int >( count ) );
	m_selValue = selValue;
	m_popupMenu.CheckMenuRadioItem( 0, count - 1, m_selValue, MF_BYPOSITION );

	if ( m_hWnd != NULL && m_pEnumTags != NULL )
		SetButtonCaption( m_pEnumTags->FormatUi( m_selValue ) );
}

void CEnumSplitButton::PreSubclassWindow( void )
{
	CPopupSplitButton::PreSubclassWindow();

	if ( m_pEnumTags != NULL && m_selValue >= 0 )
		SetButtonCaption( m_pEnumTags->FormatUi( m_selValue ) );
}


// message handlers

BEGIN_MESSAGE_MAP( CEnumSplitButton, CPopupSplitButton )
	ON_COMMAND_RANGE( IdFirstCommand, IdLastCommand, OnSelectValue )
END_MESSAGE_MAP()

void CEnumSplitButton::OnSelectValue( UINT cmdId )
{
	SetSelValue( cmdId - IdFirstCommand );
	ui::SendCommandToParent( *this, BN_CLICKED );
}