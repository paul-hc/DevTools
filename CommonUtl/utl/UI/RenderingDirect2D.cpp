
#include "stdafx.h"
#include "RenderingDirect2D.h"
#include "GdiCoords.h"
#include "utl/EnumTags.h"
#include "utl/Range.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace d2d
{
	CComPtr<ID2D1SolidColorBrush> CreateSolidBrush( ID2D1RenderTarget* pRenderTarget, const D2D1_COLOR_F& solidColor )
	{
		ASSERT_PTR( pRenderTarget );

		CComPtr<ID2D1SolidColorBrush> pSolidBrush;
		if ( HR_OK( pRenderTarget->CreateSolidColorBrush( &solidColor, NULL, &pSolidBrush ) ) )
			return pSolidBrush;

		return NULL;
	}

	bool CreateAsSolidBrush( CComPtr<ID2D1Brush>& rpBrush, ID2D1RenderTarget* pRenderTarget, const D2D1_COLOR_F& solidColor )
	{
		ASSERT_PTR( pRenderTarget );
		return HR_OK( pRenderTarget->CreateSolidColorBrush( &solidColor, NULL, reinterpret_cast<ID2D1SolidColorBrush**>( &rpBrush ) ) );
	}

	CComPtr<ID2D1GradientStopCollection> CreateGradientStops( ID2D1RenderTarget* pRenderTarget, const D2D1_COLOR_F& fromColor, const D2D1_COLOR_F& toColor )
	{
		ASSERT_PTR( pRenderTarget );

		D2D1_GRADIENT_STOP gradientStops[ 2 ];

		gradientStops[ 0 ].position = 0.0f;
		gradientStops[ 0 ].color = fromColor;
		gradientStops[ 1 ].position = 1.0f;
		gradientStops[ 1 ].color = toColor;

		CComPtr<ID2D1GradientStopCollection> pGradientStopCollection;

		if ( HR_OK( pRenderTarget->CreateGradientStopCollection( ARRAY_PAIR( gradientStops ), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &pGradientStopCollection ) ) )
			return pGradientStopCollection;

		return NULL;
	}

	CComPtr<ID2D1GradientStopCollection> CreateGradientStops( ID2D1RenderTarget* pRenderTarget, const D2D1_COLOR_F colors[], size_t count )
	{
		ASSERT_PTR( pRenderTarget );
		ASSERT( count > 1 );

		std::vector< D2D1_GRADIENT_STOP > gradientStops( count );
		gradientStops[ 0 ].position = 0.0f;
		gradientStops[ 0 ].color = colors[ 0 ];

		for ( size_t i = 1, divider = count-1; i != count; ++i )
		{
			gradientStops[ i ].position = (float)i / divider;
			gradientStops[ i ].color = colors[ i ];
		}

		CComPtr<ID2D1GradientStopCollection> pGradientStopCollection;

		if ( HR_OK( pRenderTarget->CreateGradientStopCollection( ARRAY_PAIR_V( gradientStops ), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &pGradientStopCollection ) ) )
			return pGradientStopCollection;

		return NULL;
	}

	CComPtr<ID2D1GradientStopCollection> CreateReverseGradientStops( ID2D1RenderTarget* pRenderTarget, const ID2D1GradientStopCollection* pSrcGradientStops )
	{
		ASSERT_PTR( pRenderTarget );
		ASSERT_PTR( pSrcGradientStops );

		UINT count = pSrcGradientStops->GetGradientStopCount();
		std::vector< D2D1_GRADIENT_STOP > gradientStops( count );
		pSrcGradientStops->GetGradientStops( ARRAY_PAIR_V( gradientStops ) );
		ReverseGradientStops( gradientStops.begin(), gradientStops.end() );			// reverse colour order

		D2D1_GAMMA gamma = pSrcGradientStops->GetColorInterpolationGamma();
		D2D1_EXTEND_MODE extendMode = pSrcGradientStops->GetExtendMode();

		CComPtr<ID2D1GradientStopCollection> pGradientStopCollection;

		if ( HR_OK( pRenderTarget->CreateGradientStopCollection( ARRAY_PAIR_V( gradientStops ), gamma, extendMode, &pGradientStopCollection ) ) )
			return pGradientStopCollection;

		return NULL;
	}


	CComPtr<ID2D1LinearGradientBrush> CreateLinearGradientBrush( ID2D1RenderTarget* pRenderTarget, ID2D1GradientStopCollection* pGradientStops )
	{
		ASSERT_PTR( pRenderTarget );
		ASSERT_PTR( pGradientStops );

		D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES gradBrushProperties = {};
		CComPtr<ID2D1LinearGradientBrush> pBrush;

		if ( HR_OK( pRenderTarget->CreateLinearGradientBrush( &gradBrushProperties, NULL, pGradientStops, &pBrush ) ) )
			return pBrush;

		return NULL;
	}

	CComPtr<ID2D1RadialGradientBrush> CreateRadialGradientBrush( ID2D1RenderTarget* pRenderTarget, ID2D1GradientStopCollection* pGradientStops )
	{
		ASSERT_PTR( pRenderTarget );
		ASSERT_PTR( pGradientStops );

		D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES radialGradientBrushProperties = {};
		CComPtr<ID2D1RadialGradientBrush> pBrush;

		if ( HR_OK( pRenderTarget->CreateRadialGradientBrush( &radialGradientBrushProperties, NULL, pGradientStops, &pBrush ) ) )
			return pBrush;

		return NULL;
	}

	CComPtr<ID2D1LinearGradientBrush> CreateLinearGradientBrush( ID2D1RenderTarget* pRenderTarget, const D2D1_COLOR_F& fromColor, const D2D1_COLOR_F& toColor )
	{
		if ( CComPtr<ID2D1GradientStopCollection> pGradientStops = CreateGradientStops( pRenderTarget, fromColor, toColor ) )
			return CreateLinearGradientBrush( pRenderTarget, pGradientStops );

		return NULL;
	}

	CComPtr<ID2D1RadialGradientBrush> CreateRadialGradientBrush( ID2D1RenderTarget* pRenderTarget, const D2D1_COLOR_F& fromColor, const D2D1_COLOR_F& toColor )
	{
		if ( CComPtr<ID2D1GradientStopCollection> pGradientStops = CreateGradientStops( pRenderTarget, fromColor, toColor ) )
			return CreateRadialGradientBrush( pRenderTarget, pGradientStops );

		return NULL;
	}

} //namespace d2d


