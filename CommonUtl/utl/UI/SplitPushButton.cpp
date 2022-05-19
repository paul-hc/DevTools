
#include "stdafx.h"
#include "SplitPushButton.h"
#include "ImageStore.h"
#include "ScopedGdi.h"
#include "StringUtilities.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CSplitPushButton::CSplitPushButton( void )
	: CBaseSplitButton()
	, m_activeSide( LeftSide )
{
}

CSplitPushButton::~CSplitPushButton()
{
}

const CIcon* CSplitPushButton::GetRhsIcon( void ) const
{
	if ( m_rhsIconId.IsValid() )
		return ui::GetImageStoresSvc()->RetrieveIcon( m_rhsIconId );

	return NULL;
}

void CSplitPushButton::SetRhsIconId( const CIconId& rhsIconId )
{
	m_rhsIconId = rhsIconId;

	if ( m_hWnd != NULL )
		Invalidate();
}

void CSplitPushButton::SetRhsText( const std::tstring& rhsText )
{
	m_rhsText = rhsText;
	if ( m_hWnd != NULL )
		Invalidate();
}

void CSplitPushButton::SetActiveSide( ButtonSide activeSide )
{
	if ( m_activeSide == activeSide )
		return;
	m_activeSide = activeSide;
	Invalidate();
}

CRect CSplitPushButton::GetRhsPartRect( const CRect* pClientRect /*= NULL*/ ) const
{
	CRect rhsRect( 0, 0, 0, 0 );
	if ( HasRhsPart() )
	{
		if ( pClientRect != NULL )
			rhsRect = *pClientRect;
		else
			GetClientRect( &rhsRect );

		rhsRect.DeflateRect( DefaultBorderSpacing, DefaultBorderSpacing );

		int contentWidth = 0;
		if ( const CIcon* pRhsIcon = GetRhsIcon() )
			contentWidth += pRhsIcon->GetSize().cx + 2 * IconSpacing;
		if ( !m_rhsText.empty() )
		{
			CClientDC dc( const_cast<CSplitPushButton*>( this ) );
			CScopedGdi< CFont > scopedFont( &dc, GetFont() );
			contentWidth += ui::GetTextSize( &dc, m_rhsText.c_str() ).cx + 2 * TextSpacing;
		}
		if ( contentWidth != 0 )
			contentWidth += 2 * FocusSpacing;

		rhsRect.left = rhsRect.right - contentWidth;
	}
	return rhsRect;
}

void CSplitPushButton::DrawRhsPart( CDC* pDC, const CRect& clientRect )
{
	CBaseSplitButton::DrawRhsPart( pDC, clientRect );

	bool enabled = !HasFlag( GetStyle(), WS_DISABLED );
	CRect rhsRect = GetRhsPartRect( &clientRect ), rect = rhsRect;
	rect.DeflateRect( FocusSpacing, FocusSpacing );

	if ( const CIcon* pRhsIcon = GetRhsIcon() )
	{
		CRect iconRect( 0, 0, pRhsIcon->GetSize().cx, pRhsIcon->GetSize().cy );
		ui::AlignRectHV( iconRect, rect, H_AlignLeft, V_AlignCenter );
		iconRect.OffsetRect( IconSpacing, 0 );

		pRhsIcon->Draw( *pDC, iconRect.TopLeft(), enabled );
		rect.left = iconRect.right + IconSpacing;
	}

	if ( !m_rhsText.empty() )
	{
		rect.left += TextSpacing;
		CScopedGdi< CFont > scopedFont( pDC, GetFont() );
		COLORREF textColor = pDC->SetTextColor( GetSysColor( enabled ? COLOR_BTNTEXT : COLOR_GRAYTEXT ) );
		int oldBkMode = pDC->SetBkMode( TRANSPARENT );

		pDC->DrawText( m_rhsText.c_str(), static_cast<int>( m_rhsText.length() ), &rect, DT_SINGLELINE | DT_LEFT | DT_VCENTER );

		pDC->SetTextColor( textColor );
		pDC->SetBkMode( oldBkMode );
	}
}

void CSplitPushButton::DrawFocus( CDC* pDC, const CRect& clientRect )
{
	if ( RightSide == m_activeSide )
	{	// draw focus on right-hand-side
		CRect focusRect = GetRhsPartRect( &clientRect );
		focusRect.DeflateRect( FocusSpacing, FocusSpacing );
		pDC->DrawFocusRect( &focusRect );
	}
	else
		CBaseSplitButton::DrawFocus( pDC, clientRect );		// draw focus on left-hand-side
}

void CSplitPushButton::PreSubclassWindow( void )
{
	std::vector< std::tstring > textParts;
	str::Split( textParts, ui::GetWindowText( this ).c_str(), _T("|") );
	if ( 2 == textParts.size() )
	{
		SetButtonCaption( textParts[ 0 ] );
		SetRhsText( textParts[ 1 ] );
	}

	CBaseSplitButton::PreSubclassWindow();
}


// message handlers

BEGIN_MESSAGE_MAP( CSplitPushButton, CBaseSplitButton )
	ON_WM_LBUTTONDOWN()
	ON_CONTROL_REFLECT_EX( BN_CLICKED, OnReflectClicked )
END_MESSAGE_MAP()

void CSplitPushButton::OnLButtonDown( UINT flags, CPoint point )
{
	CBaseSplitButton::OnLButtonDown( flags, point );

	if ( HasRhsPart() )
		if ( GetRhsPartRect().PtInRect( point ) )
			SetActiveSide( RightSide );
		else
			SetActiveSide( LeftSide );
}

BOOL CSplitPushButton::OnReflectClicked( void )
{
	if ( HasRhsPart() && RightSide == m_activeSide )
		if ( GetRhsPartRect().PtInRect( ui::GetCursorPos( *this ) ) )
		{
			ui::SendCommandToParent( *this, SBN_RIGHTCLICKED );
			return TRUE;	// handled
		}

	return FALSE;			// continue handling
}
