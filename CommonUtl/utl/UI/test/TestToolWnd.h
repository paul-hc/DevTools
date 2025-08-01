#ifndef TestToolWnd_h
#define TestToolWnd_h
#pragma once

#ifdef USE_UT		// no UT code in release builds

#include "UI/ui_fwd.h"
#include "UI/WindowTimer.h"


interface IWICBitmapSource;
class CMFCToolBarImages;
class CIcon;

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
		static void DisableEraseBk( void );

		// drawing pos iteration
		void ResetDrawPos( void );					// reset to origin
	private:
		static CRect MakeWindowRect( void );
		static const TCHAR* GetClassName( void );
	private:
		CWindowTimer m_destroyTimer;
		bool m_disableEraseBk;						// hack

		static CTestToolWnd* s_pWndTool;

		enum { DestroyTimerId = 1000, Width = 500, Height = 900 };
	public:
		CPoint m_drawPos;							// drawing cursor position in the current strip

		CFont m_headlineFont;						// larger bold font for titles

		// generated stuff
	protected:
		afx_msg void OnTimer( UINT_PTR eventId );
		afx_msg BOOL OnEraseBkgnd( CDC* pDC );

		DECLARE_MESSAGE_MAP()
	};


	// use this to access test tool's client DC; null pattern: if nullptr == pToolWnd -> no test output done.
	// structured in strips containing tiles
	//
	class CTestDevice
	{
	public:
		CTestDevice( CTestToolWnd* pToolWnd, TileAlign tileAlign = ut::TileRight );
		CTestDevice( UINT selfDestroySecs, TileAlign tileAlign = ut::TileRight );
		~CTestDevice();

		bool IsEnabled( void ) const { return m_pToolWnd != nullptr; }
		CDC* GetDC( void ) const { ASSERT_PTR( m_pTestDC.get() ); return m_pTestDC.get(); }
		CTestToolWnd* GetToolWnd( void ) const { ASSERT_PTR( m_pToolWnd ); return m_pToolWnd; }

		void SetTileAlign( TileAlign tileAlign );

		void SetSubTitle( const TCHAR* pSubTitle );		// augment test window title with the sub-title

		// drawing cursor
		const CPoint& GetDrawPos( void ) const { ASSERT( IsEnabled() ); return m_pToolWnd->m_drawPos; }
		void ResetOrigin( void );
		bool GotoNextTile( void );
		bool GotoNextStrip( void );
		bool CascadeNextTile( void );			// breaks to a new strip if the subsequent tile partially overflows; assumes next tile has the same size

		CTestDevice& operator++( void ) { GotoNextTile(); return *this; }

		const CRect& GetTileRect( void ) const { return m_tileRect; }
		void StoreTileRect( const CRect& tileRect );

		void DrawTileFrame( COLORREF frameColor = color::LightGray, int outerEdge = 1 );
		void DrawTileFrame( const CRect& tileRect, COLORREF frameColor = color::LightGray, int outerEdge = 1 );

		void DrawBitmap( HBITMAP hBitmap );
		void DrawBitmap( HBITMAP hBitmap, const CSize& boundsSize, CDC* pSrcDC = nullptr );
		void DrawBitmap( IWICBitmapSource* pWicBitmap, const CSize& boundsSize );
		void DrawBitmap( d2d::CRenderTarget* pBitmapRT, const d2d::CDrawBitmapTraits& traits, const CSize& boundsSize );
		void DrawIcon( HICON hIcon, const CSize& boundsSize, UINT flags = DI_NORMAL );
		void DrawIcon( const CIcon* pIcon, bool enabled = true );
		void DrawImage( CImageList* pImageList, int index, UINT style = ILD_TRANSPARENT );
		void DrawImageList( CImageList* pImageList, bool putTags = false, UINT style = ILD_TRANSPARENT );

		bool DrawWideBitmap( HBITMAP hBitmap, const CSize& glyphSize );		// on multiple rows
		void DrawImages( CMFCToolBarImages* pImages );
		void DrawImagesDetails( CMFCToolBarImages* pImages );

		void DrawTextInfo( const std::tstring& text );
		void DrawBitmapInfo( HBITMAP hBitmap );

		void DrawHeadline( const TCHAR* pHeadline, COLORREF textColor = color::Blue );	// draw headline title on a new strip, under the last drawn tile rect
		void DrawTileCaption( const std::tstring& text, bool ellipsys = false );		// draw text under the last drawn tile rect

		enum { PauseTime = 500 };

		void Await( DWORD milliseconds = PauseTime ) { ::Sleep( milliseconds ); }
	private:
		void Construct( CTestToolWnd* pToolWnd );
		CRect GetStripTotalRect( void ) const;

		static COLORREF GetBitmapFrameColor( const CSize& bmpSize, const CSize& scaledBmpSize );
	private:
		TileAlign m_tileAlign;
		CTestToolWnd* m_pToolWnd;
		std::auto_ptr<CDC> m_pTestDC;

		CRect m_workAreaRect;					// client rect shrunk by edge
		CRect m_stripRect;						// row of tiles
		CRect m_tileRect;						// last drawn cursor rect, set by current draw method
		bool m_scatterCaption;					// alternate internal state to avoid overlapping text between subsequent calls to DrawTileCaption()
	public:
		static const CSize m_edgeSize;			// edge from the client rect
	};

} //namespace ut


struct CBitmapInfo;


namespace ut
{
	std::tostream& operator<<( std::tostream& os, const CBitmapInfo& bmpInfo );
}


#include "test/Test.h"


namespace ut
{
	class CScopedTestDeviceMethod : public CScopedTestMethod
	{
	public:
		CScopedTestDeviceMethod( const ut::ITestCase* pTestCase, const char* pTestMethod, ut::CTestDevice& rTestDev )
			: CScopedTestMethod( pTestCase, pTestMethod )
			, m_rTestDev( rTestDev )
		{
			m_rTestDev.DrawHeadline( m_testMethod.c_str() );
		}

		~CScopedTestDeviceMethod()
		{
			m_rTestDev.GotoNextStrip();
			m_rTestDev.Await( 1500 );
		}
	protected:
		ut::CTestDevice& m_rTestDev;
	};
}


#define RUN_TESTDEV_1( testMethod, rTestDevArg1 )\
	do { ut::CScopedTestDeviceMethod test( this, #testMethod, rTestDevArg1 ); (testMethod)( (rTestDevArg1) ); } while ( false )

#define RUN_TESTDEV_2( testMethod, rTestDevArg1, arg2 )\
	do { ut::CScopedTestDeviceMethod test( this, #testMethod, rTestDevArg1 ); (testMethod)( (rTestDevArg1), (arg2) ); } while ( false )


#endif //USE_UT


#endif // TestToolWnd_h
