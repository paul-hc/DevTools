#ifndef RenderingDirect2D_h
#define RenderingDirect2D_h
#pragma once

#include "Direct2D.h"


namespace d2d
{
	// object creation

	CComPtr<ID2D1SolidColorBrush> CreateSolidBrush( ID2D1RenderTarget* pRenderTarget, const D2D1_COLOR_F& solidColor );
	bool CreateAsSolidBrush( CComPtr<ID2D1Brush>& rpBrush, ID2D1RenderTarget* pRenderTarget, const D2D1_COLOR_F& solidColor );

	CComPtr<ID2D1GradientStopCollection> CreateGradientStops( ID2D1RenderTarget* pRenderTarget, const D2D1_COLOR_F& fromColor, const D2D1_COLOR_F& toColor );
	CComPtr<ID2D1GradientStopCollection> CreateGradientStops( ID2D1RenderTarget* pRenderTarget, const D2D1_COLOR_F colors[], size_t count );
	CComPtr<ID2D1GradientStopCollection> CreateReverseGradientStops( ID2D1RenderTarget* pRenderTarget, const ID2D1GradientStopCollection* pSrcGradientStops );

	CComPtr<ID2D1LinearGradientBrush> CreateLinearGradientBrush( ID2D1RenderTarget* pRenderTarget, ID2D1GradientStopCollection* pGradientStops );
	CComPtr<ID2D1RadialGradientBrush> CreateRadialGradientBrush( ID2D1RenderTarget* pRenderTarget, ID2D1GradientStopCollection* pGradientStops );

	CComPtr<ID2D1LinearGradientBrush> CreateLinearGradientBrush( ID2D1RenderTarget* pRenderTarget, const D2D1_COLOR_F& fromColor, const D2D1_COLOR_F& toColor );
	CComPtr<ID2D1RadialGradientBrush> CreateRadialGradientBrush( ID2D1RenderTarget* pRenderTarget, const D2D1_COLOR_F& fromColor, const D2D1_COLOR_F& toColor );


	template< typename IteratorT >
	void ReverseGradientStops( IteratorT itFirst, IteratorT itLast )
	{
		for ( ; itFirst < itLast; ++itFirst )
			std::swap( itFirst->color, ( --itLast )->color );		// just reverse colors, keep positions (assuming they are evenly spread)
	}
}


namespace d2d
{
	// geometry

	CComPtr<ID2D1PathGeometry> CreatePolygonGeometry( const POINT points[], size_t count, bool filled = true );
	CComPtr<ID2D1PathGeometry> CreateTriangleGeometry( const POINT& point1, const POINT& point2, const POINT& point3 );

	CComPtr<ID2D1PathGeometry> CreateCombinedGeometries( ID2D1Geometry* geometries[], size_t count, D2D1_COMBINE_MODE combineMode );
	CComPtr<ID2D1PathGeometry> CreateCombinedGeometries( ID2D1Geometry* pGeometry1, ID2D1Geometry* pGeometry2, D2D1_COMBINE_MODE combineMode );

	CComPtr<ID2D1PathGeometry> CreateFrameGeometry( ID2D1RenderTarget* pRenderTarget, const RECT& boundsRect, int frameSize );
}


namespace d2d
{
	// rendering

	void DrawOutlineFrame( ID2D1RenderTarget* pRenderTarget, ID2D1Brush* pBrush, const RECT& boundsRect, int frameSize );
	void DrawGradientFrame( ID2D1RenderTarget* pRenderTarget, ID2D1LinearGradientBrush* pGradientBrush, const RECT& boundsRect, int frameSize );	// w. angular corners gradient


	enum Direction
	{
		N_S, S_N, W_E, E_W,				// right angle
		NW_SE, NE_SW, SE_NW, SW_NE		// diagonal
	};

	void SetGradientBrushDirection( ID2D1LinearGradientBrush* pGradientBrush, const RECT& boundsRect, Direction direction );


	namespace impl
	{
		// rendering details

		void DrawGradientFrameEdges( ID2D1RenderTarget* pRenderTarget, ID2D1LinearGradientBrush* pGradientBrush, const RECT& boundsRect, int frameSize );
		void DrawGradientFrameCorners( ID2D1RenderTarget* pRenderTarget, ID2D1LinearGradientBrush* pGradientBrush, const RECT& boundsRect, int frameSize );

		enum EdgePole { N, S, E, W };
		enum EdgeCorner { NW, NE, SE, SW };

		void DrawGradientEdge( ID2D1RenderTarget* pRenderTarget, ID2D1LinearGradientBrush* pGradientBrush, const CRect& edgeRect, EdgePole frameEdge );

		void DrawGradientCorner( ID2D1RenderTarget* pRenderTarget, ID2D1LinearGradientBrush* pGradientBrush, const CRect& cornerRect, EdgeCorner frameCorner );
		void DrawGradientCornerTriangle( ID2D1RenderTarget* pRenderTarget, ID2D1LinearGradientBrush* pGradientBrush,
										 const POINT& point1, const POINT& point2, const POINT& point3, bool drawDiagEdge = false );	// assume {point1, point3} is the hypotenuse
	}
}


