
#include "stdafx.h"
#include "FileAttr.h"
#include "ModelSchema.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"
#include "utl/StringUtilities.h"
#include "utl/UI/WicImageCache.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CFileAttr::CFileAttr( void )
	: m_type( FT_Generic )
	, m_lastModifTime( CFileTime() )		// { 0, 0 }
	, m_fileSize( 0 )
	, m_imageDim( 0, 0 )
{
}

CFileAttr::CFileAttr( const fs::CPath& filePath )
	: m_pathKey( filePath.Get(), 0 )
	, m_type( CFileAttr::LookupFileType( GetPath().GetPtr() ) )
	, m_lastModifTime( CFileTime() )		// { 0, 0 }
	, m_fileSize( 0 )
	, m_imageDim( 0, 0 )
{
	if ( !ReadFileStatus() )
		TRACE( _T("* CFileAttr::ReadFileStatus(): couldn't acces file status for file: %s\n"), GetPath().GetPtr() );
}

CFileAttr::CFileAttr( const CFileFind& foundFile )
	: m_pathKey( fs::CFlexPath( foundFile.GetFilePath().GetString() ), 0 )
	, m_type( CFileAttr::LookupFileType( GetPath().GetNameExt() ) )
	, m_fileSize( static_cast< UINT >( foundFile.GetLength() ) )
	, m_imageDim( 0, 0 )
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
		archive << GetImageDim();
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
