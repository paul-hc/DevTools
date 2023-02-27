
#include "stdafx.h"
#include "CustomDrawImager.h"
#include "Thumbnailer.h"
#include "ImageStore.h"
#include "resource.h"
#include "utl/AppTools.h"
#include "utl/Algorithms.h"
#include "utl/ResourcePool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFileItemsThumbnailStore implementation

CFileItemsThumbnailStore::CFileItemsThumbnailStore( void )
{
}

CFileItemsThumbnailStore::~CFileItemsThumbnailStore()
{
}

CFileItemsThumbnailStore& CFileItemsThumbnailStore::Instance( void )
{
	static CFileItemsThumbnailStore s_thumbnailStore;
	return s_thumbnailStore;
}

int CFileItemsThumbnailStore::GetDefaultGlyphDimension( ui::GlyphGauge glyphGauge )
{
	switch ( glyphGauge )
	{
		default: ASSERT( false );
		case ui::SmallGlyph:	return CIconSize::GetSizeOf( SmallIcon ).cx;
		case ui::LargeGlyph:	return CIconSize::GetSizeOf( HugeIcon_48 ).cx;
	}
}

int CFileItemsThumbnailStore::GetGlyphDimension( ui::GlyphGauge glyphGauge ) const
{
	return GetThumbnailer( glyphGauge )->GetGlyphDimension();
}

bool CFileItemsThumbnailStore::SetGlyphDimension( ui::GlyphGauge glyphGauge, int glyphDimension )
{
	return GetThumbnailer( glyphGauge )->SetGlyphDimension( glyphDimension );
}

CSize CFileItemsThumbnailStore::GetGlyphSize( ui::GlyphGauge glyphGauge ) const
{
	const int glyphDimension = GetGlyphDimension( glyphGauge );
	return CSize( glyphDimension, glyphDimension );
}

CGlyphThumbnailer* CFileItemsThumbnailStore::GetThumbnailer( ui::GlyphGauge glyphGauge )
{
	std::auto_ptr<CGlyphThumbnailer>& rpThumbnailer = m_pThumbnailer[ glyphGauge ];

	if ( nullptr == rpThumbnailer.get() )
	{
		rpThumbnailer.reset( new CGlyphThumbnailer( GetDefaultGlyphDimension( glyphGauge ) ) );
		app::GetSharedResources().AddAutoPtr( &rpThumbnailer );		// will release the shared thumbnailer in ExitInstance()
	}

	ASSERT_PTR( rpThumbnailer.get() );
	return rpThumbnailer.get();
}

void CFileItemsThumbnailStore::RegisterControl( ICustomDrawControl* pCustomDrawCtrl )
{
	ASSERT_PTR( pCustomDrawCtrl );
	ASSERT( !utl::Contains( m_customDrawCtrls, pCustomDrawCtrl ) );

	m_customDrawCtrls.push_back( pCustomDrawCtrl );
}

void CFileItemsThumbnailStore::UnregisterControl( ICustomDrawControl* pCustomDrawCtrl )
{
	ASSERT_PTR( pCustomDrawCtrl );
	utl::RemoveExisting( m_customDrawCtrls, pCustomDrawCtrl );
}

void CFileItemsThumbnailStore::UpdateControls( void )
{
	for ( std::vector< ICustomDrawControl* >::const_iterator itCustomDrawCtrl = m_customDrawCtrls.begin(); itCustomDrawCtrl != m_customDrawCtrls.end(); ++itCustomDrawCtrl )
		if ( CBaseCustomDrawImager* pImager = ( *itCustomDrawCtrl )->GetCustomDrawImager() )
			if ( ui::ICustomImageDraw* pRenderer = pImager->GetRenderer() )
				if ( pRenderer == GetThumbnailer( ui::SmallGlyph ) || pRenderer == GetThumbnailer( ui::LargeGlyph ) )		// uses any of the file thumbnailers?
					( *itCustomDrawCtrl )->SetCustomFileGlyphDraw();			// update the image lists
}


// CBaseCustomDrawImager implementation

CBaseCustomDrawImager::~CBaseCustomDrawImager()
{
}

