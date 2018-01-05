#ifndef StringCvt_h
#define StringCvt_h
#pragma once

#include "utl/Path.h"


namespace str
{
	// utilities for converting standard and utl types to MFC types (CString, etc)

	namespace traits
	{
		inline const TCHAR* GetStr( const CString& str ) { return str.GetString(); }
		inline size_t GetLength( const CString& str ) { return str.GetLength(); }

		inline const TCHAR* GetStr( const std::tstring& str ) { return str.c_str(); }
		inline size_t GetLength( const std::tstring& str ) { return str.length(); }

		inline const TCHAR* GetStr( const fs::CPath& path ) { return path.Get().c_str(); }
		inline size_t GetLength( const fs::CPath& path ) { return path.Get().length(); }
	}


	namespace cvt
	{
		// converts a source container of paths (std::tstring, fs::CPath, fs::CFlexPath) into a container of CString
		//
		template< typename MfcContainerT, typename SrcContainerT >
		MfcContainerT& MakeMfcStrings( MfcContainerT& rMfcItems, const SrcContainerT& srcItems )
		{
			for ( typename SrcContainerT::const_iterator itSrcItem = srcItems.begin(); itSrcItem != srcItems.end(); ++itSrcItem )
				rMfcItems.push_back( str::traits::GetStr( *itSrcItem ) );

			return rMfcItems;
		}

		// converts a source container of paths (std::tstring, fs::CPath, fs::CFlexPath) into a container of CString
		//
		template< typename ValueType, typename SrcContainerT >
		std::vector< ValueType >& MakeItemsAs( std::vector< ValueType >& rDestItems, const SrcContainerT& srcItems )
		{
			for ( typename SrcContainerT::const_iterator itSrcItem = srcItems.begin(); itSrcItem != srcItems.end(); ++itSrcItem )
				rDestItems.push_back( ValueType( str::traits::GetStr( *itSrcItem ) ) );

			return rDestItems;
		}
	}
}


#endif // StringCvt_h
