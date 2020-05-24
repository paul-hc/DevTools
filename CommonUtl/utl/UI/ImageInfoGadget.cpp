
#include "stdafx.h"
#include "ImageInfoGadget.h"
#include "RenderingDirect2D.h"
#include "StringUtilities.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace d2d
{
	// CImageInfoGadget implementation

	CImageInfoGadget::CImageInfoGadget( ui::IImageZoomView* pImageView, IDWriteTextFormat* pInfoFont )
		: m_pImageView( pImageView )
		, m_zoomPct( 0 )
		, m_pInfoFont( pInfoFont )
	{
		ASSERT_PTR( m_pImageView );
		ASSERT_PTR( m_pInfoFont );
	}

	CImageInfoGadget::~CImageInfoGadget()
	{
	}

	void CImageInfoGadget::DiscardDeviceResources( void )
	{
		m_pBkBrush = NULL;
		m_pTextBrush = NULL;
		m_pDimensionsBrush = NULL;
		m_pNavigBrush = NULL;
		m_pFrameTextBrush = NULL;

		m_pInfoTextLayout = NULL;
	}

	bool CImageInfoGadget::CreateDeviceResources( void )
	{
		ID2D1RenderTarget* pRenderTarget = GetHostRenderTarget();

		// text drawing objects
		enum { MyOliveGreen = RGB(204, 251, 93) };

		CreateAsSolidBrush( m_pBkBrush, pRenderTarget, ToColor( color::VeryDarkGrey, 50 ) );
		CreateAsSolidBrush( m_pTextBrush, pRenderTarget, ToColor( color::BrightGreen, 100 ) );
		CreateAsSolidBrush( m_pDimensionsBrush, pRenderTarget, ToColor( MyOliveGreen, 90 ) );
		CreateAsSolidBrush( m_pNavigBrush, pRenderTarget, ToColor( color::LightOrange ) );
		CreateAsSolidBrush( m_pFrameTextBrush, pRenderTarget, ToColor( color::Red, 100 ) );

		if ( m_info.IsValid() )
			MakeTextLayout();

		return true;
	}

	bool CImageInfoGadget::BuildInfo( void )
	{
		bool dirty = m_pInfoTextLayout == NULL;

		ui::CImageFileDetails newInfo;
		m_pImageView->QueryImageFileDetails( newInfo );
		if ( utl::ModifyValue( m_info, newInfo ) )
			dirty = true;

		UINT newZoomPct = m_pImageView->GetZoomView()->GetZoomPct();
		if ( utl::ModifyValue( m_zoomPct, newZoomPct ) )
			dirty = true;

		if ( !dirty )
			return false;

		MakeTextLayout();
		return true;
	}

	bool CImageInfoGadget::MakeTextLayout( void )
	{
		enum InfoField { FileName, Dimensions, NavigCounts, ZoomPercent,  _InfoFieldCount };

		dw::CTextLayout textLayout( _InfoFieldCount, _T(" ") );		// effect ranges indexed by InfoField

		textLayout.AddField( m_info.m_filePath.GetFilename() );

		textLayout.AddField( str::Format( _T("(%d x %d = %.2f MP, %s)"),
			m_info.m_dimensions.cx, m_info.m_dimensions.cy,
			m_info.GetMegaPixels(),
			num::FormatFileSize( m_info.m_fileSize ).c_str()
		) );

		textLayout.AddField( m_info.HasNavigInfo() ? str::Format( _T("[ %d / %d ]"), m_info.m_navigPos + 1, m_info.m_navigCount ) : str::GetEmpty() );
		textLayout.AddField( str::Format( _T("%d %%"), m_zoomPct ) );

		const D2D_SIZE_F maxSize = ToSizeF( ui::GetScreenSize() );		// start with the entire screen

	    m_pInfoTextLayout = dw::CreateTextLayout( textLayout.GetFullText(), m_pInfoFont, maxSize );
		if ( m_pInfoTextLayout != NULL )
		{	// apply text effects
			HR_VERIFY( m_pInfoTextLayout->SetDrawingEffect( m_pDimensionsBrush, textLayout.GetFieldRangeAt( Dimensions ) ) );
			HR_VERIFY( m_pInfoTextLayout->SetFontWeight( DWRITE_FONT_WEIGHT_NORMAL, textLayout.GetFieldRangeAt( Dimensions ) ) );
			HR_VERIFY( m_pInfoTextLayout->SetDrawingEffect( m_pNavigBrush, textLayout.GetFieldRangeAt( NavigCounts ) ) );
			HR_VERIFY( m_pInfoTextLayout->SetFontWeight( DWRITE_FONT_WEIGHT_NORMAL, textLayout.GetFieldRangeAt( ZoomPercent ) ) );
		}

		if ( m_info.IsMultiFrameImage() )
			m_pFrameTextLayout = dw::CreateTextLayout( str::Format( _T("Page: %d of %d"), m_info.m_framePos + 1, m_info.m_frameCount ), m_pInfoFont, maxSize );
		else
			m_pFrameTextLayout = NULL;

		return m_pInfoTextLayout != NULL;
	}

	bool CImageInfoGadget::IsValid( void ) const
	{
		return m_info.IsValid() && m_pInfoTextLayout != NULL;
	}

	void CImageInfoGadget::Draw( const CViewCoords& coords )
	{
		if ( !IsValid() )
			return;

		ID2D1RenderTarget* pRenderTarget = GetHostRenderTarget();
		ASSERT_PTR( pRenderTarget );
		ASSERT_PTR( m_pInfoTextLayout );

		pRenderTarget->SetTransform( D2D1::IdentityMatrix() );

		enum Metrics { EdgeH = 6, EdgeV = 3 };

		CSize textSize = FromSizeF( dw::ComputeTextSize( m_pInfoTextLayout ) );

		CRect textRect = coords.m_clientRect;
		textRect.DeflateRect( EdgeH, EdgeV );
		textRect.top = textRect.bottom - textSize.cy;
		textRect.right = textRect.left + textSize.cx;

		CRect backgroundRect = textRect;
		backgroundRect.InflateRect( 3, 1, 2, 2 );

		static const float s_roundRadius = 3.0f;
		D2D1_ROUNDED_RECT bkRoundRectF = { ToRectF( backgroundRect ), s_roundRadius, s_roundRadius };
		pRenderTarget->FillRoundedRectangle( bkRoundRectF, m_pBkBrush );

		D2D1_POINT_2F textOrigin = ToPointF( textRect.TopLeft() );
		pRenderTarget->DrawTextLayout( textOrigin, m_pInfoTextLayout, m_pTextBrush );				// e.g. "Iggy Pop.tif (1920 x 945 = 1.37 MP, 8.83 MB) [ 16 / 28 ] 63%"

		if ( m_pFrameTextLayout != NULL )
		{
			textOrigin.y = backgroundRect.top - ( dw::ComputeTextSize( m_pFrameTextLayout ).height + EdgeV );
			pRenderTarget->DrawTextLayout( textOrigin, m_pFrameTextLayout, m_pFrameTextBrush );		// e.g. "Page: 1 of 4"
		}
	}
}