void CBaseCustomDrawImager::InitImageLists( const CSize& smallImageSize, const CSize& largeImageSize )
{
	ASSERT_NULL( m_imageLists[ ui::SmallGlyph ].GetSafeHandle() );
	ASSERT_NULL( m_imageLists[ ui::LargeGlyph ].GetSafeHandle() );
	REQUIRE( smallImageSize.cx > 0 && smallImageSize.cy > 0 );
	REQUIRE( largeImageSize.cx > 0 && largeImageSize.cy > 0 );

	m_imageLists[ ui::SmallGlyph ].Create( smallImageSize.cx, smallImageSize.cy, ILC_COLOR32 | ILC_MASK, 0, 1 );
	m_imageLists[ ui::LargeGlyph ].Create( largeImageSize.cx, largeImageSize.cy, ILC_COLOR32 | ILC_MASK, 0, 1 );

	// add transparent image entry to image lists
	const CIcon* pTranspIcon = ui::GetImageStoresSvc()->RetrieveIcon( ID_TRANSPARENT );
	ASSERT_PTR( pTranspIcon );

	m_transpImageIndex = m_imageLists[ ui::SmallGlyph ].Add( pTranspIcon->GetHandle() );
	m_imageLists[ ui::LargeGlyph ].Add( pTranspIcon->GetHandle() );
}

bool CBaseCustomDrawImager::DrawItemGlyph( const NMCUSTOMDRAW* pDraw, const CRect& itemImageRect )
{
	ui::ICustomImageDraw* pRenderer = GetRenderer();
	return pRenderer != nullptr && pRenderer->CustomDrawItemImage( pDraw, itemImageRect );
}


// CFileGlyphCustomDrawImager implementation

CFileGlyphCustomDrawImager::CFileGlyphCustomDrawImager( ui::GlyphGauge currGlyphGauge )
	: CBaseCustomDrawImager()
	, m_currGlyphGauge( currGlyphGauge )
{
	InitImageLists( CFileItemsThumbnailStore::Instance().GetGlyphSize( ui::SmallGlyph ),
					CFileItemsThumbnailStore::Instance().GetGlyphSize( ui::LargeGlyph ) );
}

ui::ICustomImageDraw* CFileGlyphCustomDrawImager::GetRenderer( void ) const
{
	return CFileItemsThumbnailStore::Instance().GetThumbnailer( m_currGlyphGauge );
}

bool CFileGlyphCustomDrawImager::SetCurrGlyphGauge( ui::GlyphGauge currGlyphGauge )
{
	m_currGlyphGauge = currGlyphGauge;
	return true;
}


// CSingleCustomDrawImager implementation

CSingleCustomDrawImager::CSingleCustomDrawImager( ui::ICustomImageDraw* pRenderer, const CSize& smallImageSize, const CSize& largeImageSize )
	: CBaseCustomDrawImager()
	, m_pRenderer( pRenderer )
	, m_ctrlDrivesBoundsSize( smallImageSize != CSize( 0, 0 ) ? true : false )				// image-lists size drive thumbnailer bounds size?
{
	ASSERT_PTR( m_pRenderer );

	m_imageSizes[ ui::SmallGlyph ] = m_ctrlDrivesBoundsSize ? smallImageSize : m_pRenderer->GetItemImageSize( ui::SmallGlyph );
	m_imageSizes[ ui::LargeGlyph ] = m_ctrlDrivesBoundsSize ? largeImageSize : m_pRenderer->GetItemImageSize( ui::LargeGlyph );

	InitImageLists( m_imageSizes[ ui::SmallGlyph ], m_imageSizes[ ui::LargeGlyph ] );
}

ui::ICustomImageDraw* CSingleCustomDrawImager::GetRenderer( void ) const
{
	return m_pRenderer;
}

bool CSingleCustomDrawImager::SetCurrGlyphGauge( ui::GlyphGauge currGlyphGauge )
{
	if ( m_ctrlDrivesBoundsSize )
		return m_pRenderer->SetItemImageSize( m_imageSizes[ currGlyphGauge ] );

	return true;
}
