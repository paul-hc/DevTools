
#include "pch.h"
#include "WicImage.h"
#include "WicAnimatedImage.h"
#include "GdiCoords.h"
#include "StructuredStorage.h"
#include "StringUtilities.h"
#include "TimeUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const fs::TImagePathKey CWicImage::s_nullKey( fs::CFlexPath(), UINT_MAX );

CWicImage::CWicImage( const WICPixelFormatGUID* pCvtPixelFormat /*= &GUID_WICPixelFormat32bppPBGRA*/ )
	: CWicBitmap()
	, m_key( s_nullKey )
	, m_frameCount( 0 )
	, m_pCvtPixelFormat( pCvtPixelFormat )
	, m_pSharedDecoder( nullptr )
{
}

CWicImage::~CWicImage()
{
	if ( m_pSharedDecoder != nullptr )
		if ( !m_pSharedDecoder->UnloadFrame( this ) )						// no frames loaded anymore?
			VERIFY( SharedMultiFrameDecoders().Remove( m_key.first ) );		// removed the registered shared decoder
}

std::auto_ptr<CWicImage> CWicImage::CreateFromFile( const fs::TImagePathKey& imageKey, utl::ErrorHandling handlingMode /*= utl::CheckMode*/ )
{
	std::auto_ptr<CWicImage> pNewImage;

	TMultiFrameDecoderMap& rSharedDecoders = SharedMultiFrameDecoders();

	if ( CMultiFrameDecoder* pSharedDecoder = rSharedDecoders.Find( imageKey.first ) )
		return pSharedDecoder->LoadFrame( imageKey );

	wic::CBitmapDecoder decoder( imageKey.first, handlingMode );

	if ( decoder.IsValid() )
	{
		wic::TDecoderFlags decoderFlags = decoder.GetDecoderFlags();
		if ( HasFlag( decoderFlags, wic::CBitmapDecoder::MultiFrame ) )			// multi-frame image?
		{
			if ( HasFlag( decoderFlags, wic::CBitmapDecoder::Animation ) )		// multi-frame GIF with animation?
				pNewImage.reset( new CWicAnimatedImage( decoder ) );
			else
			{
				rSharedDecoders.Add( imageKey.first, CMultiFrameDecoder( decoder ) );
				return rSharedDecoders.Lookup( imageKey.first ).LoadFrame( imageKey );
			}
		}
	}
	if ( nullptr == pNewImage.get() )
		pNewImage.reset( new CWicImage() );

	if ( !pNewImage->LoadDecoderFrame( decoder, imageKey ) )
		pNewImage.reset();

	return pNewImage;
}

std::pair<UINT, wic::TDecoderFlags> CWicImage::LookupImageFileFrameCount( const fs::CFlexPath& imagePath )
{
	wic::CBitmapDecoder decoder = AcquireDecoder( imagePath, utl::CheckMode );
	return std::pair<UINT, wic::TDecoderFlags>( decoder.GetFrameCount(), decoder.GetDecoderFlags() );
}

void CWicImage::Clear( void )
{
	CWicBitmap::Clear();
	m_frameCount = 0;
	m_key = s_nullKey;
}

bool CWicImage::LoadDecoderFrame( wic::CBitmapDecoder& decoder, const fs::TImagePathKey& imageKey )
{
	Clear();
	m_key = imageKey;

	m_frameCount = decoder.GetFrameCount();

	if ( GetFramePos() < m_frameCount )
		if ( CComPtr<IWICBitmapSource> pFrameBitmap = decoder.ConvertFrameAt( GetFramePos(), m_pCvtPixelFormat ) )
			if ( StoreFrame( pFrameBitmap, GetFramePos() ) )
			{
				GetOrigin().SetOriginalPath( m_key.first );
				return true;
			}

	return false;
}

bool CWicImage::LoadFromFile( const fs::TImagePathKey& imageKey )
{
	wic::CBitmapDecoder decoder = AcquireDecoder( imageKey.first, GetHandlingMode() );
	return LoadDecoderFrame( decoder, imageKey );
}

void CWicImage::SetSharedDecoder( CMultiFrameDecoder* pSharedDecoder )
{
	m_pSharedDecoder = pSharedDecoder;
}

bool CWicImage::LoadFrame( UINT framePos )
{	// called from CWicImageTests only
	ASSERT( framePos < m_frameCount );
	return LoadFromFile( std::make_pair( GetImagePath(), framePos ) );
}

bool CWicImage::StoreFrame( IWICBitmapSource* pFrameBitmap, UINT framePos )
{
	framePos;
	return SetWicBitmap( pFrameBitmap );
}

bool CWicImage::IsAnimated( void ) const
{
	return false;
}

const CSize& CWicImage::GetBmpSize( void ) const
{
	return GetBmpFmt().m_size;
}

