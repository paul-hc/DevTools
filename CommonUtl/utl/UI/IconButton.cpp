
#include "pch.h"
#include "IconButton.h"
#include "ImageStore.h"
#include "StringUtilities.h"
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const TCHAR CIconButton::s_textSpacing[] = _T(" ");

CIconButton::CIconButton( const CIconId& iconId /*= CIconId()*/, bool useText /*= true*/ )
	: CButton()
	, m_iconId( iconId )
	, m_useText( useText )
	, m_useTextSpacing( true )
{
}

CIconButton::~CIconButton()
{
}

const CIcon* CIconButton::GetIconPtr( void ) const
{
	if ( m_iconId.m_id != 0 )
		return ui::GetImageStoresSvc()->RetrieveIcon( m_iconId );

	return nullptr;
}

void CIconButton::SetIconId( const CIconId& iconId )
{
	m_iconId = iconId;

	if ( m_hWnd != nullptr )
	{
		const CIcon* pIcon = GetIconPtr();
		if ( pIcon != nullptr )
			if ( !m_useText )
				ModifyStyle( 0, BS_ICON );

		SetIcon( pIcon != nullptr ? pIcon->GetHandle() : nullptr );
		UpdateCaption( this, m_useText, m_useTextSpacing && pIcon != nullptr );
	}
}

void CIconButton::SetUseTextSpacing( bool useTextSpacing /*= true*/ )
{
	m_useTextSpacing = useTextSpacing;
	if ( m_hWnd != nullptr )
		UpdateCaption( this, m_useText, m_useTextSpacing );
}

std::tstring CIconButton::GetButtonCaption( void ) const
{
	return TextToCaption( ui::GetWindowText( m_hWnd ), m_useText, m_useTextSpacing );
}

void CIconButton::SetButtonCaption( const std::tstring& caption )
{
	ui::SetWindowText( m_hWnd, CaptionToText( caption, m_useText, m_useTextSpacing ) );
}

void CIconButton::SetButtonIcon( CButton* pButton, const CIconId& iconId, bool useText /*= true*/, bool useTextSpacing /*= true*/ )
{
	ASSERT_PTR( pButton->GetSafeHwnd() );

	HICON hOldIcon = pButton->GetIcon(), hNewIcon = nullptr;

	if ( const CIcon* pIcon = ui::GetImageStoresSvc()->RetrieveIcon( iconId ) )
		hNewIcon = pIcon->GetHandle();

	if ( hNewIcon != hOldIcon )		// icon has changed?
		pButton->SetIcon( hNewIcon );

	UpdateCaption( pButton, useText, useTextSpacing );
}

std::tstring CIconButton::CaptionToText( const std::tstring& caption, bool useText, bool useTextSpacing )
{
	std::tstring text = caption;

	if ( useText && useTextSpacing )
	{
		str::StripPrefix( text, s_textSpacing );
		str::StripSuffix( text, s_textSpacing );

		if ( !text.empty() )
			text = std::tstring( s_textSpacing ) + text + s_textSpacing;
	}
	return text;
}

std::tstring CIconButton::TextToCaption( const std::tstring& text, bool useText, bool useTextSpacing )
{
	std::tstring caption = text;
	if ( useText && useTextSpacing )
	{
		str::StripPrefix( caption, s_textSpacing );
		str::StripSuffix( caption, s_textSpacing );
	}
	return caption;
}

bool CIconButton::UpdateIcon( void )
{
	// When the button is created using Create(), strangely the initial SetIcon call on PreSubclassWindow() doesn't stick.
	// We need to set the current icon again.
	ASSERT_PTR( m_hWnd );

	if ( 0 == m_iconId.m_id || GetIcon() != nullptr )
		return false;

	SetIconId( m_iconId );
	return true;
}

bool CIconButton::UpdateCaption( CButton* pButton, bool useText, bool useTextSpacing )
{
	ASSERT_PTR( pButton->GetSafeHwnd() );
	if ( pButton->GetIcon() != nullptr )
		if ( useText )
			return ui::SetWindowText( pButton->GetSafeHwnd(), CaptionToText( ui::GetWindowText( pButton->GetSafeHwnd() ), useText, useTextSpacing ) );
		else
			return pButton->ModifyStyle( 0, BS_ICON ) != 0;

	return false;
}

void CIconButton::PreSubclassWindow( void )
{
	CButton::PreSubclassWindow();

	if ( 0 == m_iconId.m_id )
		m_iconId.m_id = GetDlgCtrlID();

	if ( nullptr == GetIcon() )
		UpdateIcon();
}
