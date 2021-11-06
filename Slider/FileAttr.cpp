
#include "stdafx.h"
#include "FileAttr.h"
#include "ModelSchema.h"
#include "ICatalogStorage.h"			// for CCatalogStorageFactory::IsVintageCatalog()
#include "CatalogStorageService.h"		// for ToAlbumModel()
#include "AlbumModel.h"
#include "Application_fwd.h"
#include "utl/EnumTags.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"
#include "utl/StringUtilities.h"
#include "utl/UI/MfcUtilities.h"
#include "utl/UI/ImagingWic.h"
#include "utl/UI/TaskDialog.h"
#include "utl/UI/WicImageCache.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace serial
{
	fs::ImagePathKey ToStorageRelativePathKey( const fs::ImagePathKey& pathKey )
	{
		return fs::ImagePathKey( fs::CFlexPath( path::GetSubPath( pathKey.first.GetPtr() ) ), pathKey.second );
	}

	void PostLoadAdjustComplexPath( fs::CFlexPath& rPath, const fs::TStgDocPath& docStgPath )
	{
		if ( path::IsRelative( rPath.GetPtr() ) )				// was saved as relative path to storage root?
			rPath = fs::CFlexPath::MakeComplexPath( docStgPath, rPath );	// convert to full flex image path
		else if ( path::IsComplex( rPath.GetPtr() ) )			// was saved as full complex path?
		{
			if ( rPath.GetPhysicalPath() != docStgPath )		// storage was renamed?
				rPath = fs::CFlexPath::MakeComplexPath( docStgPath, rPath.GetEmbeddedPath() );		// backwards compatibility reparent: - replace old storage path with the current one (from the archive)
		}
	}
}


// CFileAttr implementation

CFileAttr::CFileAttr( void )
	: m_imageFormat( wic::UnknownImageFormat )
	, m_lastModifTime( CFileTime() )		// { 0, 0 }
	, m_fileSize( 0 )
	, m_imageDim( 0, 0 )
	, m_baselinePos( utl::npos )
{
}

CFileAttr::CFileAttr( const fs::CPath& filePath )
	: m_pathKey( filePath.Get(), 0 )
	, m_imageFormat( wic::FindFileImageFormat( GetPath().GetPtr() ) )
	, m_lastModifTime( CFileTime() )		// { 0, 0 }
	, m_fileSize( 0 )
	, m_imageDim( 0, 0 )
	, m_baselinePos( utl::npos )
{
	if ( !GetPath().IsEmpty() )
		if ( !ReadFileStatus() )
			TRACE( _T("* CFileAttr::ReadFileStatus(): couldn't acces file status for file: %s\n"), GetPath().GetPtr() );
}

CFileAttr::CFileAttr( const fs::CFileState& streamState )
	: m_pathKey( streamState.m_fullPath.Get(), 0 )
	, m_imageFormat( wic::FindFileImageFormat( GetPath().GetPtr() ) )
	, m_lastModifTime( CFileTime( streamState.m_modifTime.GetTime() ) )
	, m_fileSize( static_cast<UINT>( streamState.m_fileSize ) )
	, m_imageDim( 0, 0 )
	, m_baselinePos( utl::npos )
{
}

CFileAttr::CFileAttr( const CFileFind& foundFile )
	: m_pathKey( fs::CFlexPath( foundFile.GetFilePath().GetString() ), 0 )
	, m_imageFormat( wic::FindFileImageFormat( GetPath().GetPtr() ) )
	, m_fileSize( static_cast< UINT >( foundFile.GetLength() ) )
	, m_imageDim( 0, 0 )
	, m_baselinePos( utl::npos )
{
	foundFile.GetLastWriteTime( &m_lastModifTime );
}

CFileAttr::~CFileAttr()
{
}

void CFileAttr::Stream( CArchive& archive )
{
	if ( archive.IsStoring() )
	{
		if ( path::IsComplex( archive.m_strFileName ) )					// saving to an image archive storage?
			archive << serial::ToStorageRelativePathKey( m_pathKey );	// only save the relative path from the root storage
		else
			archive << m_pathKey;

		archive << (int)m_imageFormat;
		archive & m_lastModifTime;
		archive << m_fileSize;
		archive << GetSavingImageDim();
	}
	else
	{
		app::ModelSchema docModelSchema = EvalLoadingSchema( archive );

		if ( docModelSchema >= app::Slider_v3_8 )
			archive >> m_pathKey;
		else
		{	// backwards compatible: just the path was saved
			archive >> m_pathKey.first;
			m_pathKey.second = 0;
		}

		if ( path::IsComplex( archive.m_strFileName.GetString() ) )			// loading from an image archive storage?
			serial::PostLoadAdjustComplexPath( m_pathKey.first, path::ExtractPhysical( archive.m_strFileName.GetString() ) );

		if ( docModelSchema >= app::Slider_v5_6 )
			archive >> (int&)m_imageFormat;
		else
		{
			int oldFileType;		// enum FileType { FT_Generic, FT_BMP, FT_JPEG, FT_GIFF, FT_TIFF };
			archive >> oldFileType;
			m_imageFormat = wic::FindFileImageFormat( GetPath().GetPtr() );
		}

		archive & m_lastModifTime;
		archive >> m_fileSize;
		archive >> m_imageDim;
	}
}

