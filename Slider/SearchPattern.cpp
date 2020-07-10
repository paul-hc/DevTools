
#include "stdafx.h"
#include "SearchPattern.h"
#include "Application_fwd.h"
#include "resource.h"
#include "utl/EnumTags.h"
#include "utl/FileEnumerator.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"
#include "utl/StringUtilities.h"
#include "utl/UI/ShellDialogs.h"
#include "utl/UI/WicImage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CSearchPattern::CSearchPattern( void )
	: CPathItemBase( fs::CPath() )
	, m_type( DirPath )
	, m_searchMode( RecurseSubDirs )
{
}

CSearchPattern::CSearchPattern( const fs::CPath& searchPath )
	: CPathItemBase( fs::CPath() )
	, m_type( DirPath )
	, m_searchMode( RecurseSubDirs )
{
	SetFilePath( searchPath );
}

void CSearchPattern::Stream( CArchive& archive )
{
	__super::Stream( archive );

	if ( archive.IsStoring() )
	{
		archive << m_wildFilters;
		archive << m_type;
		archive << m_searchMode;
	}
	else
	{
		app::ModelSchema savedModelSchema = app::GetLoadingSchema( archive );

		archive >> m_wildFilters;
		archive >> (int&)m_type;
		archive >> (int&)m_searchMode;

		if ( savedModelSchema < app::Slider_v4_1 )
		{
			enum OldType { old_Invalid, old_DirPath, old_CatalogDocFile, old_ExplicitFile } oldType = static_cast< OldType >( m_type );

			(int&)m_type = oldType != old_Invalid ? ( m_type - 1 ) : BestGuessType( GetFilePath() );		// old_Invalid was removed in v4.1
		}
	}
}

void CSearchPattern::SetFilePath( const fs::CPath& filePath )
{
	__super::SetFilePath( filePath );

	if ( utl::ModifyValue( m_type, CheckType() ) )		// pattern type changed?
		switch ( m_type )
		{
			case DirPath:
				m_searchMode = RecurseSubDirs;
				break;
			case CatalogDocFile:
			case ExplicitFile:
				m_wildFilters.clear();
				m_searchMode = ShallowDir;
				break;
		}
}

CSearchPattern::Type CSearchPattern::CheckType( void ) const
{
	const fs::CPath& filePath = GetFilePath();

	if ( fs::IsValidDirectory( filePath.GetPtr() ) )
		return DirPath;
	else if ( filePath.FileExist() )
		if ( app::IsCatalogFile( filePath.GetPtr() ) )
			return CatalogDocFile;
		else
			return ExplicitFile;

	return BestGuessType( filePath );
}

CSearchPattern::Type CSearchPattern::BestGuessType( const fs::CPath& searchPath )
{
	if ( app::IsCatalogFile( searchPath.GetPtr() ) )
		return CatalogDocFile;
	else if ( str::IsEmpty( searchPath.GetExt() ) )
		return DirPath;

	return ExplicitFile;
}

const std::tstring& CSearchPattern::GetSafeWildFilters( void ) const
{
	ASSERT( IsDirPath() );

	if ( m_wildFilters.empty() )
		return app::GetAllSourcesWildSpecs();

	return m_wildFilters;
}

bool CSearchPattern::IsValidPath( void ) const
{
	if ( IsEmpty() )
		return false;

	switch ( m_type )
	{
		case DirPath:
			return fs::IsValidDirectory( GetFilePath().GetPtr() );
		case CatalogDocFile:
			return fs::IsValidStructuredStorage( GetFilePath().GetPtr() );
		default:
			ASSERT( false );
		case ExplicitFile:
			return fs::IsValidFile( GetFilePath().GetPtr() );
	}
}

bool CSearchPattern::BrowseFilePath( BrowseMode pathType /*= BrowseAsIs*/, CWnd* pParentWnd /*= NULL*/, DWORD extraFlags /*= OFN_FILEMUSTEXIST*/ )
{
	std::tstring filePath = GetFilePath().Get();

	if ( BrowseAsIs == pathType )
		if ( DirPath == m_type )
			pathType = BrowseAsDirPath;
		else		// CompoundFile, ExplicitFile
			pathType = BrowseAsFilePath;

	bool picked = false;

	if ( BrowseAsDirPath == pathType )
	{
		static const TCHAR s_dlgTitle[] = _T("Select Folder with Images");

		if ( fs::IsValidFile( filePath.c_str() ) )
			filePath = fs::CPath( filePath ).GetParentPath().Get();

		picked = shell::PickFolder( filePath, pParentWnd, 0, s_dlgTitle );
		//picked = shell::BrowseForFolder( filePath, pParentWnd, NULL, shell::BF_FileSystem, s_dlgTitle );
	}
	else
		if ( CatalogDocFile == m_type )
			picked = app::BrowseCatalogFile( filePath, pParentWnd, shell::FileOpen, extraFlags );
		else	// ExplicitFile
			picked = shell::BrowseImageFile( filePath, shell::FileOpen, extraFlags, pParentWnd );

	if ( !picked )
		return false;

	SetFilePath( filePath );
	return true;
}

void CSearchPattern::EnumImageFiles( fs::IEnumerator* pEnumerator ) const
{
	if ( !IsValidPath() )
		AfxThrowFileException( CFileException::fileNotFound, -1, GetFilePath().GetPtr() );		// folder path doesn't exist

	switch ( m_type )
	{
		case DirPath:
			fs::EnumFiles( pEnumerator, GetFilePath(), GetSafeWildFilters().c_str(), RecurseSubDirs == m_searchMode ? Deep : Shallow );
			break;
		case CatalogDocFile:
		case ExplicitFile:
			pEnumerator->AddFoundFile( GetFilePath().GetPtr() );
			break;
	}
}

bool CSearchPattern::IsNumFileName( const TCHAR* pFullPath )
{
	std::tstring fname( path::FindFilename( pFullPath ), path::FindExt( pFullPath ) );
	Range< size_t > numSeq = num::FindNumericSequence( fname );
	return numSeq.IsNonEmpty();
}

int CSearchPattern::ParseNumFileNameNumber( const TCHAR* pFullPath )
{
	std::tstring fname( path::FindFilename( pFullPath ), path::FindExt( pFullPath ) );
	Range< size_t > numSeq = num::FindNumericSequence( fname );

	int number = -1;
	if ( numSeq.IsNonEmpty() )
		num::ParseNumber( number, fname.c_str() + numSeq.m_start );
	return number;
}

std::tstring CSearchPattern::FormatNumericFilePath( const TCHAR* pFullPath, int numSeq )
{
	fs::CPathParts parts( pFullPath );
	parts.m_fname = str::Format( _T("%04d"), numSeq );
	return parts.MakePath().Get();
}

const CEnumTags& CSearchPattern::GetTags_Type( void )
{
	static const CEnumTags s_tags( _T("Folder|Image Archive|Image") );
	return s_tags;
}

const CEnumTags& CSearchPattern::GetTags_SearchMode( void )
{
	static const CEnumTags s_tags( _T("Shallow|Recurse|Auto-drop recipient (for numeric filenames)") );
	return s_tags;
}