namespace d2d
{
	CComPtr<ID2D1PathGeometry> CreatePolygonGeometry( const POINT points[], size_t count, bool filled /*= true*/ )
	{
		ASSERT_PTR( points );
		ASSERT( count > 1 );

		ID2D1Factory* pFactory = CFactory::Factory();

		CComPtr<ID2D1PathGeometry> pPolygonGeometry;
		if ( HR_OK( pFactory->CreatePathGeometry( &pPolygonGeometry ) ) )
		{
			CComPtr<ID2D1GeometrySink> pSink;
			if ( HR_OK( pPolygonGeometry->Open( &pSink ) ) )
			{
				const POINT* pPoint = points;
				const POINT* pPointEnd = pPoint + count;

				pSink->BeginFigure( ToPointF( *pPoint ), filled ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW );

				while ( ++pPoint != pPointEnd )
					pSink->AddLine( ToPointF( *pPoint ) );

				pSink->EndFigure( filled ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN );

				if ( HR_OK( pSink->Close() ) )
					return pPolygonGeometry;
			}
		}
		return NULL;
	}

	CComPtr<ID2D1PathGeometry> CreateTriangleGeometry( const POINT& point1, const POINT& point2, const POINT& point3 )
	{
		const POINT points[] = { point1, point2, point3 };
		return CreatePolygonGeometry( ARRAY_PAIR( points ) );
	}


	CComPtr<ID2D1PathGeometry> CreateCombinedGeometries( ID2D1Geometry* geometries[], size_t count, D2D1_COMBINE_MODE combineMode )
	{
		ASSERT_PTR( geometries );
		ASSERT( count > 1 );

		CComPtr<ID2D1PathGeometry> pNewGeometry;
		if ( HR_OK( CFactory::Factory()->CreatePathGeometry( &pNewGeometry ) ) )
		{
			CComPtr<ID2D1GeometrySink> pSink;
			if ( HR_OK( pNewGeometry->Open( &pSink ) ) )
			{
				for ( size_t i = 0; i < count; ++i )
					HR_OK( geometries[ i ]->CombineWithGeometry( geometries[ i + 1 ], combineMode, NULL, NULL, pSink ) );

				if ( HR_OK( pSink->Close() ) )
					return pNewGeometry;
			}
		}
		return NULL;
	}

