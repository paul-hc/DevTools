
#include "pch.h"
#include "TextClipboard.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const TCHAR CTextClipboard::s_lineEnd[] = _T("\r\n");

CTextClipboard::CTextClipboard( HWND hWnd )
	: m_hWnd( hWnd )
{
	ASSERT_PTR( m_hWnd );
}

CTextClipboard::~CTextClipboard()
{
	Close();
}

CTextClipboard* CTextClipboard::Open( HWND hWnd )
{
	return ::OpenClipboard( hWnd ) ? new CTextClipboard( hWnd ) : nullptr;
}

void CTextClipboard::Close( void )
{
	if ( m_hWnd != nullptr )
	{
		::CloseClipboard();
		m_hWnd = nullptr;
	}
}

bool CTextClipboard::WriteData( UINT clipFormat, const void* pBuffer, size_t byteCount )
{
	ASSERT( IsOpen() );

	// GHND, GMEM_MOVEABLE
	if ( HGLOBAL hBuffer = ::GlobalAlloc( GHND, byteCount + 128 ) )		// extra buffer size to prevent Explorer.exe from crashing
	{
		if ( void* pClipData = ::GlobalLock( hBuffer ) )
		{
			try
			{
				::CopyMemory( pClipData, pBuffer, byteCount );
				::GlobalUnlock( hBuffer );

				if ( ::SetClipboardData( clipFormat, hBuffer ) != nullptr )
					return true;
			}
			catch ( ... )
			{
				// either ::CopyMemory or GlobalUnlock can fail for large amounts of data, not clear why exactly
				// keep explorer.exe alive
			}
		}

		::GlobalFree( hBuffer );		// free the memory on failure
	}

	TRACE_FL( _T("Error on clipboard copy! Please retry.\n") );
	return false;
}

bool CTextClipboard::WriteString( const std::string& text )
{
	return WriteData( CF_TEXT, text.c_str(), ( text.size() + 1 ) * sizeof( char ) );
}

bool CTextClipboard::WriteString( const std::wstring& text )
{
	return WriteData( CF_UNICODETEXT, text.c_str(), ( text.size() + 1 ) * sizeof( wchar_t ) );
}

bool CTextClipboard::ReadString( std::tstring& rOutText ) const
{
	if ( IsFormatAvailable( CF_UNICODETEXT ) )
		if ( HGLOBAL hBuffer = ::GetClipboardData( CF_UNICODETEXT ) )
			if ( const TCHAR* pWideString = (const TCHAR*)GlobalLock( hBuffer ) )
			{
				rOutText = pWideString;
				::GlobalUnlock( hBuffer );
				return true;
			}

	if ( IsFormatAvailable( CF_TEXT ) )
		if ( HGLOBAL hBuffer = ::GetClipboardData( CF_TEXT ) )
			if ( const char* pUtf8String = (const char*)GlobalLock( hBuffer ) )
			{
				rOutText = str::FromUtf8( pUtf8String );
				::GlobalUnlock( hBuffer );
				return true;
			}

	return false;
}

bool CTextClipboard::DoCopyText( const std::string& utf8Text, const std::wstring& wideText, HWND hWnd, bool clear )
{
	std::auto_ptr<CTextClipboard> pClipboard( Open( hWnd ) );
	if ( nullptr == pClipboard.get() )
		return false;

	if ( clear )
		pClipboard->Clear();

	return
		pClipboard->WriteString( utf8Text.c_str() ) &&		// UTF8 (UNICODE)
		pClipboard->WriteString( wideText );				// WIDE (UNICODE)
}

bool CTextClipboard::CanPasteText( void )
{
	return IsFormatAvailable( CF_UNICODETEXT ) || IsFormatAvailable( CF_TEXT );
}

bool CTextClipboard::PasteText( std::tstring& rText, HWND hWnd )
{
	std::auto_ptr<CTextClipboard> pClipboard( Open( hWnd ) );
	return pClipboard.get() != nullptr && pClipboard->ReadString( rText );
}


// CTextClipboard::CMessageWnd implementation

CTextClipboard::CMessageWnd::CMessageWnd( void )
	: m_hWnd( ::CreateWindowEx( 0, L"Message", _T("<my_msgs>"), WS_POPUP, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, 0L ) )
{
}

void CTextClipboard::CMessageWnd::Destroy( void )
{
	if ( m_hWnd != nullptr )
	{
		::DestroyWindow( m_hWnd );
		m_hWnd = nullptr;
	}
}
