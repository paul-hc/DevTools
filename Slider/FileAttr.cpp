
#include "stdafx.h"
#include "FileAttr.h"
#include "ModelSchema.h"
#include "Application_fwd.h"
#include "utl/EnumTags.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"
#include "utl/StringUtilities.h"
#include "utl/UI/MfcUtilities.h"
#include "utl/UI/TaskDialog.h"
#include "utl/UI/WicImageCache.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFileAttr implementation

CFileAttr::CFileAttr( void )
	: m_type( FT_Generic )
	, m_lastModifTime( CFileTime() )		// { 0, 0 }
	, m_fileSize( 0 )
	, m_imageDim( 0, 0 )
	, m_baselinePos( utl::npos )
{
}

CFileAttr::CFileAttr( const fs::CPath& filePath )
	: m_pathKey( filePath.Get(), 0 )
	, m_type( CFileAttr::LookupFileType( GetPath().GetPtr() ) )
	, m_lastModifTime( CFileTime() )		// { 0, 0 }
	, m_fileSize( 0 )
	, m_imageDim( 0, 0 )
	, m_baselinePos( utl::npos )
{
	if ( !ReadFileStatus() )
		TRACE( _T("* CFileAttr::ReadFileStatus(): couldn't acces file status for file: %s\n"), GetPath().GetPtr() );
}

CFileAttr::CFileAttr( const CFileFind& foundFile )
	: m_pathKey( fs::CFlexPath( foundFile.GetFilePath().GetString() ), 0 )
	, m_type( CFileAttr::LookupFileType( GetPath().GetNameExt() ) )
	, m_fileSize( static_cast< UINT >( foundFile.GetLength() ) )
	, m_imageDim( 0, 0 )
	, m_baselinePos( utl::npos )
{
	foundFile.GetLastWriteTime( &m_lastModifTime );
}

CFileAttr::~CFileAttr()
{
}

void CFileAttr::SetFromTransferPair( const fs::CFlexPath& srcPath, const fs::CFlexPath& destPath )
{
	*this = CFileAttr( srcPath );					// inherit src attributes
	GetImageDim();									// force init image dim through image loading
	m_pathKey.first.Set( destPath.GetPtr() );		// restore the true destination path
}

void CFileAttr::Stream( CArchive& archive )
{
	if ( archive.IsStoring() )
	{
		archive << m_pathKey;
		archive << (int)m_type;
		archive & m_lastModifTime;
		archive << m_fileSize;
		archive << GetSavingImageDim();
	}
	else
	{
		app::ModelSchema docModelSchema = app::GetLoadingSchema( archive );

		if ( docModelSchema >= app::Slider_v3_8 )
			archive >> m_pathKey;
		else
		{	// backwards compatible: just the path was saved
			archive >> m_pathKey.first;
			m_pathKey.second = 0;
		}

		archive >> (int&)m_type;
		archive & m_lastModifTime;
		archive >> m_fileSize;
		archive >> m_imageDim;
	}
}

bool CFileAttr::ReadFileStatus( void )
{
	if ( GetPath().IsEmpty() || GetPath().IsComplexPath() )
		return true;							// nothing to evaluate

	TCHAR absolutePath[ MAX_PATH ];
	if ( AfxFullPath( absolutePath, GetPath().GetPtr() ) )
		m_pathKey.first.Set( absolutePath );	// convert to absolute path

	WIN32_FILE_ATTRIBUTE_DATA status;
	if ( !::GetFileAttributesEx( GetPath().GetPtr(), GetFileExInfoStandard, &status ) )
		return false;

	m_lastModifTime = status.ftLastWriteTime;
	m_fileSize = status.nFileSizeLow;			// ignore status.nFileSizeHigh due to UINT storage (backwards compatibility)
	return true;
}

const CSize& CFileAttr::GetImageDim( void ) const
{
	if ( 0 == m_imageDim.cx && 0 == m_imageDim.cy )
		m_imageDim = CWicImageCache::Instance().LookupImageDim( m_pathKey );

	return m_imageDim;
}

