
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "ImageTests.h"
#include "resource.h"
#include "utl/UI/DibDraw.h"
#include "utl/UI/DibSection.h"
#include "utl/UI/DibPixels.h"
#include "utl/UI/GroupIconRes.h"
#include "utl/UI/Icon.h"
#include "utl/UI/ImageStore.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/resource.h"
#include "utl/UI/test/TestToolWnd.h"
#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CImageTests::CImageTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );
}

CImageTests& CImageTests::Instance( void )
{
	static CImageTests s_testCase;
	return s_testCase;
}

void CImageTests::TestGroupIconRes( void )
{
	{
		const CGroupIconRes groupIcon( IDR_IMAGE_STRIP );
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

		const CGroupIconRes groupIcon( IDR_MAINFRAME );

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
		const CGroupIconRes groupIcon( IDR_TEST_DOC_TYPE );

		ASSERT( groupIcon.ContainsSize( SmallIcon ) );
		ASSERT( !groupIcon.ContainsSize( MediumIcon ) );
		ASSERT( groupIcon.ContainsSize( LargeIcon ) );
		ASSERT( !groupIcon.ContainsSize( HugeIcon_48 ) );
		ASSERT( !groupIcon.ContainsSize( HugeIcon_256 ) );

		ASSERT_EQUAL( std::make_pair( ILC_COLOR4, SmallIcon ), groupIcon.FindSmallest() );
		ASSERT_EQUAL( std::make_pair( ILC_COLOR4, LargeIcon ), groupIcon.FindLargest() );
	}
}

void CImageTests::TestIcon( ut::CTestDevice& rTestDev )
{
	const CIcon* pIcon = ui::GetImageStoresSvc()->RetrieveIcon( ID_AUTO_TRANSP_TOOL );
	ASSERT_PTR( pIcon );
	CIconInfo info( pIcon->GetHandle() );

	rTestDev.DrawIcon( pIcon );
}

void CImageTests::TestIconGroup( ut::CTestDevice& rTestDev )
{
}

void CImageTests::TestImageList( ut::CTestDevice& rTestDev )
{
	enum { Image_Fill = 4, ImageCount };

	CImageList imageList;

	VERIFY( res::LoadImageListDIB( imageList, IDR_IMAGE_STRIP ).m_imageCount != 0 );		// use default image count, implied from the width/height ratio
	CSize imageSize = gdi::GetImageIconSize( imageList );
	ASSERT_EQUAL( ImageCount, imageList.GetImageCount() );
	ASSERT( CIconSize::GetSizeOf( SmallIcon ) == imageSize );

	{	// reload with explicit image count
		CImageList imageList2;
		VERIFY( res::LoadImageListDIB( imageList2, IDR_IMAGE_STRIP, color::Auto, ImageCount ).m_imageCount != 0 );
		ASSERT_EQUAL( imageList.GetImageCount(), imageList2.GetImageCount() );
		ASSERT( imageSize == gdi::GetImageIconSize( imageList2 ) );
	}

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
	rTestDev.DrawImage( &imageList, Image_Fill );
	rTestDev.DrawTileCaption( _T("image-list IDR_IMAGE_STRIP") );
	++rTestDev;

	// NB: for some reason imageInfo.hbmImage cannot be selected into a DC - most likely is kept selected into a cached DC by the system...
	CDibSection dupDib;
	dupDib.Copy( imageInfo.hbmImage );

	CRect rect( rTestDev.GetDrawPos(), dupDib.GetSize() );
	dupDib.Draw( rTestDev.GetDC(), rect );
	rTestDev.StoreTileRect( rect );
	rTestDev.DrawTileFrame( color::AzureBlue );
	rTestDev.DrawTileCaption( _T("dupDib IDR_IMAGE_STRIP") );
	++rTestDev;


	CImageList disabledImageList;
	gdi::MakeDisabledImageList( disabledImageList, imageList );

	VERIFY( disabledImageList.GetImageInfo( Image_Fill, &imageInfo ) );
	dupDib.Copy( imageInfo.hbmImage );

	rect = CRect( rTestDev.GetDrawPos(), dupDib.GetSize() );
	dupDib.Draw( rTestDev.GetDC(), rect );
	rTestDev.StoreTileRect( rect );
	rTestDev.DrawTileFrame( color::AzureBlue );
	rTestDev.DrawTileCaption( _T("imageInfo.hbmImage IDR_IMAGE_STRIP") );
	++rTestDev;

	rTestDev.DrawImageList( &disabledImageList, true );
	rTestDev.DrawTileCaption( _T("disabledImageList IDR_IMAGE_STRIP") );
	++rTestDev;

	// transparent icon
	const CIcon* pTranspIcon = ui::GetImageStoresSvc()->RetrieveIcon( ID_TRANSPARENT );
	rTestDev.DrawIcon( pTranspIcon->GetHandle(), pTranspIcon->GetSize() );		// there is one white pixel at right-bottom so that the icon is not completely black (GDI bug?)
	rTestDev.DrawTileCaption( _T("icon IDR_IMAGE_STRIP") );
	++rTestDev;

	// image list transparent image
	imageList.DeleteImageList();
	imageList.Create( imageSize.cx, imageSize.cy, ILC_COLOR32 | ILC_MASK, 0, 2 );
	imageList.Add( pTranspIcon->GetHandle() );				// one white pixel at right-bottom
	imageList.Add( nullptr, CLR_NONE );						// another was to add an empty icon
	rTestDev.DrawImageList( &imageList, true );
	rTestDev.DrawTileCaption( _T("image-list IDR_IMAGE_STRIP") );
}


void CImageTests::Run( void )
{
	ut::CTestDevice testDev( ut::CTestToolWnd::AcquireWnd( 10 ) );
	testDev.SetSubTitle( _T("CImageTests") );

	RUN_TEST( TestGroupIconRes );
	RUN_TESTDEV_1( TestIcon, testDev );
	RUN_TESTDEV_1( TestIconGroup, testDev );
	RUN_TESTDEV_1( TestImageList, testDev );
}


#endif //USE_UT
