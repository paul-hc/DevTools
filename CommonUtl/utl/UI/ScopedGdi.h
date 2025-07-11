#ifndef ScopedGdi_h
#define ScopedGdi_h
#pragma once


class CScopedGdiBatchLimit : private utl::noncopyable
{
public:
	CScopedGdiBatchLimit( DWORD batchLimit = 1 ) : m_oldBatchLimit( ::GdiGetBatchLimit() ) { ::GdiSetBatchLimit( batchLimit ); }
	~CScopedGdiBatchLimit() { ::GdiSetBatchLimit( m_oldBatchLimit ); }
private:
	DWORD m_oldBatchLimit;
};


class CScopedGdiObj : private utl::noncopyable
{
public:
	CScopedGdiObj( CDC* pDC, HGDIOBJ hGdiObj ) : m_pDC( pDC ), m_hOldGdiObj( SelectObject( hGdiObj ) ) { ASSERT_PTR( m_pDC ); }
	CScopedGdiObj( CDC* pDC, int stockObjectIndex ) : m_pDC( pDC ), m_hOldGdiObj( SelectStockObject( stockObjectIndex ) ) { ASSERT_PTR( m_pDC ); }
	CScopedGdiObj( HDC hDC, HGDIOBJ hGdiObj ) : m_pDC( CDC::FromHandle( hDC ) ), m_hOldGdiObj( SelectObject( hGdiObj ) ) { ASSERT_PTR( m_pDC ); }
	~CScopedGdiObj() { m_pDC->SelectObject( m_hOldGdiObj ); }

	HGDIOBJ GetOldObject( void ) const { return m_hOldGdiObj; }
	HGDIOBJ SelectObject( HGDIOBJ hGdiObj ) { return m_pDC->SelectObject( hGdiObj ); }      // do not use for regions

	HGDIOBJ SelectStockObject( int stockObjectIndex ) { m_pDC->SelectStockObject( stockObjectIndex )->GetSafeHandle(); }
private:
	CDC* m_pDC;
	HGDIOBJ m_hOldGdiObj;
};


template< typename GdiObjectType >
class CScopedGdi : private utl::noncopyable
{
public:
	CScopedGdi( CDC* pDC, GdiObjectType* pGdiObject )
		: m_pDC( pDC )
		, m_pOldGdiObject( m_pDC->SelectObject( pGdiObject ) )
	{
		ASSERT_PTR( m_pDC );
	}

	CScopedGdi( CDC* pDC, int stockObjectIndex )
		: m_pDC( pDC )
		, m_pOldGdiObject( m_pDC->SelectStockObject( stockObjectIndex ) )
	{
		ASSERT_PTR( m_pDC );
	}

	~CScopedGdi()
	{
		Restore();
	}

	void Restore( void )
	{
		if ( m_pDC != nullptr )
		{
			m_pDC->SelectObject( m_pOldGdiObject );
			m_pDC = nullptr;
			m_pOldGdiObject = nullptr;
		}
	}

	GdiObjectType* GetOldObject( void ) const
	{
		return m_pOldGdiObject;
	}

	GdiObjectType* SelectObject( GdiObjectType* pNewGdiObject )
	{
		return checked_static_cast<GdiObjectType*>( m_pDC->SelectObject( pNewGdiObject ) );
	}

	CGdiObject* SelectStockObject( int stockObjectIndex )
	{
		return m_pDC->SelectStockObject( stockObjectIndex );
	}
private:
	CDC* m_pDC;
	GdiObjectType* m_pOldGdiObject;
};


class CScopedPalette : private utl::noncopyable
{
public:
	CScopedPalette( CDC* pDC, CPalette* pPalette, bool realize = true, bool forceBackground = false )
		: m_pPaletteDC( pPalette != nullptr && HasFlag( pDC->GetDeviceCaps( RASTERCAPS ), RC_PALETTE ) ? pDC : nullptr )
		, m_pOldPalette( m_pPaletteDC != nullptr ? m_pPaletteDC->SelectPalette( pPalette, forceBackground ) : nullptr )
		, m_forceBackground( forceBackground )
	{
		if ( realize && m_pPaletteDC != nullptr )
			if ( GDI_ERROR == m_pPaletteDC->RealizePalette() )
				TRACE( "GDI_ERROR on RealizePalette() containing %d colors!\n", pPalette->GetEntryCount() );
	}

	~CScopedPalette()
	{
		if ( m_pPaletteDC != nullptr )
			m_pPaletteDC->SelectPalette( m_pOldPalette, m_forceBackground );
	}
private:
	CDC* m_pPaletteDC;
	CPalette* m_pOldPalette;
	bool m_forceBackground;
};


#endif // ScopedGdi_h
