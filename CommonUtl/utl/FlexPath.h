#ifndef FlexPath_h
#define FlexPath_h
#pragma once

#include "Path.h"


namespace path
{
	std::tstring MakeShortHashedFilename( const TCHAR* pFullPath, size_t maxFnameExtLen );	// make short file name with length limited to maxFnameExtLen
}


namespace fs
{
	typedef fs::CPath TEmbeddedPath;


	// Path that works uniformly with normal paths or structured storage complex paths to embedded storages and streams.
	// Complex path: "physical_path>stg_embedded_path".
	// Examples:
	//	 "C:\Images\fruit.stg>apple.jpg": physical_path="C:\Images\fruit.stg", stg_embedded_path="apple.jpg"
	//	 "C:\Images\colors.stg>Pastel\lime.jpg": physical_path="C:\Images\colors.stg", stg_embedded_path="Pastel\lime.jpg"

	class CFlexPath : public CPath
	{
	public:
		CFlexPath( void ) {}
		CFlexPath( const std::tstring& filePath ) : CPath( filePath ) { ASSERT( path::IsWellFormed( filePath.c_str() ) ); }

		static CFlexPath MakeComplexPath( const fs::CPath& physicalPath, const fs::TEmbeddedPath& embeddedPath ) { return path::MakeComplex( physicalPath.Get(), embeddedPath.GetPtr() ); }
		bool SplitComplexPath( fs::CPath& rPhysicalPath, TEmbeddedPath& rEmbeddedPath ) const { return path::SplitComplex( rPhysicalPath.Ref(), rEmbeddedPath.Ref(), Get() ); }

		fs::CPath GetPhysicalPath( void ) const { return path::ExtractPhysical( Get() ); }
		fs::TEmbeddedPath GetEmbeddedPath( void ) const { return fs::TEmbeddedPath( GetEmbeddedPathPtr() ); }
		const TCHAR* GetEmbeddedPathPtr( void ) const { return path::GetEmbedded( GetPtr() ); }

		const TCHAR* GetLeafSubPath( void ) const { return IsComplexPath() ? GetEmbeddedPathPtr() : GetNameExt(); }
		fs::CPath GetOriginParentPath( void ) const { return IsComplexPath() ? GetPhysicalPath() : GetParentPath(); }		// storage path or parent directory path

		CFlexPath GetParentFlexPath( bool trailSlash = false ) const;

		std::tstring FormatPretty( void ) const;		// "C:\Images\fruit.stg>StgDir/apple.jpg" <- "C:\Images/fruit.stg>StgDir\apple.jpg"
		std::tstring FormatPrettyLeaf( void ) const;	// "StgDir/apple.jpg";  "C:\Images\orange.png" -> "orange.png" <- "C:\Images/fruit.stg>StgDir\apple.jpg"

		bool FileExist( AccessMode accessMode = Exist ) const { return fs::FileExist( path::ExtractPhysical( Get() ).c_str(), accessMode ); }
		bool FlexFileExist( AccessMode accessMode = Exist ) const;
	};


	inline CFlexPath ToFlexPath( const CPath& path ) { return CFlexPath( path.Get() ); }									// convenience downcast
	inline const CFlexPath& CastFlexPath( const CPath& path ) { return reinterpret_cast< const CFlexPath& >( path ); }		// convenience reference downcast
}


namespace path
{
	void QueryPhysicalPaths( std::vector< fs::CPath >& rPhysicalPaths, const std::vector< fs::CFlexPath >& flexPaths );
	void ConvertToPhysicalPaths( IN OUT std::vector< fs::CFlexPath >& rFlexPaths );			// strip-out complex paths to physical paths (retaining existing physical paths)


	template< typename PathT >
	inline size_t GetComplexPathCount( const std::vector< PathT >& flexPaths )
	{
		return std::count_if( flexPaths.begin(), flexPaths.end(), std::mem_fun_ref( &fs::CPath::IsComplexPath ) );
	}

	template< typename PathT >
	inline size_t GetPhysicalPathCount( const std::vector< PathT >& flexPaths )
	{
		return std::count_if( flexPaths.begin(), flexPaths.end(), std::mem_fun_ref( &fs::CPath::IsPhysicalPath ) );
	}

	template< typename PathT >
	bool ExtractPhysicalPaths( OUT std::vector< fs::CPath >& rPhysicalPaths, IN OUT std::vector< PathT >& rFlexPaths )
	{
		typename std::vector< PathT >::iterator itRemove = std::remove_if( rFlexPaths.begin(), rFlexPaths.end(), std::mem_fun_ref( &fs::CPath::IsPhysicalPath ) );	// just move physical paths at the end

		if ( (void*)&rPhysicalPaths != (void*)&rFlexPaths )
			rPhysicalPaths.assign( itRemove, rFlexPaths.end() );

		rFlexPaths.erase( itRemove, rFlexPaths.end() );		// OUT: leave only the complex paths
		return !rPhysicalPaths.empty();
	}
}


namespace stdext
{
	inline size_t hash_value( const fs::CFlexPath& path ) { return path.GetHashValue(); }
}


namespace pred
{
	struct FlexFileExist
	{
		FlexFileExist( fs::AccessMode accessMode = fs::Exist ) : m_accessMode( accessMode ) {}

		template< typename PathKey >
		bool operator()( const PathKey& pathKey ) const
		{
			return fs::CastFlexPath( func::PathOf( pathKey ) ).FlexFileExist( m_accessMode );
		}
	private:
		fs::AccessMode m_accessMode;
	};
}


#endif // FlexPath_h
