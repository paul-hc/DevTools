
#include "pch.h"
#include "MultiValueBase.h"
#include "ScopedGdi.h"
#include "VisualTheme.h"
#include "WndUtils.h"
#include "utl/EnumTags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace multi
{
	const CEnumTags& GetTags_MultiValueState( void )
	{
		static const CEnumTags s_tags( _T("<null>|<shared value>|<different values>") );
		return s_tags;
	}

	const std::tstring& GetMultipleValueTag( void )
	{
		return GetTags_MultiValueState().FormatUi( MultipleValue );
	}
}


namespace ui
{
	CMultiValueBase::CMultiValueBase( void )
		: m_multiValueTag( multi::GetMultipleValueTag() )
	{
	}

	void CMultiValueBase::DrawMultiValueText( CDC* pDC, const CWnd* pCtrl, HBRUSH hBkBrush /*= ::GetSysColorBrush( COLOR_WINDOW )*/, UINT dtFmt /*= TextFmt*/ )
	{
		ASSERT_PTR( pDC );
		ASSERT_PTR( pCtrl->GetSafeHwnd() );

		CRect rect;

		pCtrl->GetClientRect( &rect );

		if ( hBkBrush != nullptr )
			ui::FillRect( *pDC, rect, hBkBrush );

		CFont* pOldFont = pDC->SelectObject( pCtrl->GetParent()->GetFont() );
		COLORREF oldTextColor = pDC->SetTextColor( ::GetSysColor( COLOR_GRAYTEXT ) );
		int oldBkMode = pDC->SetBkMode( TRANSPARENT );

		rect.left += HorizEdge;
		pDC->DrawText( m_multiValueTag.c_str(), (int)m_multiValueTag.size(), &rect, dtFmt );

		pDC->SetTextColor( oldTextColor );
		pDC->SetBkMode( oldBkMode );
		pDC->SelectObject( pOldFont );
	}

	void CMultiValueBase::DrawEditCueBanner( CDC* pDC, const CEdit* pEdit, UINT dtFmt /*= TextFmt*/ )
	{	// draws only the cue banner text, assuming the borders and background have been painted by sending WM_ERASEBKGND and WM_PRINTCLIENT messages.
		ASSERT_PTR( pEdit->GetSafeHwnd() );

		COLORREF cueTextColor = ::GetSysColor( COLOR_GRAYTEXT );
		CRect textRect;

		pEdit->GetRect( &textRect );

		CVisualTheme theme( L"EDIT", pEdit->GetSafeHwnd() );
		CFont* pOldFont = pDC->SelectObject( pEdit->GetFont() );
		int oldBkMode = pDC->SetBkMode( TRANSPARENT );

		if ( theme.IsThemed() )
		{
			cueTextColor = theme.GetThemeColor( EP_EDITTEXT, ETS_CUEBANNER, TMT_TEXTCOLOR, cueTextColor );		// fallback to disabled text (gray) default
			pDC->SetTextColor( cueTextColor );

			// draw the cue banner text
			theme.DrawThemeText( pDC->GetSafeHdc(), EP_EDITTEXT, ETS_CUEBANNER, textRect, GetMultiValueTag().c_str(), dtFmt );
		}
		else
		{	// non-themed classic fallback
			pDC->SetTextColor( cueTextColor );
			pDC->DrawText( GetMultiValueTag().c_str(), -1, &textRect, dtFmt );
		}

		pDC->SelectObject( pOldFont );
		pDC->SetBkMode( oldBkMode );
	}
}