app::ModelSchema CFileAttr::EvalLoadingSchema( CArchive& rLoadArchive )
{
	// NOTES:
	//	1) All this complexity is required to maintain backwards compatibility to the very earliest Slider, since the ModelSchema was not saved.
	//	2) It's fairly efficient, since we are checking only for the first loaded CFileAttr object.
	//	3) This is messy, in order to keep CFileAttr::Stream() as clean as possible.

	app::ModelSchema docModelSchema = app::GetLoadingSchema( rLoadArchive );
	CAlbumModel* pAlbumModel = NULL;

	if ( rLoadArchive.m_pDocument != NULL )
		pAlbumModel = svc::ToAlbumModel( rLoadArchive.m_pDocument );

	if ( serial::CStreamingGuard* pLoadGuard = serial::CStreamingGuard::GetTop() )
		if ( !pLoadGuard->HasStreamingFlag( Loading_InspectedPathEncoding ) )
		{	// backwards compatibility with first versions of Slider, pre 2.1, that were serializing paths as wide encoded strings:
			pLoadGuard->SetStreamingFlag( Loading_InspectedPathEncoding );		// check this only once

			if ( serial::WideEncoding == serial::InspectSavedStringEncoding( rLoadArchive ) )		// found old wide-encoded path?
			{
				if ( app::Slider_LatestModelSchema == docModelSchema )		// album did not persist model schema?
				{
					docModelSchema = app::Slider_v4_0;						// assume an earlier schema

					if ( pAlbumModel != NULL )
						pAlbumModel->StoreModelSchema( docModelSchema );	// mark model to earlier version
				}

				// note: versions up to Slider_v4_0 used WIDE encoding
				TRACE( _T(" (!) CFileAttr::Stream() - detected earlier version %s (using WIDE-encoded paths) in document '%s'\n"),
					app::FormatModelVersion( docModelSchema ).c_str(), rLoadArchive.m_strFileName.GetString() );
			}

			// check whether is older than app::Slider_v3_8 (having saved just the path, not the framePos)
			const BYTE* pOldCursor = serial::GetLoadingCursor( rLoadArchive );
			persist fs::ImagePathKey pathKey;
			persist int imageFormat;

			rLoadArchive >> pathKey;
			rLoadArchive >> imageFormat;

			serial::UnreadBytes( rLoadArchive, serial::GetLoadingCursor( rLoadArchive ) - pOldCursor );		// rewind back in order to re-read {m_pathKey, m_imageFormat}

			if ( imageFormat < 0 || imageFormat > wic::UnknownImageFormat )			// out-of-range imageFormat?
			{
				docModelSchema = app::Slider_v3_7;									// we have a Slider_v3_7 (-) document

				if ( path::IsComplex( rLoadArchive.m_strFileName.GetString() ) )	// loading from an image archive storage?
				{
					fs::TStgDocPath docStgPath = path::ExtractPhysical( rLoadArchive.m_strFileName.GetString() );

					if ( CCatalogStorageFactory::IsVintageCatalog( docStgPath.GetPtr() ) )		// a .cid or .icf catalog?
						docModelSchema = app::Slider_v3_1;							// assume the oldest Slider_v3_1 (-) document
				}

				if ( pAlbumModel != NULL )
					pAlbumModel->StoreModelSchema( docModelSchema );				// mark model to earlier version

				TRACE( _T(" (!) CFileAttr::Stream() - detected earlier version %s in document '%s' (not using framePos with path)\n"),
					app::FormatModelVersion( docModelSchema ).c_str(), rLoadArchive.m_strFileName.GetString() );
			}
		}

	if ( pAlbumModel != NULL )
		docModelSchema = std::min( docModelSchema, pAlbumModel->GetModelSchema() );		// use the earliest marked schema version

	return docModelSchema;
}

const CSize& CFileAttr::GetSavingImageDim( void ) const
{
	// OPTIMIZATION for documents with many images: speed up saving by saving unevaluated dimensions.
	// note: GetImageDim() uses WIC to load each image, an expensive operation.
	if ( serial::CStreamingGuard* pTimeGuard = serial::CStreamingGuard::GetTop() )
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

const std::tstring& CFileAttr::GetCode( void ) const
{
	return GetPath().Get();
}

std::tstring CFileAttr::GetDisplayCode( void ) const
{
	return GetPath().FormatPrettyLeaf();
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