bool CWicImage::IsCorruptFile( const fs::CFlexPath& imagePath )
{
	if ( imagePath.IsEmpty() || !imagePath.FileExist( fs::Read ) )
		return false;				// file doesn't exist -> doesn't mean is corrupt!

	wic::CBitmapDecoder decoder = AcquireDecoder( imagePath, utl::CheckMode );
	if ( !decoder.IsValid() || 0 == decoder.GetFrameCount() )
		return false;

	static const CSize s_emptySize( 0, 0 );

	for ( UINT framePos = 0; framePos != decoder.GetFrameCount(); ++framePos )
		if ( s_emptySize == wic::GetBitmapSize( decoder.GetFrameAt( framePos ) ) )
			return false;			// frame is corrupted

	return true;
}

bool CWicImage::IsCorruptFrame( const fs::TImagePathKey& imageKey )
{
	if ( imageKey.first.IsEmpty() || !imageKey.first.FileExist( fs::Read ) )
		return false;				// file doesn't exist -> doesn't mean is corrupt!

	wic::CBitmapDecoder decoder = AcquireDecoder( imageKey.first, utl::CheckMode );

	if ( decoder.IsValid() )
		if ( CSize( 0, 0 ) == wic::GetBitmapSize( decoder.GetFrameAt( imageKey.second ) ) )
			return true;			// corrupt frame

	return false;					// we can't say -> doesn't mean is corrupt!
}

#ifdef _DEBUG

std::tstring CWicImage::FormatDbg( void ) const
{
	static const TCHAR sep[] = _T(" ");

	std::tostringstream oss;
	oss << m_key.first.Get() << _T(" - frame #") << m_key.second;
	oss << sep << GetBmpFmt().FormatDbg( sep );

	CTime lastModifyTime = fs::ReadLastModifyTime( m_key.first );
	if ( time_utl::IsValid( lastModifyTime ) )
		oss << sep << _T("modify_time=") << lastModifyTime.Format( _T("%d/%m/%Y %H:%M:%S") ).GetString();

	return oss.str();
}

#endif // _DEBUG

CWicImage::TMultiFrameDecoderMap& CWicImage::SharedMultiFrameDecoders( void )
{
	static TMultiFrameDecoderMap s_loadedDecoders;
	return s_loadedDecoders;
}

wic::CBitmapDecoder CWicImage::AcquireDecoder( const fs::CFlexPath& imagePath, utl::ErrorHandling handlingMode /*= utl::CheckMode*/ )
{
	if ( CMultiFrameDecoder* pSharedDecoder = SharedMultiFrameDecoders().Find( imagePath ) )
		return pSharedDecoder->GetDecoder();
	else
	{
		wic::CBitmapDecoder decoder( imagePath, handlingMode );
		return decoder;
	}
}


// CWicImage::CMultiFrameDecoder implementation

CWicImage::CMultiFrameDecoder::CMultiFrameDecoder( const wic::CBitmapDecoder& decoder )
	: m_decoder( decoder )
{
	wic::TDecoderFlags decoderFlags = decoder.GetDecoderFlags();
	ASSERT( HasFlag( decoderFlags, wic::CBitmapDecoder::MultiFrame ) );
	ASSERT( !HasFlag( decoderFlags, wic::CBitmapDecoder::Animation ) );
	decoderFlags;
}

CWicImage::CMultiFrameDecoder::~CMultiFrameDecoder()
{
	ASSERT( m_loadedFrames.empty() );		// all frames within should have been destroyed
}

size_t CWicImage::CMultiFrameDecoder::FindPosLoaded( UINT framePos ) const
{
	ASSERT( framePos < m_decoder.GetFrameCount() );

	for ( size_t pos = 0; pos != m_loadedFrames.size(); ++pos )
		if ( m_loadedFrames[ pos ]->GetFramePos() == framePos )
			return pos;

	return utl::npos;
}

std::auto_ptr<CWicImage> CWicImage::CMultiFrameDecoder::LoadFrame( const fs::TImagePathKey& imageKey )
{
	REQUIRE( !IsLoaded( imageKey.second ) );

	std::auto_ptr<CWicImage> pNewFrameImage( new CWicImage() );

	if ( !pNewFrameImage->LoadDecoderFrame( m_decoder, imageKey ) )
		pNewFrameImage.reset();			// invalid framePos?
	else
	{
		pNewFrameImage->SetSharedDecoder( this );
		m_loadedFrames.push_back( pNewFrameImage.get() );
	}

	return pNewFrameImage;
}

