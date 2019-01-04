
#include "stdafx.h"
#include "MemoryDC.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CMemoryDC::CMemoryDC( CDC& dc, bool useMemDC /*= true*/ )
	: m_rDC( dc )
	, m_pOldBitmap( NULL )
{
	m_rDC.GetClipBox( &m_sourceRect );
	if ( useMemDC )
		Construct();
}

CMemoryDC::CMemoryDC( CDC& dc, const CRect& rect, bool useMemDC /*= true*/ )
	: m_rDC( dc )
	, m_sourceRect( rect )
	, m_pOldBitmap( NULL )
{
	if ( useMemDC )
		Construct();
}

CMemoryDC::~CMemoryDC()
{
	if ( IsMemDC() )
	{
		m_rDC.BitBlt( m_sourceRect.left, m_sourceRect.top, m_sourceRect.Width(), m_sourceRect.Height(), &m_memDC, m_sourceRect.left, m_sourceRect.top, SRCCOPY );
		m_memDC.SelectObject( m_pOldBitmap );
	}
}

void CMemoryDC::Construct( void )
{
	if ( !m_rDC.IsPrinting() )
		if ( m_memDC.CreateCompatibleDC( &m_rDC ) )
		{
			m_rDC.LPtoDP( &m_sourceRect );
			if ( m_bitmap.CreateCompatibleBitmap( &m_rDC, m_sourceRect.Width(), m_sourceRect.Height() ) )
			{
				m_pOldBitmap = m_memDC.SelectObject( &m_bitmap );
				m_rDC.DPtoLP( &m_sourceRect );
				m_memDC.SetWindowOrg( m_sourceRect.TopLeft() );
			}
		}

	if ( m_bitmap.GetSafeHandle() != NULL )
	{
		m_memDC.m_bPrinting = m_rDC.m_bPrinting;
		m_memDC.m_hAttribDC = m_rDC.m_hAttribDC;
	}
	else
	{
		m_memDC.DeleteDC();			// release eventually created compatible DC
		ASSERT_NULL( m_memDC.GetSafeHdc() );
	}
}
