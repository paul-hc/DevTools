#ifndef TestToolWnd_h
#define TestToolWnd_h
#pragma once

#ifdef _DEBUG		// no UT code in release builds

#include "ui_fwd.h"
#include "WindowTimer.h"


interface IWICBitmapSource;

namespace d2d
{
	struct CDrawBitmapTraits;
	class CRenderTarget;
}


namespace ut
{
	enum TileAlign { TileRight, TileDown };


	class CTestToolWnd : public CFrameWnd
	{
		CTestToolWnd( UINT elapseSelfDestroy );		// self-destroying singleton window
		virtual ~CTestToolWnd();
	public:
		static CTestToolWnd* AcquireWnd( UINT selfDestroySecs = 5 );

		// drawing pos iteration
		void ResetDrawPos( void );					// reset to origin
	private:
		static CRect MakeWindowRect( void );
		static const TCHAR* GetClassName( void );
	private:
		CWindowTimer m_destroyTimer;
		static CTestToolWnd* s_pWndTool;

		enum { DestroyTimerId = 1000, Width = 500, Height = 900 };
	public:
		CPoint m_drawPos;							// drawing cursor position in the current strip
	protected:
		// generated stuff
		afx_msg void OnTimer( UINT_PTR eventId );

		DECLARE_MESSAGE_MAP()
	};


	// use this to access test tool's client DC; null pattern: if NULL == pToolWnd -> no test output done.
	// structured in strips containing tiles
	//
	class CTestDevice
	{
	public:
		CTestDevice( CTestToolWnd* pToolWnd, TileAlign tileAlign = TileDown );
		CTestDevice( UINT selfDestroySecs, TileAlign tileAlign = TileDown );
		~CTestDevice();

		bool IsEnabled( void ) const { return m_pToolWnd != NULL; }
		CDC* GetDC( void ) const { return m_pTestDC.get(); }

		// drawing cursor
		const CPoint& GetDrawPos( void ) const { ASSERT( IsEnabled() ); return m_pToolWnd->m_drawPos; }
		void GotoOrigin( void );
		bool GotoNextTile( void );
		bool GotoNextStrip( void );
		bool CascadeNextTile( void );			// breaks to a new strip if the subsequent tile partially overflows; assumes next tile has the same size

		CTestDevice& operator++( void ) { GotoNextTile(); return *this; }

		const CRect& GetTileRect( void ) const { return m_tileRect; }
		void StoreTileRect( const CRect& tileRect );

		void DrawTileFrame( COLORREF frameColor = color::LightGrey, int outerEdge = 1 );
		void DrawTileFrame( const CRect& tileRect, COLORREF frameColor = color::LightGrey, int outerEdge = 1 );

		void DrawBitmap( HBITMAP hBitmap );
		void DrawBitmap( HBITMAP hBitmap, const CSize& boundsSize, CDC* pSrcDC = NULL );
		void DrawBitmap( IWICBitmapSource* pWicBitmap, const CSize& boundsSize );
		void DrawBitmap( d2d::CRenderTarget* pBitmapRT, const d2d::CDrawBitmapTraits& traits, const CSize& boundsSize );
		void DrawIcon( HICON hIcon, const CSize& boundsSize, UINT flags = DI_NORMAL );
		void DrawImage( CImageList* pImageList, int index, UINT style = ILD_TRANSPARENT );
		void DrawImageList( CImageList* pImageList, bool putTags = false, UINT style = ILD_TRANSPARENT );

		void DrawTextInfo( const std::tstring& text );
		void DrawBitmapInfo( HBITMAP hBitmap );
	private:
		static COLORREF GetBitmapFrameColor( const CSize& bmpSize, const CSize& scaledBmpSize );
	private:
		void Construct( CTestToolWnd* pToolWnd );
	private:
		TileAlign m_tileAlign;
		CTestToolWnd* m_pToolWnd;
		std::auto_ptr< CDC > m_pTestDC;

		CRect m_workAreaRect;					// client rect shrunk by edge
		CRect m_stripRect;						// row of tiles
		CRect m_tileRect;						// set by current draw method
	public:
		static const CSize m_edgeSize;			// edge from the client rect
	};

} //namespace ut


struct CBitmapInfo;


namespace ut
{
	std::tostream& operator<<( std::tostream& os, const CBitmapInfo& bmpInfo );
}


#endif //_DEBUG


#endif // TestToolWnd_h