class CEnumTags;


namespace d2d
{
	enum FrameStyle { NoFrame, OutlineFrame, OutlineGradientFrame, GradientFrame, GradientFrameRadialCorners };

	const CEnumTags& GetTags_FrameStyle( void );


	interface IRenderFrame : public utl::IMemoryManaged
	{
		virtual bool IsValid( void ) const = 0;
		virtual void Create( ID2D1RenderTarget* pRenderTarget, ID2D1GradientStopCollection* pGradientStops ) = 0;
		virtual void Draw( ID2D1RenderTarget* pRenderTarget, const RECT& boundsRect ) = 0;
	};


	class CFrameGadget : public CGadgetBase
	{
	public:
		CFrameGadget( int frameSize, const D2D1_COLOR_F colors[], size_t count );
		~CFrameGadget();

		FrameStyle GetFrameStyle( void ) const { return m_frameStyle; }
		void SetFrameStyle( FrameStyle frameStyle ) { m_frameStyle = frameStyle; DiscardDeviceResources(); }

		int GetFrameSize( void ) const { return m_frameSize; }
		void SetFrameSize( int frameSize ) { m_frameSize = frameSize; DiscardDeviceResources(); }

		const std::vector< D2D1_COLOR_F >& GetColors( void ) const { return m_colors; }
		void SetColors( const D2D1_COLOR_F colors[], size_t count ) { m_colors.assign( colors, colors + count ); DiscardDeviceResources(); }

		// IDeviceComponent interface
		virtual void DiscardDeviceResources( void );
		virtual bool CreateDeviceResources( void );

		// IGadgetComponent interface
		virtual bool IsValid( void ) const;
		virtual void Draw( const CViewCoords& coords );

		IRenderFrame* GetRenderFrame( void ) const { return m_pRenderFrame.get(); }

		template< typename RenderFrameT >
		RenderFrameT* GetRenderFrameAs( void ) const { return checked_static_cast<RenderFrameT*>( m_pRenderFrame.get() ); }
	private:
		CRect MakeFrameRect( const CViewCoords& coords ) const;
	protected:
		int m_frameSize;
		std::vector< D2D1_COLOR_F > m_colors;
		FrameStyle m_frameStyle;
	private:
		std::auto_ptr<IRenderFrame> m_pRenderFrame;
	};


	abstract class CBaseRenderFrame : public IRenderFrame
	{
	protected:
		CBaseRenderFrame( int frameSize ) : m_frameSize( frameSize ) {}
	protected:
		int m_frameSize;
	};


	class COutlineFrame : public CBaseRenderFrame
	{
	public:
		COutlineFrame( int frameSize ) : CBaseRenderFrame( frameSize ), m_gradientDirection( NW_SE ) {}

		void CreateSolidBrush( ID2D1RenderTarget* pRenderTarget, const D2D1_COLOR_F& solidColor );

		// gradient outline
		static CComPtr<ID2D1GradientStopCollection> MakeMirrorGradientStops( ID2D1RenderTarget* pRenderTarget, const std::vector< D2D1_COLOR_F >& srcColors );

		Direction GetGradientDirection( void ) const { return m_gradientDirection; }
		void SetGradientDirection( Direction gradientDirection ) { m_gradientDirection = gradientDirection; }

		// IRenderFrame interface
		virtual bool IsValid( void ) const;
		virtual void Create( ID2D1RenderTarget* pRenderTarget, ID2D1GradientStopCollection* pGradientStops );
		virtual void Draw( ID2D1RenderTarget* pRenderTarget, const RECT& boundsRect );
	private:
		Direction m_gradientDirection;
		CComPtr<ID2D1Brush> m_pFrameBrush;
	};


	class CGradientFrame : public CBaseRenderFrame
	{
	public:
		CGradientFrame( int frameSize ) : CBaseRenderFrame( std::max( frameSize, 3 ) ) {}

		// IRenderFrame interface
		virtual bool IsValid( void ) const;
		virtual void Create( ID2D1RenderTarget* pRenderTarget, ID2D1GradientStopCollection* pGradientStops );
		virtual void Draw( ID2D1RenderTarget* pRenderTarget, const RECT& boundsRect );
	private:
		CComPtr<ID2D1LinearGradientBrush> m_pGradientBrush;
	};


	class CRadialCornersFrame : public CBaseRenderFrame
	{
	public:
		CRadialCornersFrame( int frameSize ) : CBaseRenderFrame( std::max( frameSize, 3 ) ) {}

		// IRenderFrame interface
		virtual bool IsValid( void ) const;
		virtual void Create( ID2D1RenderTarget* pRenderTarget, ID2D1GradientStopCollection* pGradientStops );
		virtual void Draw( ID2D1RenderTarget* pRenderTarget, const RECT& boundsRect );
	private:
		CComPtr<ID2D1LinearGradientBrush> m_pEdgeBrush;			// for edges
		CComPtr<ID2D1RadialGradientBrush> m_pCornerBrush;			// for corners (inverted colours)
	};
}


#endif // RenderingDirect2D_h
