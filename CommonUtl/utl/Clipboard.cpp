
#include "stdafx.h"
#include "Clipboard.h"
#include "utl/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const TCHAR CClipboard::s_lineEnd[] = _T("\r\n");

CClipboard::CClipboard( CWnd* pWnd )
	: m_pWnd( pWnd )
{
	ASSERT_PTR( m_pWnd );
}

CClipboard::~CClipboard()
{
	Close();
}

CClipboard* CClipboard::Open( CWnd* pWnd /*= AfxGetMainWnd()*/ )
{
	ASSERT_PTR( pWnd->GetSafeHwnd() );
	return pWnd->OpenClipboard() ? new CClipboard( pWnd ) : NULL;
}

void CClipboard::Close( void )
{
	if ( m_pWnd != NULL )
	{
		::CloseClipboard();
		m_pWnd = NULL;
	}
}

bool CClipboard::WriteData( UINT clipFormat, const void* pBuffer, size_t byteCount )
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

				if ( ::SetClipboardData( clipFormat, hBuffer ) != NULL )
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

	AfxMessageBox( _T("Error on clipboard copy!\r\nPlease retry."), MB_ICONERROR );
	return false;
}

bool CClipboard::WriteString( const std::string& text )
{
	return WriteData( CF_TEXT, text.c_str(), ( text.size() + 1 ) * sizeof( char ) );
}

bool CClipboard::WriteString( const std::wstring& text )
{
	return WriteData( CF_UNICODETEXT, text.c_str(), ( text.size() + 1 ) * sizeof( wchar_t ) );
}

bool CClipboard::ReadString( std::tstring& rOutText ) const
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

bool CClipboard::CopyText( const std::tstring& text, CWnd* pWnd /*= AfxGetMainWnd()*/ )
{
	std::string utf8Text = str::ToUtf8( text.c_str() );

	std::auto_ptr< CClipboard > pClipboard( Open( pWnd ) );
	if ( NULL == pClipboard.get() )
		return false;

	pClipboard->Clear();

	return
		pClipboard->WriteString( utf8Text.c_str() ) &&		// UTF8 (UNICODE)
		pClipboard->WriteString( text );					// WIDE (UNICODE)
}

bool CClipboard::CanPasteText( void )
{
	return IsFormatAvailable( CF_UNICODETEXT ) || IsFormatAvailable( CF_TEXT );
}

bool CClipboard::PasteText( std::tstring& rText, CWnd* pWnd /*= AfxGetMainWnd()*/ )
{
	std::auto_ptr< CClipboard > pClipboard( Open( pWnd ) );
	return pClipboard.get() != NULL && pClipboard->ReadString( rText );
}
