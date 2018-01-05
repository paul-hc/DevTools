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
	// path that works uniformly with normal paths or structured storage complex paths to embedded storages and streams;
	// complex path: "physical_path>stg_embedded_path"; example "C:\Images\fruit.stg>apple.jpg": physical_path="C:\Images\fruit.stg", stg_embedded_path="apple.jpg"

	class CFlexPath : public CPath
	{
	public:
		CFlexPath( void ) {}
		CFlexPath( const std::tstring& filePath ) : CPath( filePath ) { ASSERT( path::IsWellFormed( filePath.c_str() ) ); }

		static CFlexPath MakeComplexPath( const std::tstring& physicalPath, const TCHAR* pEmbeddedPath ) { return path::MakeComplex( physicalPath, pEmbeddedPath ); }
		bool SplitComplexPath( std::tstring& rPhysicalPath, std::tstring& rEmbeddedPath ) const { return path::SplitComplex( rPhysicalPath, rEmbeddedPath, Get() ); }

		std::tstring GetPhysicalPath( void ) const { return path::GetPhysical( Get() ); }
		const TCHAR* GetEmbeddedPath( void ) const { return path::GetEmbedded( GetPtr() ); }

		const TCHAR* GetLeafSubPath( void ) const { return IsComplexPath() ? GetEmbeddedPath() : GetNameExt(); }
		std::tstring GetParentPath( void ) const { return IsComplexPath() ? GetPhysicalPath() : GetDirPath(); }

		std::tstring FormatPretty( void ) const;		// "C:\Images\fruit.stg>StgDir/apple.jpg" <- "C:\Images/fruit.stg>StgDir\apple.jpg"
		std::tstring FormatPrettyLeaf( void ) const;	// "StgDir/apple.jpg";  "C:\Images\orange.png" -> "orange.png" <- "C:\Images/fruit.stg>StgDir\apple.jpg"

		bool FileExist( AccessMode accessMode = Exist ) const { return fs::FileExist( path::GetPhysical( Get() ).c_str(), accessMode ); }
		bool FlexFileExist( AccessMode accessMode = Exist ) const;
	};


	inline CFlexPath ToFlexPath( const CPath& path ) { return CFlexPath( path.Get() ); }									// convenience downcast
	inline const CFlexPath& CastFlexPath( const CPath& path ) { return reinterpret_cast< const CFlexPath& >( path ); }		// convenience reference downcast
}


namespace stdext
{
	inline size_t hash_value( const fs::CFlexPath& path ) { return path.hash_value(); }
}


#endif // FlexPath_h
