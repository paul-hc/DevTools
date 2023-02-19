
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds
#include "ImagingD2DTests.h"
#include "utl/UI/ImagingWic.h"
#include "utl/UI/ImagingDirect2D.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/WindowTimer.h"
#include "utl/UI/test/TestToolWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CImagingD2DTests::CImagingD2DTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CImagingD2DTests& CImagingD2DTests::Instance( void )
{
	static CImagingD2DTests s_testCase;
	return s_testCase;
}

void CImagingD2DTests::TestImage( ut::CTestDevice* pTestDev, d2d::CDCRenderTarget* pRenderTarget )
{
	fs::CFlexPath imagePath = MakeTestImageFilePath( Flamingos_jpg );
	if ( imagePath.IsEmpty() )
		return;

	pTestDev->SetSubTitle( _T("CImagingD2DTests::TestImage") );

	wic::CBitmapDecoder decoder( imagePath );
	CComPtr<IWICBitmapSource> pWicBitmap = decoder.ConvertFrameAt( 0 );

	pRenderTarget->SetWicBitmap( pWicBitmap );

	pTestDev->DrawBitmap( pRenderTarget, d2d::CDrawBitmapTraits( color::directx::LavenderBlush ), CSize( 200, 200 ) );
	pTestDev->DrawTileCaption( imagePath.GetFilename() );
	pTestDev->Await();
	++*pTestDev;
}

void CImagingD2DTests::TestImageEffects( ut::CTestDevice* pTestDev, d2d::CDCRenderTarget* pRenderTarget )
{
	// draw Dice.png with effects and transformations
	fs::CFlexPath imagePath = MakeTestImageFilePath( Dice_png );
	if ( imagePath.IsEmpty() )
		return;

	pTestDev->SetSubTitle( _T("CImagingD2DTests::TestImageEffects") );

	pRenderTarget->SetWicBitmap( wic::CBitmapDecoder( imagePath ).ConvertFrameAt( 0 ) );

	d2d::CDrawBitmapTraits traits( color::directx::LavenderBlush );
	traits.m_opacity = .25f;
	pTestDev->DrawBitmap( pRenderTarget, traits, CSize( 300, 300 ) );
	pTestDev->DrawTileCaption( imagePath.GetFilename() );
	pTestDev->Await();

	CPoint imageCenter = pTestDev->GetTileRect().CenterPoint();

	traits.m_transform = D2D1::Matrix3x2F::Rotation( 30.f, d2d::ToPointF( imageCenter ) );
	traits.m_opacity = .5f;
	traits.m_bkColor = CLR_NONE;
	pTestDev->DrawBitmap( pRenderTarget, traits, CSize( 300, 300 ) );
	pTestDev->DrawTileCaption( imagePath.GetFilename() );
	pTestDev->Await();

	traits.m_transform = D2D1::Matrix3x2F::Rotation( 60.f, d2d::ToPointF( imageCenter ) );
	traits.m_opacity = 1.f;
	pTestDev->DrawBitmap( pRenderTarget, traits, CSize( 300, 300 ) );
	pTestDev->DrawTileCaption( imagePath.GetFilename() );
	pTestDev->Await();

	++*pTestDev;
}

void CImagingD2DTests::TestImageAnimation( ut::CTestDevice* pTestDev, d2d::CDCRenderTarget* pRenderTarget )
{
	fs::CFlexPath imagePath = MakeTestImageFilePath( Animated_gif );
	if ( imagePath.IsEmpty() )
		return;

	pTestDev->SetSubTitle( _T("CImagingD2DTests::TestImageAnimation") );

	wic::CBitmapDecoder decoder( imagePath );
	d2d::CDrawBitmapTraits traits;

	for ( UINT count = 0; count != 3; ++count )
		for ( UINT framePos = 0; framePos != decoder.GetFrameCount(); ++framePos, pTestDev->Await( 20 ) )
		{
			pRenderTarget->SetWicBitmap( decoder.ConvertFrameAt( framePos ) );
			pTestDev->DrawBitmap( pRenderTarget, traits, CSize( 300, 300 ) );

			if ( 0 == count && 0 == framePos )
				pTestDev->DrawTileCaption( imagePath.GetFilename() );		// draw caption once for the first frame
		}

	++*pTestDev;
}

void CImagingD2DTests::Run( void )
{
	ut::CTestDevice testDev( ut::CTestToolWnd::AcquireWnd( 10 ), ut::TileDown );
	d2d::CDCRenderTarget renderTarget( testDev.GetDC() );

	RUN_TEST2( TestImage, &testDev, &renderTarget );
	RUN_TEST2( TestImageEffects, &testDev, &renderTarget );
	RUN_TEST2( TestImageAnimation, &testDev, &renderTarget );

	ut::CTestToolWnd::DisableEraseBk();
}


#endif //USE_UT
