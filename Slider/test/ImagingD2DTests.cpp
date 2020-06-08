
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "ImagingD2DTests.h"
#include "utl/UI/ImagingWic.h"
#include "utl/UI/ImagingDirect2D.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/WindowTimer.h"
#include "utl/UI/test/TestToolWnd.h"

#define new DEBUG_NEW


CImagingD2DTests::CImagingD2DTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CImagingD2DTests& CImagingD2DTests::Instance( void )
{
	static CImagingD2DTests testCase;
	return testCase;
}

void CImagingD2DTests::TestImage( ut::CTestDevice* pTestDev, d2d::CDCRenderTarget* pRenderTarget )
{
	fs::CFlexPath imagePath = MakeTestImageFilePath( Flamingos_jpg );
	if ( imagePath.IsEmpty() )
		return;

	wic::CBitmapDecoder decoder( imagePath );
	CComPtr< IWICBitmapSource > pWicBitmap = decoder.ConvertFrameAt( 0 );

	pRenderTarget->SetWicBitmap( pWicBitmap );

	pTestDev->DrawBitmap( pRenderTarget, d2d::CDrawBitmapTraits( color::directx::LavenderBlush ), CSize( 200, 200 ) );
	++*pTestDev;
}

void CImagingD2DTests::TestImageEffects( ut::CTestDevice* pTestDev, d2d::CDCRenderTarget* pRenderTarget )
{
	// draw Dice.png with effects and transformations
	fs::CFlexPath imagePath = MakeTestImageFilePath( Dice_png );
	if ( imagePath.IsEmpty() )
		return;

	pRenderTarget->SetWicBitmap( wic::CBitmapDecoder( imagePath ).ConvertFrameAt( 0 ) );
	enum { PauseTime = 250 };

	d2d::CDrawBitmapTraits traits( color::directx::LavenderBlush );
	traits.m_opacity = .25f;
	pTestDev->DrawBitmap( pRenderTarget, traits, CSize( 300, 300 ) );
	::Sleep( PauseTime );

	CPoint imageCenter = pTestDev->GetTileRect().CenterPoint();

	traits.m_transform = D2D1::Matrix3x2F::Rotation( 30.f, d2d::ToPointF( imageCenter ) );
	traits.m_opacity = .5f;
	traits.m_bkColor = CLR_NONE;
	pTestDev->DrawBitmap( pRenderTarget, traits, CSize( 300, 300 ) );
	::Sleep( PauseTime );

	traits.m_transform = D2D1::Matrix3x2F::Rotation( 60.f, d2d::ToPointF( imageCenter ) );
	traits.m_opacity = 1.f;
	pTestDev->DrawBitmap( pRenderTarget, traits, CSize( 300, 300 ) );
	::Sleep( PauseTime );
	++*pTestDev;
}

void CImagingD2DTests::TestImageAnimation( ut::CTestDevice* pTestDev, d2d::CDCRenderTarget* pRenderTarget )
{
	fs::CFlexPath imagePath = MakeTestImageFilePath( Animated_gif );
	if ( imagePath.IsEmpty() )
		return;

	wic::CBitmapDecoder decoder( imagePath );
	d2d::CDrawBitmapTraits traits;

	for ( UINT count = 0; count != 3; ++count )
		for ( UINT framePos = 0; framePos != decoder.GetFrameCount(); ++framePos, ::Sleep( 20 ) )
		{
			pRenderTarget->SetWicBitmap( decoder.ConvertFrameAt( framePos ) );
			pTestDev->DrawBitmap( pRenderTarget, traits, CSize( 300, 300 ) );
		}

	++*pTestDev;
}

void CImagingD2DTests::Run( void )
{
	__super::Run();

	ut::CTestDevice testDev( ut::CTestToolWnd::AcquireWnd( 10 ) );
	testDev.GotoOrigin();
	d2d::CDCRenderTarget renderTarget( testDev.GetDC() );

	TestImage( &testDev, &renderTarget );
	TestImageEffects( &testDev, &renderTarget );
	TestImageAnimation( &testDev, &renderTarget );
}


#endif //_DEBUG
