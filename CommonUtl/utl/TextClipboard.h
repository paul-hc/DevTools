#ifndef TextClipboard_h
#define TextClipboard_h
#pragma once


class CTextClipboard
{
protected:
	CTextClipboard( HWND hWnd );
public:
	~CTextClipboard();

	static CTextClipboard* Open( HWND hWnd );

	void Close( void );
	bool IsOpen( void ) const { return m_hWnd != NULL; }

	void Clear( void ) { ASSERT( IsOpen() ); ::EmptyClipboard(); }

	HGLOBAL GetData( UINT clipFormat ) const { ASSERT( IsOpen() ); return IsFormatAvailable( clipFormat ) ? ::GetClipboardData( clipFormat ) : NULL; }
	bool SetData( UINT clipFormat, HGLOBAL hGlobal ) { ASSERT( IsOpen() ); return ::SetClipboardData( clipFormat, hGlobal ) != NULL; }

	bool WriteData( UINT clipFormat, const void* pBuffer, size_t byteCount );

	bool WriteString( const std::string& text );
	bool WriteString( const std::wstring& text );
	bool ReadString( std::tstring& rOutText ) const;

	template< typename ScalarT >
	bool Write( UINT clipFormat, const ScalarT& value );

	template< typename ScalarT >
	bool Read( UINT clipFormat, ScalarT& rOutValue ) const;
public:
	// service
	static bool IsFormatAvailable( UINT clipFormat ) { return ::IsClipboardFormatAvailable( clipFormat ) != FALSE; }
	static bool CanPasteText( void );

	static bool CopyText( const std::tstring& text, HWND hWnd, bool clear = true );
	static bool PasteText( std::tstring& rText, HWND hWnd );

	template< typename ContainerT >
	static bool CopyToLines( const ContainerT& textItems, HWND hWnd, const TCHAR* pLineEnd = s_lineEnd, bool clear = true );

	template< typename StringT >
	static bool PasteFromLines( std::vector< StringT >& rTextItems, HWND hWnd, const TCHAR* pLineEnd = s_lineEnd );
public:
	class CMessageWnd : private utl::noncopyable		// lightweight message window wrapper for clipboard IO in console applications
	{
	public:
		CMessageWnd( void );
		~CMessageWnd() { Destroy(); }

		HWND GetWnd( void ) const { return m_hWnd; }
		bool IsValid( void ) const { return m_hWnd != NULL; }

		void Destroy( void );
	private:
		HWND m_hWnd;
	};

private:
	HWND m_hWnd;
public:
	static const TCHAR s_lineEnd[];
};


// template code

template< typename ScalarT >
inline bool CTextClipboard::Write( UINT clipFormat, const ScalarT& value )
{
	return WriteData( clipFormat, &value, sizeof( value ) );
}

template< typename ScalarT >
inline bool CTextClipboard::Read( UINT clipFormat, ScalarT& rOutValue ) const
{
	if ( IsFormatAvailable( clipFormat ) )
		if ( HGLOBAL hBuffer = ::GetClipboardData( clipFormat ) )
			if ( const ScalarT* pValue = (const ScalarT*)::GlobalLock( hBuffer ) )
			{
				rOutValue = *pValue;
				::GlobalUnlock( hBuffer );
				return true;
			}

	return false;
}

template< typename ContainerT >
inline bool CTextClipboard::CopyToLines( const ContainerT& textItems, HWND hWnd, const TCHAR* pLineEnd /*= s_lineEnd*/, bool clear /*= true*/ )
{
	return CopyText( str::JoinLines( textItems, pLineEnd ), hWnd, clear );
}

template< typename StringT >
bool CTextClipboard::PasteFromLines( std::vector< StringT >& rTextItems, HWND hWnd, const TCHAR* pLineEnd /*= s_lineEnd*/ )
{
	std::tstring text;
	if ( !PasteText( text, hWnd ) )
	{
		rTextItems.clear();
		return false;
	}

	str::SplitLines( rTextItems, text.c_str(), pLineEnd );
	return true;
}


#endif // TextClipboard_h