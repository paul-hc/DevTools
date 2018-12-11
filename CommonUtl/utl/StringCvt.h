#ifndef StringCvt_h
#define StringCvt_h
#pragma once

#include "utl/Path.h"


namespace str
{
	namespace traits		// utils for converting stringy (string-like) types to character-ptr
	{
		inline const char* GetCharPtr( const CStringA& text ) { return text.GetString(); }
		inline const wchar_t* GetCharPtr( const CStringW& text ) { return text.GetString(); }
	}

	namespace traits		// utils for converting stringy (string-like) types to character-ptr
	{
		inline size_t GetLength( const CStringA& text ) { return text.GetLength(); }
		inline size_t GetLength( const CStringW& text ) { return text.GetLength(); }
	}
}


namespace str
{
	// utilities for converting standard and utl types to MFC types (CString, etc)

	namespace cvt
	{
		// converts a source container of paths (std::tstring, fs::CPath, fs::CFlexPath) into a container of CString
		//
		template< typename MfcContainerT, typename SrcContainerT >
		MfcContainerT& MakeMfcStrings( MfcContainerT& rMfcItems, const SrcContainerT& srcItems )
		{
			for ( typename SrcContainerT::const_iterator itSrcItem = srcItems.begin(); itSrcItem != srcItems.end(); ++itSrcItem )
				rMfcItems.push_back( str::traits::GetCharPtr( *itSrcItem ) );

			return rMfcItems;
		}

		// converts a source container of paths (std::tstring, fs::CPath, fs::CFlexPath) into a container of CString
		//
		template< typename ValueType, typename SrcContainerT >
		std::vector< ValueType >& MakeItemsAs( std::vector< ValueType >& rDestItems, const SrcContainerT& srcItems )
		{
			for ( typename SrcContainerT::const_iterator itSrcItem = srcItems.begin(); itSrcItem != srcItems.end(); ++itSrcItem )
				rDestItems.push_back( ValueType( str::traits::GetCharPtr( *itSrcItem ) ) );

			return rDestItems;
		}
	}
}


#endif // StringCvt_h