	CComPtr<ID2D1PathGeometry> CreateCombinedGeometries( ID2D1Geometry* pGeometry1, ID2D1Geometry* pGeometry2, D2D1_COMBINE_MODE combineMode )
	{
		ID2D1Geometry* geometries[] = { pGeometry1, pGeometry2 };
		return CreateCombinedGeometries( ARRAY_PAIR( geometries ), combineMode );
	}


	CComPtr<ID2D1PathGeometry> CreateFrameGeometry( const RECT& boundsRect, int frameSize )
	{
		CRect rect = boundsRect;

		ID2D1Factory* pFactory = CFactory::Factory();

		CComPtr<ID2D1RectangleGeometry> pOuterGeometry;
		if ( HR_OK( pFactory->CreateRectangleGeometry( ToRectF( rect ), &pOuterGeometry ) ) )
		{
			rect.DeflateRect( frameSize, frameSize );

			CComPtr<ID2D1RectangleGeometry> pInnerGeometry;
			if ( HR_OK( pFactory->CreateRectangleGeometry( ToRectF( rect ), &pInnerGeometry ) ) )
				return CreateCombinedGeometries( pOuterGeometry, pInnerGeometry, D2D1_COMBINE_MODE_EXCLUDE );
		}
		return NULL;
	}

} //namespace d2d


namespace d2d
{
	void DrawOutlineFrame( ID2D1RenderTarget* pRenderTarget, ID2D1Brush* pBrush, const RECT& boundsRect, int frameSize )
	{
		ASSERT_PTR( pRenderTarget );
		ASSERT_PTR( pBrush );

		CRect outlineRect = boundsRect;

		int halfFrameSize = frameSize / 2;
		if ( HasFlag( frameSize, 0x01 ) )		// even size?
			++halfFrameSize;					// fine adjust frame bounds for outline (due to inherent draw anti-aliasing)

		outlineRect.DeflateRect( halfFrameSize, halfFrameSize );
		pRenderTarget->DrawRectangle( d2d::ToRectF( outlineRect ), pBrush, (float)frameSize );		// draw the outline
	}

	void DrawGradientFrame( ID2D1RenderTarget* pRenderTarget, ID2D1LinearGradientBrush* pGradientBrush, const RECT& boundsRect, int frameSize )
	{
		impl::DrawGradientFrameEdges( pRenderTarget, pGradientBrush, boundsRect, frameSize );
		impl::DrawGradientFrameCorners( pRenderTarget, pGradientBrush, boundsRect, frameSize );
	}

	void SetGradientBrushDirection( ID2D1LinearGradientBrush* pGradientBrush, const RECT& boundsRect, Direction direction )
	{
		ASSERT_PTR( pGradientBrush );

		Range<POINT> pointRange( ui::TopLeft( boundsRect ) );

		switch ( direction )
		{
			// right angle
			case N_S:
			case S_N:
				pointRange.m_end.y = boundsRect.bottom;
				break;
			case W_E:
			case E_W:
				pointRange.m_end.x = boundsRect.right;
				break;
			// diagonal
			case NW_SE:
			case SE_NW:
				pointRange.m_end = ui::BottomRight( boundsRect );
				break;
			case NE_SW:
			case SW_NE:
				pointRange.SetRange( ui::TopRight( boundsRect ), ui::BottomLeft( boundsRect ) );
				break;
		}

		switch ( direction )
		{
			case S_N:
			case E_W:
			case SE_NW:
			case SW_NE:
				pointRange.SwapBounds();
				break;
		}

		pGradientBrush->SetStartPoint( ToPointF( pointRange.m_start ) );
		pGradientBrush->SetEndPoint( ToPointF( pointRange.m_end ) );
	}


	namespace impl
	{
		void DrawGradientFrameEdges( ID2D1RenderTarget* pRenderTarget, ID2D1LinearGradientBrush* pGradientBrush, const RECT& boundsRect, int frameSize )
		{
			{
				CRect hEdgeRect( boundsRect );
				hEdgeRect.bottom = hEdgeRect.top + frameSize;
				hEdgeRect.DeflateRect( frameSize, 0 );

				DrawGradientEdge( pRenderTarget, pGradientBrush, hEdgeRect, N );

				ui::AlignRect( hEdgeRect, boundsRect, V_AlignBottom );
				DrawGradientEdge( pRenderTarget, pGradientBrush, hEdgeRect, S );
			}

			{
				CRect vEdgeRect( boundsRect );
				vEdgeRect.right = vEdgeRect.left + frameSize;
				vEdgeRect.DeflateRect( 0, frameSize );

				DrawGradientEdge( pRenderTarget, pGradientBrush, vEdgeRect, W );

				ui::AlignRect( vEdgeRect, boundsRect, H_AlignRight );
				DrawGradientEdge( pRenderTarget, pGradientBrush, vEdgeRect, E );
			}
		}

