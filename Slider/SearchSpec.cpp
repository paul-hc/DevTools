
#include "stdafx.h"
#include "SearchSpec.h"
#include "ImageArchiveStg.h"
#include "Application.h"
#include "resource.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"
#include "utl/StringUtilities.h"
#include "utl/UI/ShellDialogs.h"
#include "utl/UI/WicImage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CSearchSpec::CSearchSpec( void )
	: CPathItemBase( fs::CPath() )
	, m_type( DirPath )
	, m_searchMode( RecurseSubDirs )
{
}

CSearchSpec::CSearchSpec( const fs::CPath& searchPath )
	: CPathItemBase( fs::CPath() )
{
	SetFilePath( searchPath );
}

void CSearchSpec::Stream( CArchive& archive )
{
	__super::Stream( archive );

	if ( archive.IsStoring() )
	{
		archive << m_searchFilters;
		archive << m_type;
		archive << m_searchMode;
	}
	else
	{
		app::ModelSchema savedModelSchema = app::GetLoadingSchema( archive );

		archive >> m_searchFilters;
		archive >> (int&)m_type;
		archive >> (int&)m_searchMode;

		if ( savedModelSchema < app::Slider_v4_1 )
		{
			enum OldType { old_Invalid, old_DirPath, old_ArchiveStgFile, old_ExplicitFile } oldType = static_cast< OldType >( m_type );

			(int&)m_type = oldType != old_Invalid ? ( m_type - 1 ) : BestGuessType( GetFilePath() );		// old_Invalid was removed in v4.1
		}
	}
}

void CSearchSpec::SetFilePath( const fs::CPath& filePath )
{
	__super::SetFilePath( filePath );

	m_type = CheckType();

	switch ( m_type )
	{
		case DirPath:
			m_searchMode = RecurseSubDirs;
			break;
		case ArchiveStgFile:
		case ExplicitFile:
			m_searchFilters.clear();
			m_searchMode = ShallowDir;
			break;
	}
}

CSearchSpec::Type CSearchSpec::CheckType( void ) const
{
	if ( fs::IsValidDirectory( GetFilePath().GetPtr() ) )
		return DirPath;
	else if ( GetFilePath().FileExist() )
		if ( CImageArchiveStg::HasImageArchiveExt( GetFilePath().GetPtr() ) )
			return ArchiveStgFile;
		else
			return ExplicitFile;

	return BestGuessType( GetFilePath() );
}

CSearchSpec::Type CSearchSpec::BestGuessType( const fs::CPath& searchPath )
{
	if ( CImageArchiveStg::HasImageArchiveExt( searchPath.GetPtr() ) )
		return ArchiveStgFile;
	else if ( str::IsEmpty( searchPath.GetExt() ) )
		return DirPath;

	return ExplicitFile;
}

const std::tstring& CSearchSpec::GetImageSearchFilters( void ) const
{
	ASSERT( IsDirPath() ); return !m_searchFilters.empty() ? m_searchFilters : app::GetAllSourcesSpecs();
}

bool CSearchSpec::BrowseFilePath( BrowseMode pathType /*= BrowseAsIs*/, CWnd* pParentWnd /*= NULL*/, DWORD extraFlags /*= OFN_FILEMUSTEXIST*/ )
{
	std::tstring filePath = GetFilePath().Get();

	if ( BrowseAsIs == pathType )
		if ( DirPath == m_type )
			pathType = BrowseAsDirPath;
		else		// CompoundFile, ExplicitFile
			pathType = BrowseAsFilePath;

	bool picked = false;

	if ( BrowseAsDirPath == pathType )
		picked = shell::BrowseForFolder( filePath, pParentWnd, NULL, shell::BF_FileSystem, _T("Select the image searching folder path:") );
	else
		if ( ArchiveStgFile == m_type )
			picked = app::BrowseArchiveStgFile( filePath, pParentWnd, shell::FileOpen, extraFlags );
		else	// ExplicitFile, Invalid
			picked = shell::BrowseImageFile( filePath, shell::FileOpen, extraFlags, pParentWnd );

	if ( !picked )
		return false;

	SetFilePath( filePath );
	return true;
}

void CSearchSpec::EnumImageFiles( fs::IEnumerator* pEnumerator ) const
{
	if ( !IsValidPath() )
		AfxThrowFileException( CFileException::fileNotFound, -1, GetFilePath().GetPtr() );		// folder path doesn't exist

	switch ( m_type )
	{
		case DirPath:
			fs::EnumFiles( pEnumerator, GetFilePath(), GetImageSearchFilters().c_str(), RecurseSubDirs == m_searchMode ? Deep : Shallow );
			break;
		case ArchiveStgFile:
		case ExplicitFile:
			pEnumerator->AddFoundFile( GetFilePath().GetPtr() );
			break;
	}
}

bool CSearchSpec::IsNumFileName( const TCHAR* pFullPath )
{
	std::tstring fname( path::FindFilename( pFullPath ), path::FindExt( pFullPath ) );
	Range< size_t > numSeq = num::FindNumericSequence( fname );
	return numSeq.IsNonEmpty();
}

int CSearchSpec::ParseNumFileNameNumber( const TCHAR* pFullPath )
{
	std::tstring fname( path::FindFilename( pFullPath ), path::FindExt( pFullPath ) );
	Range< size_t > numSeq = num::FindNumericSequence( fname );

	int number = -1;
	if ( numSeq.IsNonEmpty() )
		num::ParseNumber( number, fname.c_str() + numSeq.m_start );
	return number;
}

std::tstring CSearchSpec::FormatNumericFilePath( const TCHAR* pFullPath, int numSeq )
{
	fs::CPathParts parts( pFullPath );
	parts.m_fname = str::Format( _T("%04d"), numSeq );
	return parts.MakePath().Get();
}
