
#include "stdafx.h"
#include "Serialization.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace serial
{
	// CPolicy implementation

	UnicodeEncoding CPolicy::s_strEncoding = Utf8Encoding;		// more compact and readable strings

	void CPolicy::ToUtf8String( CStringA& rUtf8Str, const std::wstring& wideStr )
	{
		if ( const int wideLength = static_cast< int >( wideStr.length() ) )
			if ( int length = ::WideCharToMultiByte( CP_UTF8, 0, wideStr.c_str(), wideLength, NULL, 0, NULL, NULL ) )		// get length of the new multibyte string (excluding EOS)
			{
				if ( char* pBuffer = rUtf8Str.GetBuffer( length ) )
					::WideCharToMultiByte( CP_UTF8, 0, wideStr.c_str(), wideLength, pBuffer, length, 0, 0 );				// convert to UTF8
				rUtf8Str.ReleaseBuffer();
				return;
			}

		rUtf8Str.Empty();
	}

	void CPolicy::FromUtf8String( std::wstring& rWideStr, const CStringA& utf8Str )
	{
		if ( int narrowLength = utf8Str.GetLength() )
			if ( int requiredSize = ::MultiByteToWideChar( CP_UTF8, 0, utf8Str.GetString(), narrowLength, NULL, 0 ) )
			{
				rWideStr.resize( requiredSize, L'\0' );
				::MultiByteToWideChar( CP_UTF8, 0, utf8Str.GetString(), narrowLength, &rWideStr[ 0 ], requiredSize );
				return;
			}

		rWideStr.clear();
	}
}


CArchive& operator<<( CArchive& archive, const std::wstring& wideStr )
{
	using namespace serial;

	switch ( CPolicy::s_strEncoding )
	{
		case WideEncoding:
			return archive << CStringW( wideStr.c_str() );			// save as Wide
		case Utf8Encoding:
			return archive << &wideStr;								// save as Utf8
	}

	ASSERT( false );
	return archive;
}

CArchive& operator>>( CArchive& archive, std::wstring& rWideStr )
{
	using namespace serial;

	switch ( CPolicy::s_strEncoding )
	{
		case WideEncoding:
		{
			CStringW wideStr;
			archive >> wideStr;										// load as Wide
			rWideStr = wideStr.GetString();
			break;
		}
		case Utf8Encoding:
			archive >> &rWideStr;									// load as Utf8
			break;
		default:
			ASSERT( false );
	}

	return archive;
}

CArchive& operator<<( CArchive& archive, const std::wstring* pWideStr )			// Unicode string as UTF8 (for better readability in archive stream)
{
	ASSERT_PTR( pWideStr );

	CStringA utf8Str;
	serial::CPolicy::ToUtf8String( utf8Str, *pWideStr );
	return archive << utf8Str;								// as Utf8
}

CArchive& operator>>( CArchive& archive, std::wstring* pOutWideStr )			// Unicode string as UTF8 (for better readability in archive stream)
{
	ASSERT_PTR( pOutWideStr );

	CStringA utf8Str;
	archive >> utf8Str;
	serial::CPolicy::FromUtf8String( *pOutWideStr, utf8Str );
	return archive;
}
