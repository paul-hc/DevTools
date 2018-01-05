
#include "stdafx.h"
#include "IconButton.h"
#include "ImageStore.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CIconButton::CIconButton( const CIconId& iconId /*= CIconId()*/, bool useText /*= true*/ )
	: CButton()
	, m_iconId( iconId )
	, m_useText( useText )
{
}

CIconButton::~CIconButton()
{
}

const CIcon* CIconButton::GetIconPtr( void ) const
{
	if ( m_iconId.m_id != 0 && CImageStore::HasSharedStore() )
		return CImageStore::GetSharedStore()->RetrieveIcon( m_iconId );

	return NULL;
}

void CIconButton::SetIconId( const CIconId& iconId )
{
	m_iconId = iconId;

	if ( m_hWnd != NULL )
	{
		if ( const CIcon* pIcon = GetIconPtr() )
		{
			if ( !m_useText )
				ModifyStyle( 0, BS_ICON );

			SetIcon( pIcon->GetHandle() );
		}
		UpdateCaption( this, m_useText, m_useTextSpacing );
	}
}

void CIconButton::SetUseTextSpacing( bool useTextSpacing /*= true*/ )
{
	m_useTextSpacing = useTextSpacing;
	if ( m_hWnd != NULL )
		UpdateCaption( this, m_useText, m_useTextSpacing );
}

bool CIconButton::UpdateIcon( void )
{
	// When the button is created using Create(), strangely the initial SetIcon call on PreSubclassWindow() doesn't stick.
	// We need to set the current icon again.
	ASSERT_PTR( m_hWnd );

	if ( 0 == m_iconId.m_id || GetIcon() != NULL )
		return false;

	SetIconId( m_iconId );
	return true;
}

bool CIconButton::UpdateCaption( CButton* pButton, bool useText, bool useTextSpacing )
{
	ASSERT_PTR( pButton->GetSafeHwnd() );
	if ( pButton->GetIcon() != NULL )
		if ( useText )
		{
			std::tstring text = ui::GetWindowText( *pButton );
			if ( !text.empty() )		// handle spacing only if we have caption
			{
				static const TCHAR space[] = _T(" ");
				if ( !useTextSpacing )
					str::TrimLeft( text, space );
				else if ( text[ 0 ] != space[ 0 ] )
					text = space + text;

				return ui::SetWindowText( *pButton, text );
			}
		}
		else
			pButton->ModifyStyle( 0, BS_ICON );

	return false;
}

void CIconButton::PreSubclassWindow( void )
{
	CButton::PreSubclassWindow();

	if ( 0 == m_iconId.m_id )
		m_iconId.m_id = GetDlgCtrlID();

	if ( NULL == GetIcon() )
		UpdateIcon();
}

void CIconButton::SetButtonIcon( CButton* pButton, const CIconId& iconId, bool useText /*= true*/, bool useTextSpacing /*= true*/ )
{
	ASSERT_PTR( pButton->GetSafeHwnd() );

	if ( const CIcon* pIcon = CImageStore::SharedStore()->RetrieveIcon( iconId ) )
		pButton->SetIcon( pIcon->GetHandle() );
	UpdateCaption( pButton, useText, useTextSpacing );
}
