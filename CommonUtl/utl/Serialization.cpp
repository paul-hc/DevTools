
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
		if ( const int wideLength = static_cast<int>( wideStr.length() ) )
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


	class CArchiveGuts : public ::CArchive
	{
		CArchiveGuts( void );
	public:
		const BYTE* GetCursor( void ) const { return m_lpBufCur; }

		void Rewind( size_t bytes )
		{	// rollback the archive cursor to allow re-reading the bytes that were read
			ASSERT( IsLoading() );

			m_lpBufCur -= bytes;

			ENSURE( m_lpBufCur >= m_lpBufStart );		// no underflow
		}

		UnicodeEncoding InspectSavedStringEncoding( size_t* pLength = NULL )
		{
			const BYTE* pOldCursor = GetCursor();

			int savedCharSize;			// 1 = char, 2 = wchar_t
			size_t length = static_cast<size_t>( AfxReadStringLength( *this, savedCharSize ) );

			Rewind( m_lpBufCur - pOldCursor );			// go back to allow re-reading the string
			utl::AssignPtr( pLength, length );

			enum SavedCharSize { Char = 1, WideChar = 2 };
			return WideChar == savedCharSize ? WideEncoding : Utf8Encoding;
		}
	};


	// advanced archive utilities

	const BYTE* GetLoadingCursor( const ::CArchive& rLoadArchive )
	{
		const CArchiveGuts* pArchiveGuts = reinterpret_cast<const CArchiveGuts*>( &rLoadArchive );
		return pArchiveGuts->GetCursor();
	}

	void UnreadBytes( ::CArchive& rLoadArchive, size_t bytes )
	{
		CArchiveGuts* pArchiveGuts = reinterpret_cast<CArchiveGuts*>( &rLoadArchive );
		pArchiveGuts->Rewind( bytes );
	}

	UnicodeEncoding InspectSavedStringEncoding( ::CArchive& rLoadArchive, size_t* pLength /*= NULL*/ )
	{
		CArchiveGuts* pArchiveGuts = reinterpret_cast<CArchiveGuts*>( &rLoadArchive );
		return pArchiveGuts->InspectSavedStringEncoding( pLength );
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
