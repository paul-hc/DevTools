
#include "stdafx.h"
#include "StringBase.h"
#include <comdef.h>			// _com_error

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace utl
{
	HRESULT Audit( HRESULT hResult, const char* pFuncName )
	{
		pFuncName;
		if ( !SUCCEEDED( hResult ) )
			TRACE( " * %s: hResult=0x%08x: '%s' in function %s\n", FAILED( hResult ) ? "FAILED" : "ERROR", hResult, CStringA( _com_error( hResult ).ErrorMessage() ).GetString(), pFuncName );
		return hResult;
	}
}


std::ostream& operator<<( std::ostream& os, const wchar_t* pWide )
{
	if ( NULL == pWide )
		return os << "<NULL>";
	return os << str::ToUtf8( pWide );
}

std::wostream& operator<<( std::wostream& os, const char* pUtf8 )
{
	if ( NULL == pUtf8 )
		return os << L"<NULL>";
	return os << str::FromUtf8( pUtf8 );
}

std::ostream& operator<<( std::ostream& os, wchar_t chWide )
{
	const wchar_t strWide[] = { chWide, _T('\0') };
	return os << strWide;
}

std::wostream& operator<<( std::wostream& os, char chUtf8 )
{
	const char strUtf8[] = { chUtf8, '\0' };
	return os << strUtf8;
}


namespace hlp
{
	std::string ToNarrow( const wchar_t* pWide, UINT codePage )
	{	// codePage: CP_UTF8/CP_ACP
		if ( str::IsEmpty( pWide ) )
			return std::string();

		int wideLength = (int)wcslen( pWide );
		int requiredSize = ::WideCharToMultiByte( codePage, 0, pWide, wideLength, NULL, 0, NULL, NULL );

		std::string narrow( requiredSize, '\0' );
		::WideCharToMultiByte( codePage, 0, pWide, wideLength, &narrow[ 0 ], requiredSize, NULL, NULL );
		return narrow;
	}

	std::wstring ToWide( const char* pNarrow, UINT codePage )
	{	// codePage: CP_UTF8/CP_ACP
		if ( str::IsEmpty( pNarrow ) )
			return std::wstring();

		int narrowLength = (int)strlen( pNarrow );
		int requiredSize = ::MultiByteToWideChar( codePage, 0, pNarrow, narrowLength, NULL, 0 );

		std::wstring wide( requiredSize, L'\0' );
		::MultiByteToWideChar( codePage, 0, pNarrow, narrowLength, &wide[ 0 ], requiredSize );
		return wide;
	}
}


namespace str
{
	template<> const char* StdWhitespace< char >( void ) { return " \t\r\n"; }
	template<> const wchar_t* StdWhitespace< wchar_t >( void ) { return L" \t\r\n"; }


	const std::locale& GetUserLocale( void )
	{
		static const std::locale userLocale( "" );		// user-specific locale
		return userLocale;
	}

	const std::tstring& GetEmpty( void )
	{
		static const std::tstring s_empty;
		return s_empty;
	}

	std::string ToAnsi( const wchar_t* pWide )
	{
		return hlp::ToNarrow( pWide, CP_ACP );
	}

	std::wstring FromAnsi( const char* pAnsi )
	{
		return hlp::ToWide( pAnsi, CP_ACP );
	}

	std::string ToUtf8( const wchar_t* pWide )
	{
		return hlp::ToNarrow( pWide, CP_UTF8 );
	}

	std::wstring FromUtf8( const char* pUtf8 )
	{
		return hlp::ToWide( pUtf8, CP_UTF8 );
	}


	namespace win
	{
		std::string ToAnsi( const wchar_t* pWide )
		{
			std::string text;
			if ( pWide != NULL )
			{
				USES_CONVERSION;
				text = W2A( pWide );
			}
			return text;
		}

		std::wstring FromAnsi( const char* pAnsi )
		{
			std::wstring text;
			if ( pAnsi != NULL )
			{
				USES_CONVERSION;
				text = A2W( pAnsi );
			}
			return text;
		}

		// problems with UTF8 conversion; ex. 'x266D': music flat symbol (bemol)

