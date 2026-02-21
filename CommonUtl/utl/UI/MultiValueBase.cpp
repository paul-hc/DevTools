
#include "pch.h"
#include "MultiValueBase.h"
#include "ScopedGdi.h"
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
			ui::FillRect( *pDC, rect, ::GetSysColorBrush( COLOR_WINDOW ) );

		CScopedGdi<CFont> scopedFont( pDC, pCtrl->GetParent()->GetFont() );
		COLORREF oldTextColor = pDC->SetTextColor( ::GetSysColor( COLOR_SCROLLBAR ) );
		int oldBkMode = pDC->SetBkMode( TRANSPARENT );

		rect.left += HorizEdge;
		pDC->DrawText( m_multiValueTag.c_str(), (int)m_multiValueTag.size(), &rect, dtFmt );

		pDC->SetTextColor( oldTextColor );
		pDC->SetBkMode( oldBkMode );
	}
}
