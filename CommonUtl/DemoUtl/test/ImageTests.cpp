
#include "stdafx.h"

#ifdef _DEBUG		// no UT code in release builds
#include "ImageTests.h"
#include "resource.h"
#include "utl/UI/DibDraw.h"
#include "utl/UI/DibSection.h"
#include "utl/UI/DibPixels.h"
#include "utl/UI/GroupIcon.h"
#include "utl/UI/Icon.h"
#include "utl/UI/ImageStore.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/resource.h"
#include "utl/UI/test/TestToolWnd.h"
#include <sstream>

#define new DEBUG_NEW


CImageTests::CImageTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );
}

CImageTests& CImageTests::Instance( void )
{
	static CImageTests testCase;
	return testCase;
}

void CImageTests::TestGroupIcon( void )
{
	{
		const CGroupIcon groupIcon( IDR_IMAGE_STRIP );
		ASSERT( !groupIcon.IsValid() );						// no such icon
	}

	{
		/* GroupIconDir resource order (IDR_MAINFRAME):
			m_images[0]: m_id=1,  1 bpp, 32x32
			m_images[1]: m_id=2,  4 bpp, 48x48
			m_images[2]: m_id=3,  4 bpp, 32x32
			m_images[3]: m_id=4,  8 bpp, 32x32
			m_images[4]: m_id=5, 32 bpp, 48x48
			m_images[5]: m_id=6, 32 bpp, 32x32
			m_images[6]: m_id=7, 32 bpp, 24x24
			m_images[7]: m_id=8, 32 bpp, 16x16
		*/

		const CGroupIcon groupIcon( IDR_MAINFRAME );

		ASSERT( groupIcon.ContainsSize( DefaultSize ) );
		ASSERT( groupIcon.ContainsSize( SmallIcon ) );
		ASSERT( groupIcon.ContainsSize( MediumIcon ) );
		ASSERT( groupIcon.ContainsSize( LargeIcon ) );
		ASSERT( groupIcon.ContainsSize( HugeIcon_48 ) );
		ASSERT( !groupIcon.ContainsSize( HugeIcon_256 ) );

		ASSERT( groupIcon.ContainsBpp( ILC_MASK ) );
		ASSERT( groupIcon.ContainsBpp( ILC_COLOR4 ) );
		ASSERT( groupIcon.ContainsBpp( ILC_COLOR8 ) );
		ASSERT( groupIcon.ContainsBpp( ILC_COLOR32 ) );

		ASSERT( groupIcon.Contains( ILC_MASK, LargeIcon ) );
		ASSERT( groupIcon.Contains( ILC_COLOR32, HugeIcon_48 ) );

		ASSERT_EQUAL( std::make_pair( ILC_COLOR32, SmallIcon ), groupIcon.FindSmallest() );
		ASSERT_EQUAL( std::make_pair( ILC_COLOR32, HugeIcon_48 ), groupIcon.FindLargest() );
	}

	{
		const CGroupIcon groupIcon( IDR_TEST_DOC_TYPE );

		ASSERT( groupIcon.ContainsSize( SmallIcon ) );
		ASSERT( !groupIcon.ContainsSize( MediumIcon ) );
		ASSERT( groupIcon.ContainsSize( LargeIcon ) );
		ASSERT( !groupIcon.ContainsSize( HugeIcon_48 ) );
		ASSERT( !groupIcon.ContainsSize( HugeIcon_256 ) );

		ASSERT_EQUAL( std::make_pair( ILC_COLOR4, SmallIcon ), groupIcon.FindSmallest() );
		ASSERT_EQUAL( std::make_pair( ILC_COLOR4, LargeIcon ), groupIcon.FindLargest() );
	}
}

void CImageTests::TestIcon( void )
{
	const CIcon* pIcon = CImageStore::GetSharedStore()->RetrieveIcon( ID_AUTO_TRANSP_TOOL );
	ASSERT_PTR( pIcon );
	CIconInfo info( pIcon->GetHandle() );
}

void CImageTests::TestImageList( void )
{
	enum { Image_Fill = 4, ImageCount };

	const CSize imageSize = CIconId::GetStdSize( SmallIcon );
	CImageList imageList;
	VERIFY( res::LoadImageList( imageList, IDR_IMAGE_STRIP, ImageCount, imageSize, color::Auto ) );
	ASSERT_EQUAL( ImageCount, imageList.GetImageCount() );

	IMAGEINFO imageInfo;
	VERIFY( imageList.GetImageInfo( Image_Fill, &imageInfo ) );
	ASSERT( !gdi::HasMask( imageList, Image_Fill ) );
	ASSERT( gdi::HasAlphaTransparency( imageList, Image_Fill ) );
	ASSERT_NULL( imageInfo.hbmMask );

	CBitmapInfo bmpInfo( imageInfo.hbmImage );
	ASSERT( bmpInfo.IsDibSection() );
	ASSERT( bmpInfo.HasAlphaChannel() );
	ASSERT( bmpInfo.Is32Bit() );

	// DDB icon: 32 bpp, alpha channel info is lost
	{
		CIcon icon( imageList.ExtractIcon( Image_Fill ) );
		CIconInfo iconInfo( icon.GetHandle() );
		CBitmapInfo iconBmpInfo( iconInfo.m_info.hbmColor );
		ASSERT( iconBmpInfo.IsValid() );
		ASSERT( !iconBmpInfo.IsDibSection() );
	}

	// drawing
	ut::CTestDevice testDev( 5, ut::TileRight );

	testDev.DrawImage( &imageList, Image_Fill );
	++testDev;

	// NB: for some reason imageInfo.hbmImage cannot be selected into a DC - most likely is kept selected into a cached DC by the system...
	CDibSection dupDib;
	dupDib.Copy( imageInfo.hbmImage );

	CRect rect;
	rect = CRect( testDev.GetDrawPos(), dupDib.GetSize() );
	dupDib.Draw( testDev.GetDC(), rect );
	testDev.StoreTileRect( rect );
	testDev.DrawTileFrame( color::AzureBlue );
	++testDev;


	CImageList disabledImageList;
	gdi::MakeDisabledImageList( disabledImageList, imageList );

	VERIFY( disabledImageList.GetImageInfo( Image_Fill, &imageInfo ) );
	dupDib.Copy( imageInfo.hbmImage );

	rect = CRect( testDev.GetDrawPos(), dupDib.GetSize() );
	dupDib.Draw( testDev.GetDC(), rect );
	testDev.StoreTileRect( rect );
	testDev.DrawTileFrame( color::AzureBlue );
	++testDev;

	testDev.DrawImageList( &disabledImageList, true );
	++testDev;

	// transparent icon
	const CIcon* pTranspIcon = CImageStore::SharedStore()->RetrieveIcon( ID_TRANSPARENT );
	testDev.DrawIcon( pTranspIcon->GetHandle(), pTranspIcon->GetSize() );		// there is one white pixel at right-bottom so that the icon is not completely black (GDI bug?)
	++testDev;

	// image list transparent image
	imageList.DeleteImageList();
	imageList.Create( imageSize.cx, imageSize.cy, ILC_COLOR32 | ILC_MASK, 0, 2 );
	imageList.Add( pTranspIcon->GetHandle() );				// one white pixel at right-bottom
	imageList.Add( NULL, CLR_NONE );						// another was to add an empty icon
	testDev.DrawImageList( &imageList, true );
}

void CImageTests::Run( void )
{
	__super::Run();

	TestGroupIcon();
	TestIcon();
	TestImageList();
}


#endif //_DEBUG