		std::string ToUtf8( const wchar_t* pWide )
		{
			std::string utf8;
			if ( !str::IsEmpty( pWide ) )
			{
				size_t size = wcslen( pWide ) + 1;
				char* pUtf8 = new char[ size ];
				size_t convertedChars = 0;
				wcstombs_s( &convertedChars, pUtf8, size, pWide, _TRUNCATE );		// WIDE -> UTF8
				utf8 = pUtf8;
				delete[] pUtf8;
			}
			return utf8;
		}

		std::wstring FromUtf8( const char* pUtf8 )
		{
			std::wstring wide;
			if ( !str::IsEmpty( pUtf8 ) )
			{
				size_t size = strlen( pUtf8 ) + 1;
				std::vector< wchar_t > wideBuff( size );
				size_t convertedChars = 0;
				mbstowcs_s( &convertedChars, &wideBuff.front(), size, pUtf8, _TRUNCATE );		// UTF8 -> WIDE
				wide = &wideBuff.front();
			}
			return wide;
		}
	}

	std::string AsNarrow( const std::tstring& text )
	{
		#ifdef _UNICODE
			return ToUtf8( text.c_str() );
		#else
			return text;
		#endif
	}

	std::wstring AsWide( const std::tstring& text )
	{
		#ifdef _UNICODE
			return text;
		#else
			return FromUtf8( text.c_str() );
		#endif
	}


	std::tstring Format( const TCHAR* pFormat, ... )
	{
		va_list argList;

		va_start( argList, pFormat );

		CString message;
		message.FormatV( pFormat, argList );

		va_end( argList );

		return message.GetString();
	}

	std::tstring Format( UINT formatId, ... )
	{
		va_list argList;

		va_start( argList, formatId );

		CString format, message;
		format.LoadString( AfxGetResourceHandle(), formatId );

		message.FormatV( (const TCHAR*)format, argList );
		va_end( argList );

		return message.GetString();
	}

	std::tstring Load( UINT strId, bool* pLoaded /*= NULL*/ )
	{
		CString text;
		bool loaded = text.LoadString( AfxGetResourceHandle(), strId ) != FALSE;
		if ( pLoaded != NULL )
			*pLoaded = loaded;
		return text.GetString();
	}

	std::vector< std::tstring > LoadStrings( UINT strId, const TCHAR* pSep /*= _T("|")*/, bool* pLoaded /*= NULL*/ )
	{
		std::vector< std::tstring > items;
		Split( items, Load( strId, pLoaded ).c_str(), pSep );
		return items;
	}

	std::pair< std::tstring, std::tstring > LoadPair( UINT strId, const TCHAR* pSep /*= _T("|")*/, bool* pLoaded /*= NULL*/ )
	{
		std::tstring text = Load( strId, pLoaded );
		size_t sepPos = text.find( pSep );
		if ( std::tstring::npos == sepPos )
		{
			ASSERT( false );
			return std::make_pair( text, std::tstring() );
		}
		return std::make_pair( text.substr( 0, sepPos ), text.substr( sepPos + 1 ) );
	}


	namespace mfc
	{
	#ifdef _MFC_VER
		CString Load( UINT strId )
		{
			CString text;
			text.LoadString( AfxGetResourceHandle(), strId );
			return text;
		}

		CString GetTypeName( const CObject* pObject )
		{
			if ( pObject != NULL )
				if ( CRuntimeClass* pRuntimeClass = pObject->GetRuntimeClass() )
					return CString( pRuntimeClass->m_lpszClassName );

			return _T("<null_ptr>");
		}
	#endif //_MFC_VER
	}


	std::tstring GetTypeName( const type_info& info )
	{
		const char* pName = info.name();
		if ( const char* pCoreName = strchr( pName, ' ' ) )
			pName = pCoreName + 1;

		std::tstring name( str::FromAnsi( pName ) );
		str::Replace( name, _T(" "), _T("") );
		return name;
	}
}


namespace stream
{
	bool Tag( std::tstring& rOutput, const std::tstring& tag, const TCHAR* pPrefixSep )
	{
		if ( tag.empty() )
			return false;

		if ( !rOutput.empty() && !str::IsEmpty( pPrefixSep ) )
			rOutput += pPrefixSep;

		rOutput += tag;
		return true;
	}

	std::tstring InputLine( std::istream& is )
	{
		std::string line;
		std::getline( is, line );
		return str::FromUtf8( line.c_str() );
	}
}
