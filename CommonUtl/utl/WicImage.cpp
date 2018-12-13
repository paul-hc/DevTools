
#include "stdafx.h"
#include "WicImage.h"
#include "WicAnimatedImage.h"
#include "StructuredStorage.h"
#include "TimeUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const fs::ImagePathKey CWicImage::s_nullKey( fs::CFlexPath(), UINT_MAX );

CWicImage::CWicImage( const WICPixelFormatGUID* pCvtPixelFormat /*= &GUID_WICPixelFormat32bppPBGRA*/ )
	: CWicBitmap()
	, m_key( s_nullKey )
	, m_frameCount( 0 )
	, m_pCvtPixelFormat( pCvtPixelFormat )
{
}

CWicImage::~CWicImage()
{
}

std::auto_ptr< CWicImage > CWicImage::CreateFromFile( const fs::ImagePathKey& imageKey, bool throwMode /*= false*/ )
{
	wic::CBitmapDecoder decoder( imageKey.first, throwMode );
	std::auto_ptr< CWicImage > pNewImage;
	if ( decoder.IsValid() )
	{
		int decoderFlags = decoder.GetDecoderFlags();
		if ( HasFlag( decoderFlags, wic::CBitmapDecoder::MultiFrame ) && HasFlag( decoderFlags, wic::CBitmapDecoder::Animation ) )		// multi-frame GIF with animation?
			pNewImage.reset( new CWicAnimatedImage( decoder ) );
	}
	if ( NULL == pNewImage.get() )
		pNewImage.reset( new CWicImage );

	if ( !pNewImage->LoadDecoderFrame( decoder, imageKey ) )
		pNewImage.reset();

	return pNewImage;
}

void CWicImage::Clear( void )
{
	CWicBitmap::Clear();
	m_frameCount = 0;
	m_key = s_nullKey;
}

bool CWicImage::LoadFromFile( const fs::ImagePathKey& imageKey )
{
	wic::CBitmapDecoder decoder( imageKey.first, IsThrowMode() );
	return LoadDecoderFrame( decoder, imageKey );
}

bool CWicImage::LoadDecoderFrame( wic::CBitmapDecoder& decoder, const fs::ImagePathKey& imageKey )
{
	Clear();
	m_key = imageKey;

	m_frameCount = decoder.GetFrameCount();

	if ( GetFramePos() < m_frameCount )
		if ( CComPtr< IWICBitmapSource > pFrameBitmap = decoder.ConvertFrameAt( GetFramePos(), m_pCvtPixelFormat ) )
			if ( StoreFrame( pFrameBitmap, GetFramePos() ) )
			{
				GetOrigin().SetOriginalPath( m_key.first );
				return true;
			}

	return false;
}

bool CWicImage::LoadFrame( UINT framePos )
{
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

	wic::CBitmapDecoder decoder( imagePath );
	if ( !decoder.IsValid() || 0 == decoder.GetFrameCount() )
		return false;

	static const CSize emptySize( 0, 0 );

	for ( UINT framePos = 0; framePos != decoder.GetFrameCount(); ++framePos )
		if ( emptySize == wic::GetBitmapSize( decoder.GetFrameAt( framePos ) ) )
			return false;			// frame is corrupted

	return true;
}

bool CWicImage::IsCorruptFrame( const fs::ImagePathKey& imageKey )
{
	if ( imageKey.first.IsEmpty() || !imageKey.first.FileExist( fs::Read ) )
		return false;				// file doesn't exist -> doesn't mean is corrupt!

	wic::CBitmapDecoder decoder( imageKey.first );
	return
		decoder.IsValid() &&
		decoder.GetFrameCount() != 0 &&
		wic::GetBitmapSize( decoder.GetFrameAt( imageKey.second ) ) != CSize( 0, 0 );
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
		static CImageFilterStore openFilterSpecs( WICDecoder ), saveFilterSpecs( WICEncoder );
		return fs::BrowseOpen == browseMode ? openFilterSpecs : saveFilterSpecs;
	}

	void CImageFilterStore::Enumerate( WICComponentType codec )
	{
		CComPtr< IEnumUnknown > pEnum;
		if ( !HR_OK( wic::CImagingFactory::Factory()->CreateComponentEnumerator( codec, WICComponentEnumerateDefault, &pEnum ) ) )
			return;

		ULONG elemCount = 0;
		for ( CComPtr< IUnknown > pElement; S_OK == pEnum->Next( 1, &pElement, &elemCount ); pElement = NULL )
		{
			CComQIPtr< IWICBitmapCodecInfo > pDecoderInfo = pElement;	// IWICBitmapCodecInfo is common base of IWICBitmapDecoderInfo, IWICBitmapEncoderInfo
			std::tstring name, specs;
			{
				UINT nameLen = 0, extsLen = 0;
				pDecoderInfo->GetFriendlyName( 0, NULL, &nameLen );		// get necessary buffer sizes
				pDecoderInfo->GetFileExtensions( 0, NULL, &extsLen );

				std::vector< wchar_t > nameBuffer( nameLen ), extsBuffer( extsLen );
				pDecoderInfo->GetFriendlyName( nameLen, &nameBuffer.front(), &nameLen );
				pDecoderInfo->GetFileExtensions( extsLen, &extsBuffer.front(), &extsLen );

				name = &nameBuffer.front();						// "BMP Decoder"

				specs = &extsBuffer.front();					// ".bmp,.dib,.rle"
				str::Replace( specs, _T(","), s_specSep );
				str::Replace( specs, _T("."), _T("*.") );		// "*.bmp;*.dib;*.rle"
			}

			AddFilter( fs::FilterPair( name, specs ) );
		}
	}

} //namespace fs


namespace shell
{
	bool BrowseImageFile( std::tstring& rFilePath, BrowseMode browseMode /*= FileOpen*/, DWORD flags /*= 0*/, CWnd* pParentWnd /*= NULL*/ )
	{
		fs::CFilterJoiner filterJoiner( fs::CImageFilterStore::Instance( browseMode ) );
		return filterJoiner.BrowseFile( rFilePath, browseMode, flags, NULL, pParentWnd );
	}
}
