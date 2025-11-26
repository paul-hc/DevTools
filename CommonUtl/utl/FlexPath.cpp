
#include "pch.h"
#include "FlexPath.h"
#include "StructuredStorage.h"
#include "Unique.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace path
{
	bool IsEmbedded( const TCHAR* pPath )
	{
		if ( !IsRelative( pPath ) )
			return false;

		if ( HasPrefix( pPath, _T(".") ) || HasPrefix( pPath, _T("..") ) )
			return false;

		return true;
	}

	std::tstring MakeShortHashedFilename( const TCHAR* pFullPath, size_t maxFilenameLen )
	{
		// pFullPath could refer to a dir, file, or a sub-path
		fs::CFlexPath shortFullPath( pFullPath );
		if ( shortFullPath.GetFilename().length() > maxFilenameLen )
		{
			fs::CPathParts parts( shortFullPath.Get() );

			const size_t prefixLen = maxFilenameLen - str::GetLength( _T("_ABCDEFAB") ) - parts.m_ext.length();
			const UINT hashKey = static_cast<UINT>( path::GetHashValuePtr( pFullPath ) );		// hash key is unique for the whole path

			parts.m_fname = str::Format( _T("%s_%08X"), parts.m_fname.substr( 0, prefixLen ).c_str(), hashKey );	// "prefix_hexHashKey"
			shortFullPath.Set( parts.MakePath().Get() );

			ENSURE( str::GetLength( path::FindFilename( shortFullPath.GetPtr() ) ) <= maxFilenameLen );
		}
		return shortFullPath.Get();
	}

} //namespace path


namespace fs
{
	// CFlexPath implementation

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
		if ( IsPhysicalPath() )
			return FileExist( accessMode );

		fs::TStgDocPath docStgPath = GetPhysicalPath();

		if ( !fs::IsValidStructuredStorage( docStgPath.GetPtr() ) )
			return false;

		switch ( accessMode )
		{
			case Write:
			case ReadWrite:
				if ( fs::IsReadOnlyFile( docStgPath.GetPtr() ) )
					return false;			// cannot modify the read-only storage
		}

		if ( CStructuredStorage* pDocStorage = CStructuredStorage::FindOpenedStorage( docStgPath ) )
		{
			CScopedErrorHandling scopedIgnore( pDocStorage, utl::IgnoreMode );		// testing: failure not an error
			return pDocStorage->LocateReadStream( GetEmbeddedPath() ).get() != nullptr;
		}

		return false;
		//return true;		// assume is a valid stream path if the storage document is not open (don't bother opening)
	}

} //namespace fs


namespace path
{
	bool QueryPhysicalPaths( OUT std::vector<fs::CPath>& rPhysicalPaths, const std::vector<fs::CFlexPath>& flexPaths )
	{
		for ( std::vector<fs::CFlexPath>::const_iterator itFlexPath = flexPaths.begin(); itFlexPath != flexPaths.end(); ++itFlexPath )
			if ( itFlexPath->IsPhysicalPath() )
				rPhysicalPaths.push_back( *itFlexPath );

		return !rPhysicalPaths.empty();
	}

	bool QueryStorageDocPaths( OUT std::vector<fs::CPath>& rDocStgPaths, const std::vector<fs::CFlexPath>& flexPaths )
	{
		utl::CUniqueIndex<fs::CPath> uniqueIndex;

		uniqueIndex.Uniquify( &rDocStgPaths );

		for ( std::vector<fs::CFlexPath>::const_iterator itFlexPath = flexPaths.begin(); itFlexPath != flexPaths.end(); ++itFlexPath )
			uniqueIndex.Augment( &rDocStgPaths, itFlexPath->IsComplexPath() ? itFlexPath->GetPhysicalPath() : itFlexPath->Get() );

		// rDocStgPaths is guaranteed unique on return
		return !rDocStgPaths.empty();
	}

	void ConvertToPhysicalPaths( IN OUT std::vector<fs::CFlexPath>& rFlexPaths )
	{
		for ( std::vector<fs::CFlexPath>::iterator itFlexPath = rFlexPaths.begin(); itFlexPath != rFlexPaths.end(); ++itFlexPath )
			if ( itFlexPath->IsComplexPath() )
				itFlexPath->Set( itFlexPath->GetPhysicalPath().Get() );

		utl::Uniquify( rFlexPaths );		// remove duplicates that may have been created (multiple embedded path with same storage path)
	}
}