		void DrawGradientFrameCorners( ID2D1RenderTarget* pRenderTarget, ID2D1LinearGradientBrush* pGradientBrush, const RECT& boundsRect, int frameSize )
		{
			CRect cornerRect( ui::TopLeft( boundsRect ), CSize( frameSize, frameSize ) );

			ui::AlignRect( cornerRect, boundsRect, H_AlignLeft );
			DrawGradientCorner( pRenderTarget, pGradientBrush, cornerRect, NW );

			ui::AlignRect( cornerRect, boundsRect, H_AlignRight );
			DrawGradientCorner( pRenderTarget, pGradientBrush, cornerRect, NE );

			ui::AlignRect( cornerRect, boundsRect, H_AlignLeft | V_AlignBottom );
			DrawGradientCorner( pRenderTarget, pGradientBrush, cornerRect, SW );

			ui::AlignRect( cornerRect, boundsRect, H_AlignRight );
			DrawGradientCorner( pRenderTarget, pGradientBrush, cornerRect, SE );
		}


		void DrawGradientEdge( ID2D1RenderTarget* pRenderTarget, ID2D1LinearGradientBrush* pGradientBrush, const CRect& edgeRect, EdgePole frameEdge )
		{
			ASSERT_PTR( pRenderTarget );

			Direction brushDirection = N_S;

			switch ( frameEdge )
			{
				case N: brushDirection = N_S; break;
				case S: brushDirection = S_N; break;
				case E: brushDirection = E_W; break;
				case W: brushDirection = W_E; break;
			}

			SetGradientBrushDirection( pGradientBrush, edgeRect, brushDirection );
			pRenderTarget->FillRectangle( ToRectF( edgeRect ), pGradientBrush );
		}


		void DrawGradientCorner( ID2D1RenderTarget* pRenderTarget, ID2D1LinearGradientBrush* pGradientBrush, const CRect& cornerRect, EdgeCorner frameCorner )
		{
			ASSERT_PTR( pRenderTarget );

			POINT corners[] = { ui::TopLeft( cornerRect ), ui::TopRight( cornerRect ), ui::BottomRight( cornerRect ), ui::BottomLeft( cornerRect ) };

			switch ( frameCorner )
			{
				case NW:
					SetGradientBrushDirection( pGradientBrush, cornerRect, N_S );
					DrawGradientCornerTriangle( pRenderTarget, pGradientBrush, corners[ NW ], corners[ NE ], corners[ SE ], true );

					SetGradientBrushDirection( pGradientBrush, cornerRect, W_E );
					DrawGradientCornerTriangle( pRenderTarget, pGradientBrush, corners[ NW ], corners[ SW ], corners[ SE ] );
					break;
				case NE:
					SetGradientBrushDirection( pGradientBrush, cornerRect, N_S );
					DrawGradientCornerTriangle( pRenderTarget, pGradientBrush, corners[ SW ], corners[ NW ], corners[ NE ], true );

					SetGradientBrushDirection( pGradientBrush, cornerRect, E_W );
					DrawGradientCornerTriangle( pRenderTarget, pGradientBrush, corners[ SW ], corners[ SE ], corners[ NE ] );
					break;
				case SE:
					SetGradientBrushDirection( pGradientBrush, cornerRect, E_W );
					DrawGradientCornerTriangle( pRenderTarget, pGradientBrush, corners[ NW ], corners[ NE ], corners[ SE ], true );

					SetGradientBrushDirection( pGradientBrush, cornerRect, S_N );
					DrawGradientCornerTriangle( pRenderTarget, pGradientBrush, corners[ NW ], corners[ SW ], corners[ SE ] );
					break;
				case SW:
					SetGradientBrushDirection( pGradientBrush, cornerRect, W_E );
					DrawGradientCornerTriangle( pRenderTarget, pGradientBrush, corners[ SW ], corners[ NW ], corners[ NE ], true );

					SetGradientBrushDirection( pGradientBrush, cornerRect, S_N );
					DrawGradientCornerTriangle( pRenderTarget, pGradientBrush, corners[ SW ], corners[ SE ], corners[ NE ] );
					break;
			}
		}