bool CWicImage::CMultiFrameDecoder::UnloadFrame( CWicImage* pFrameImage )
{
	ASSERT_PTR( pFrameImage );
	size_t foundPos = FindPosLoaded( pFrameImage->GetFramePos() );
	if ( foundPos != utl::npos )
	{
		pFrameImage->SetSharedDecoder( nullptr );
		m_loadedFrames.erase( m_loadedFrames.begin() + foundPos );
	}
	else
		ASSERT( false );

	return AnyFramesLoaded();
}


namespace ui
{
	// CImageFileDetails implementation

	void CImageFileDetails::Reset( const CWicImage* pImage /*= nullptr*/ )
	{
		m_filePath.Clear();
		m_isAnimated = false;
		m_fileSize = m_framePos = m_frameCount = m_navigPos = m_navigCount = 0;
		m_dimensions = CSize( 0, 0 );

		if ( pImage != nullptr )
		{
			m_filePath = pImage->GetImagePath();
			m_isAnimated = pImage->IsAnimated();

			m_fileSize = static_cast<UINT>( fs::flex::GetFileSize( m_filePath ) );

			m_framePos = pImage->GetFramePos();
			m_frameCount = pImage->GetFrameCount();
			m_dimensions = pImage->GetBmpSize();

			m_navigCount = m_frameCount;		// assume a single multi-frame image by default (overriden for embedded images)
		}
	}

	double CImageFileDetails::GetMegaPixels( void ) const
	{
		return num::ConvertFileSize( ui::GetSizeArea( m_dimensions ), num::MegaBytes ).first;
	}

	bool CImageFileDetails::operator==( const CImageFileDetails& right ) const
	{
		return
			m_filePath == right.m_filePath &&
			m_isAnimated == right.m_isAnimated &&
			m_fileSize == right.m_fileSize &&
			m_framePos == right.m_framePos &&
			m_frameCount == right.m_frameCount &&
			m_dimensions == right.m_dimensions &&
			m_navigPos == right.m_navigPos &&
			m_navigCount == right.m_navigCount;
	}
}


namespace fs
{
	// CImageFilterStore implementation

	const std::tstring CImageFilterStore::s_classTag = _T("Images");

	CImageFilterStore::CImageFilterStore( WICComponentType codec /*= WICDecoder*/ )
		: CFilterStore( s_classTag, WICDecoder == codec ? fs::BrowseOpen : fs::BrowseSave )
	{
		Enumerate( codec );
	}

	CImageFilterStore& CImageFilterStore::Instance( shell::BrowseMode browseMode )
	{
		static CImageFilterStore s_openFilterSpecs( WICDecoder ), s_saveFilterSpecs( WICEncoder );
		return fs::BrowseOpen == browseMode ? s_openFilterSpecs : s_saveFilterSpecs;
	}

	void CImageFilterStore::Enumerate( WICComponentType codec )
	{
		CComPtr<IEnumUnknown> pEnum;
		if ( !HR_OK( wic::CImagingFactory::Factory()->CreateComponentEnumerator( codec, WICComponentEnumerateDefault, &pEnum ) ) )
			return;

		ULONG elemCount = 0;
		for ( CComPtr<IUnknown> pElement; S_OK == pEnum->Next( 1, &pElement, &elemCount ); pElement = nullptr )
		{
			CComQIPtr<IWICBitmapCodecInfo> pDecoderInfo( pElement );	// IWICBitmapCodecInfo is common base of IWICBitmapDecoderInfo, IWICBitmapEncoderInfo
			std::tstring name, specs;
			{
				UINT nameLen = 0, extsLen = 0;
				pDecoderInfo->GetFriendlyName( 0, nullptr, &nameLen );		// get necessary buffer sizes
				pDecoderInfo->GetFileExtensions( 0, nullptr, &extsLen );

				std::vector<wchar_t> nameBuffer( nameLen ), extsBuffer( extsLen );
				pDecoderInfo->GetFriendlyName( nameLen, &nameBuffer.front(), &nameLen );
				pDecoderInfo->GetFileExtensions( extsLen, &extsBuffer.front(), &extsLen );

				name = &nameBuffer.front();						// "BMP Decoder"

				specs = &extsBuffer.front();					// ".bmp,.dib,.rle"
				str::Replace( specs, _T(","), s_specSep );
				str::Replace( specs, _T("."), _T("*.") );		// "*.bmp;*.dib;*.rle"
			}

			AddFilter( fs::TFilterPair( name, specs ) );
		}
	}

} //namespace fs


namespace shell
{
	bool BrowseImageFile( fs::CPath& rFilePath, BrowseMode browseMode /*= FileOpen*/, DWORD flags /*= 0*/, CWnd* pParentWnd /*= nullptr*/ )
	{
		fs::CFilterJoiner filterJoiner( fs::CImageFilterStore::Instance( browseMode ) );
		return filterJoiner.BrowseFile( rFilePath, browseMode, flags, nullptr, pParentWnd );
	}
}
