
#include "pch.h"
#include "TextEffect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	// CFrameFillTraits implementation

	void CFrameFillTraits::Init( COLORREF color, UINT frameOpacityPct, UINT fillOpacityPct )
	{
		m_color = color;
		m_frameAlpha = MakeOpaqueAlpha( frameOpacityPct );
		m_fillAlpha = MakeOpaqueAlpha( fillOpacityPct );
	}


	// CTextEffect implementation

	const ui::CTextEffect CTextEffect::s_null;

	bool CTextEffect::AssignPtr( const CTextEffect* pRight )
	{
		if ( nullptr == pRight )
			return false;

		if ( pRight != this )
			*this = *pRight;
		return true;
	}

	void CTextEffect::Combine( const CTextEffect& right )
	{
		if ( &right != this )
		{
			m_fontEffect |= right.m_fontEffect;

			if ( right.m_textColor != CLR_NONE )
				m_textColor = right.m_textColor;

			if ( right.m_bkColor != CLR_NONE )
				m_bkColor = right.m_bkColor;

			if ( !right.m_frameFillTraits.IsNull() )
				m_frameFillTraits = right.m_frameFillTraits;
		}
	}

	bool CTextEffect::CombinePtr( const CTextEffect* pRight )
	{
		if ( nullptr == pRight )
			return false;

		Combine( *pRight );
		return true;
	}
}