		void DrawGradientCornerTriangle( ID2D1RenderTarget* pRenderTarget, ID2D1LinearGradientBrush* pGradientBrush, const POINT& point1, const POINT& point2, const POINT& point3, bool drawDiagEdge /*= false*/ )
		{
			ASSERT_PTR( pRenderTarget );
			ASSERT( ui::DiagonalLine == ui::GetLineOrientation( point1, point3 ) );		// {point1, point3} is the hypotenuse?

			CComPtr<ID2D1PathGeometry> triangle = CreateTriangleGeometry( point1, point2, point3 );
			pRenderTarget->FillGeometry( triangle, pGradientBrush );

			if ( drawDiagEdge )
				pRenderTarget->DrawLine( ToPointF( point1 ), ToPointF( point3 ), pGradientBrush );		// cover the diagonal edge left by FillGeometry()
		}

	} //namespace impl

} //namespace d2d


namespace d2d
{
	const CEnumTags& GetTags_FrameStyle( void )
	{
		static const CEnumTags tags( _T("None, Outline Frame, Gradient Outline Frame, Gradient Frame, Gradient Frame with Radial Corners") );
		return tags;
	}


	// CFrameGadget implementation

	CFrameGadget::CFrameGadget( int frameSize, const D2D1_COLOR_F colors[], size_t count )
		: m_frameSize( frameSize )
		, m_colors( colors, colors + count )
		, m_frameStyle( NoFrame )
	{
	}

	CFrameGadget::~CFrameGadget()
	{
	}

	void CFrameGadget::DiscardDeviceResources( void )
	{
		m_pRenderFrame.reset();
	}

	bool CFrameGadget::CreateDeviceResources( void )
	{
		ID2D1RenderTarget* pRenderTarget = GetHostRenderTarget();

		CComPtr<ID2D1GradientStopCollection> pGradientStops;

		switch ( m_frameStyle )
		{
			case NoFrame:
				return true;
			case OutlineFrame:
				m_pRenderFrame.reset( new COutlineFrame( m_frameSize ) );
				GetRenderFrameAs<COutlineFrame>()->CreateSolidBrush( pRenderTarget, m_colors.front() );		// solid brush
				return m_pRenderFrame->IsValid();
			case OutlineGradientFrame:
				m_pRenderFrame.reset( new COutlineFrame( m_frameSize ) );
				pGradientStops = COutlineFrame::MakeMirrorGradientStops( pRenderTarget, m_colors );
				break;
			case GradientFrame:
				m_pRenderFrame.reset( new CGradientFrame( m_frameSize ) );
				break;
			case GradientFrameRadialCorners:
				m_pRenderFrame.reset( new CRadialCornersFrame( m_frameSize ) );
				break;
			default:
				ASSERT( false );
		}

		if ( NULL == pGradientStops )
			pGradientStops = CreateGradientStops( pRenderTarget, ARRAY_PAIR_V( m_colors ) );

		if ( pGradientStops != NULL )
			m_pRenderFrame->Create( pRenderTarget, pGradientStops );

		return m_pRenderFrame->IsValid();
	}

	bool CFrameGadget::IsValid( void ) const
	{
		return GetRenderFrame() != NULL && GetRenderFrame()->IsValid();
	}

	CRect CFrameGadget::MakeFrameRect( const CViewCoords& coords ) const
	{
		CRect drawRect = coords.m_contentRect;

		drawRect.InflateRect( m_frameSize, m_frameSize );			// try not to cover small images
		ui::EnsureVisibleRect( drawRect, coords.m_clientRect );
		return drawRect;
	}

	void CFrameGadget::Draw( const CViewCoords& coords )
	{
		if ( IsValid() )
			m_pRenderFrame->Draw( GetHostRenderTarget(), MakeFrameRect( coords ) );
	}


	// COutlineFrame implementation

	void COutlineFrame::CreateSolidBrush( ID2D1RenderTarget* pRenderTarget, const D2D1_COLOR_F& solidColor )
	{
		CreateAsSolidBrush( m_pFrameBrush, pRenderTarget, solidColor );
	}