const CSize& CFileAttr::GetSavingImageDim( void ) const
{
	// OPTIMIZATION for documents with many images: speed up saving by saving unevaluated dimensions.
	// note: GetImageDim() uses WIC to load each image, an expensive operation.
	if ( serial::CStreamingTimeGuard* pTimeGuard = serial::CStreamingTimeGuard::GetTop() )
		if ( pTimeGuard->HasStreamingFlag( Saving_SkipImageDimEvaluation ) )
			return m_imageDim;
		else
			if ( pTimeGuard->IsTimeout( 15.0 ) )								// saving takes more that 15 seconds?
				if ( !pTimeGuard->HasStreamingFlag( Saving_PromptedSpeedUp ) )
				{
					pTimeGuard->SetStreamingFlag( Saving_PromptedSpeedUp );		// user was asked

					if ( PromptedSpeedUpSaving( pTimeGuard->GetTimer() ) )
					{	// speed up saving by saving unevaluated dimensions (will be lazy evaluation for the rest of images)
						pTimeGuard->SetStreamingFlag( Saving_SkipImageDimEvaluation );
						app::LogLine( _T("Speeding up saving by saving unevaluated dimensions for album: %s"), serial::GetDocumentPath( pTimeGuard->GetArchive() ).GetPtr() );
					}
				}

	return GetImageDim();
}

const std::tstring& CFileAttr::GetCode( void ) const
{
	return GetPath().Get();
}

std::tstring CFileAttr::GetDisplayCode( void ) const
{
	return GetPath().FormatPrettyLeaf();
}

FileType CFileAttr::LookupFileType( const TCHAR* pFilePath )
{
	static struct { fs::CPath ext; FileType type; } knownTypes[] =
	{
		{ _T(".jpg"), FT_JPEG }, { _T(".jpeg"), FT_JPEG }, { _T(".spj"), FT_JPEG }, { _T(".bmp"), FT_BMP }, { _T(".dib"), FT_BMP },
		{ _T(".gif"), FT_GIFF }, { _T(".giff"), FT_GIFF }, { _T(".tif"), FT_TIFF }, { _T(".tiff"), FT_TIFF }
	};
	fs::CPath ext( path::FindExt( pFilePath ) );

	if ( !ext.IsEmpty() )
		for ( size_t i = 0; i != COUNT_OF( knownTypes ); ++i )
			if ( knownTypes[ i ].ext == ext )
				return knownTypes[ i ].type;

	return FT_Generic;
}

std::tstring CFileAttr::FormatFileSize( DWORD divideBy /*= KiloByte*/, const TCHAR* pFormat /*= _T("%s KB")*/ ) const
{
	return str::Format( pFormat, num::FormatNumber( m_fileSize / divideBy, str::GetUserLocale() ).c_str(), m_imageDim.cx, m_imageDim.cy );	// assume m_imageDim is computed if used
}


bool CFileAttr::PromptedSpeedUpSaving( CTimer& rSavingTimer )
{
	static bool s_dontAsk = false, s_speedUp = false;
	static const std::tstring s_contentText = _T("Saving is taking too long due to the evaluation of image dimensions, which requires loading of each image file.");

	if ( !s_dontAsk )
	{
		CTimer promptTimer;

		{
			CTaskDialog dlg(
				_T("Album Save Mode"),					// title
				_T("Saving Album Optimization!"),		// instruction (blue)
				s_contentText,							// content
				ID_SKIP_IMAGE_DIM_EVALUATION_CMDLINK,	// speed-up
				ID_KEEP_IMAGE_DIM_EVALUATION_CMDLINK,	// keep evaluating
				TDCBF_CLOSE_BUTTON );

			dlg.SetMainIcon( TD_WARNING_ICON );
			dlg.SetFooterIcon( TD_INFORMATION_ICON );
			dlg.SetFooterText( _T("You could speed-up saving by deferring image dimension evaluation for later, when required.\n") );

			// verification checkbox
			dlg.SetVerificationText( _T("Do not ask in the future") );
			dlg.SetVerificationChecked( s_dontAsk );

			switch ( dlg.DoModal( AfxGetMainWnd() ) )
			{
				case ID_SKIP_IMAGE_DIM_EVALUATION_CMDLINK:
					s_speedUp = true;
					break;
				case ID_KEEP_IMAGE_DIM_EVALUATION_CMDLINK:
				default:	// TDCBF_CLOSE_BUTTON
					s_speedUp = false;
			}

			s_dontAsk = dlg.IsVerificationChecked() != FALSE;
		}

		rSavingTimer.SubtractByElapsed( promptTimer );		// ignore the user interaction elapsed time from the guard
	}

	return s_speedUp;
}
