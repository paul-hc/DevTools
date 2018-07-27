
#include "stdafx.h"
#include "FlexPath.h"
#include "StructuredStorage.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace path
{
	std::tstring MakeShortHashedFilename( const TCHAR* pFullPath, size_t maxFnameExtLen )
	{
		// pFullPath could refer to a dir, file, or a sub-path
		fs::CFlexPath shortFullPath( pFullPath );
		if ( str::GetLength( shortFullPath.GetNameExt() ) > maxFnameExtLen )
		{
			fs::CPathParts parts( shortFullPath.Get() );

			const size_t prefixLen = maxFnameExtLen - str::GetLength( _T("_ABCDEFAB") ) - parts.m_ext.length();
			const UINT hashKey = static_cast< UINT >( path::GetHashValue( pFullPath ) );		// hash key is unique for the whole path

			parts.m_fname = str::Format( _T("%s_%08X"), parts.m_fname.substr( 0, prefixLen ).c_str(), hashKey );	// "prefix_hexHashKey"
			shortFullPath.Set( parts.MakePath() );

			ENSURE( str::GetLength( path::FindFilename( shortFullPath.GetPtr() ) ) <= maxFnameExtLen );
		}
		return shortFullPath.Get();
	}

} //namespace path


namespace fs
{
	// CFlexPath implementation

	CFlexPath CFlexPath::GetParentFlexPath( bool trailSlash /*= false*/ ) const
	{
		std::tstring parentPath = GetParentPath( trailSlash ).Get();

		str::StripSuffix( parentPath, &path::s_complexPathSep, 1 );
		return CFlexPath( parentPath );
	}

	std::tstring CFlexPath::FormatPretty( void ) const
	{
		std::tstring pretty = Get();
		size_t sepPos = path::FindComplexSepPos( pretty.c_str() );
		std::tstring::iterator itEmbedded = sepPos != std::tstring::npos ? ( pretty.begin() + sepPos + 1 ) : pretty.end();

		std::replace( pretty.begin(), itEmbedded, _T('/'), _T('\\') );
		std::replace( itEmbedded, pretty.end(), _T('\\'), _T('/') );
		return pretty;
	}

	std::tstring CFlexPath::FormatPrettyLeaf( void ) const
	{
		std::tstring prettyLeaf( GetLeafSubPath() );
		std::replace( prettyLeaf.begin(), prettyLeaf.end(), _T('\\'), _T('/') );
		return prettyLeaf;
	}

	bool CFlexPath::FlexFileExist( AccessMode accessMode /*= Exist*/ ) const
	{
		const fs::CPath physicalPath( path::GetPhysical( Get() ) );
		if ( !physicalPath.FileExist( accessMode ) )
			return false;

		if ( IsComplexPath() )
			if ( CStructuredStorage* pDocStg = CStructuredStorage::FindOpenedStorage( physicalPath ) )
				return pDocStg->OpenDeepStream( GetEmbeddedPath() ) != NULL;
			else
			{
				fs::CStructuredStorage stg;
				return stg.Open( physicalPath.GetPtr() ) && stg.OpenDeepStream( GetEmbeddedPath() ) != NULL;
			}

		return true;
	}

} //namespace fs