	CComPtr<ID2D1GradientStopCollection> COutlineFrame::MakeMirrorGradientStops( ID2D1RenderTarget* pRenderTarget, const std::vector< D2D1_COLOR_F >& srcColors )
	{	// make a nicer looking gradient {to, ..., from, ..., to}
		if ( srcColors.size() > 1 )
		{
			std::vector< D2D1_COLOR_F > mirroredColors = srcColors;
			mirroredColors.insert( mirroredColors.end(), srcColors.rbegin() + 1, srcColors.rend() );

			return CreateGradientStops( pRenderTarget, ARRAY_PAIR_V( mirroredColors ) );
		}

		return CreateGradientStops( pRenderTarget, ARRAY_PAIR_V( srcColors ) );
	}

	bool COutlineFrame::IsValid( void ) const
	{
		return m_pFrameBrush != NULL;
	}

	void COutlineFrame::Create( ID2D1RenderTarget* pRenderTarget, ID2D1GradientStopCollection* pGradientStops )
	{
		m_pFrameBrush = CreateLinearGradientBrush( pRenderTarget, pGradientStops );
	}

	void COutlineFrame::Draw( ID2D1RenderTarget* pRenderTarget, const RECT& boundsRect )
	{
		if ( CComQIPtr<ID2D1LinearGradientBrush> pLinearGradientBrush = m_pFrameBrush.p )
			SetGradientBrushDirection( pLinearGradientBrush, boundsRect, m_gradientDirection );

		DrawOutlineFrame( pRenderTarget, m_pFrameBrush, boundsRect, m_frameSize );
	}


	// CGradientFrame implementation

	bool CGradientFrame::IsValid( void ) const
	{
		return m_pGradientBrush != NULL;
	}

	void CGradientFrame::Create( ID2D1RenderTarget* pRenderTarget, ID2D1GradientStopCollection* pGradientStops )
	{
		m_pGradientBrush = CreateLinearGradientBrush( pRenderTarget, pGradientStops );
	}

	void CGradientFrame::Draw( ID2D1RenderTarget* pRenderTarget, const RECT& boundsRect )
	{
		DrawGradientFrame( pRenderTarget, m_pGradientBrush, boundsRect, m_frameSize );
	}


	// CRadialCornersFrame implementation

	bool CRadialCornersFrame::IsValid( void ) const
	{
		return m_pEdgeBrush != NULL && m_pCornerBrush != NULL;
	}

	void CRadialCornersFrame::Create( ID2D1RenderTarget* pRenderTarget, ID2D1GradientStopCollection* pGradientStops )
	{
		m_pEdgeBrush = CreateLinearGradientBrush( pRenderTarget, pGradientStops );
		m_pCornerBrush = CreateRadialGradientBrush( pRenderTarget, CreateReverseGradientStops( pRenderTarget, pGradientStops ) );		// reversed colors
	}

	void CRadialCornersFrame::Draw( ID2D1RenderTarget* pRenderTarget, const RECT& boundsRect )
	{
		ASSERT_PTR( pRenderTarget );

		impl::DrawGradientFrameEdges( pRenderTarget, m_pEdgeBrush, boundsRect, m_frameSize );

		CRect cornerRect( ui::TopLeft( boundsRect ), CSize( m_frameSize, m_frameSize ) );

		m_pCornerBrush->SetRadiusX( (float)cornerRect.Width() );
		m_pCornerBrush->SetRadiusY( (float)cornerRect.Height() );

		m_pCornerBrush->SetCenter( ToPointF( ui::BottomRight( cornerRect ) ) );
		pRenderTarget->FillRectangle( ToRectF( cornerRect ), m_pCornerBrush );

		ui::AlignRect( cornerRect, boundsRect, H_AlignRight );
		m_pCornerBrush->SetCenter( ToPointF( ui::BottomLeft( cornerRect ) ) );
		pRenderTarget->FillRectangle( ToRectF( cornerRect ), m_pCornerBrush );

		ui::AlignRect( cornerRect, boundsRect, V_AlignBottom );
		m_pCornerBrush->SetCenter( ToPointF( ui::TopLeft( cornerRect ) ) );
		pRenderTarget->FillRectangle( ToRectF( cornerRect ), m_pCornerBrush );

		ui::AlignRect( cornerRect, boundsRect, H_AlignLeft );
		m_pCornerBrush->SetCenter( ToPointF( ui::TopRight( cornerRect ) ) );
		pRenderTarget->FillRectangle( ToRectF( cornerRect ), m_pCornerBrush );
	}

} //namespace d2d
