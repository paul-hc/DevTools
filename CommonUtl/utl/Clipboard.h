#ifndef Clipboard_h
#define Clipboard_h
#pragma once


class CClipboard
{
	CClipboard( CWnd* pWnd );
public:
	~CClipboard();

	static CClipboard* Open( CWnd* pWnd = AfxGetMainWnd() );
	void Close( void );
	bool IsOpen( void ) const { return m_pWnd != NULL; }

	void Clear( void );

	HGLOBAL GetData( UINT clipFormat ) const { ASSERT( IsOpen() ); return IsFormatAvailable( clipFormat ) ? ::GetClipboardData( clipFormat ) : NULL; }
	bool SetData( UINT clipFormat, HGLOBAL hGlobal ) { ASSERT( IsOpen() ); return ::SetClipboardData( clipFormat, hGlobal ) != NULL; }

	bool WriteData( UINT clipFormat, const void* pBuffer, size_t byteCount );

	bool WriteString( const std::string& text );
	bool WriteString( const std::wstring& text );
	bool ReadString( std::tstring& rOutText ) const;

	template< typename ScalarType >
	bool Write( UINT clipFormat, const ScalarType& value );

	template< typename ScalarType >
	bool Read( UINT clipFormat, ScalarType& rOutValue ) const;

	static bool IsFormatAvailable( UINT clipFormat ) { return ::IsClipboardFormatAvailable( clipFormat ) != FALSE; }

	static bool CanPasteText( void );
	static bool PasteText( std::tstring& rText, CWnd* pWnd = AfxGetMainWnd() );
	static bool CopyText( const std::tstring& text, CWnd* pWnd = AfxGetMainWnd() );
private:
	CWnd* m_pWnd;
};


// inline code

inline void CClipboard::Clear( void )
{
	ASSERT( IsOpen() );
	EmptyClipboard();
}

template< typename ScalarType >
inline bool CClipboard::Write( UINT clipFormat, const ScalarType& value )
{
	return WriteData( clipFormat, &value, sizeof( value ) );
}

template< typename ScalarType >
inline bool CClipboard::Read( UINT clipFormat, ScalarType& rOutValue ) const
{
	if ( IsFormatAvailable( clipFormat ) )
		if ( HGLOBAL hBuffer = ::GetClipboardData( clipFormat ) )
			if ( const ScalarType* pValue = (const ScalarType*)GlobalLock( hBuffer ) )
			{
				rOutValue = *pValue;
				GlobalUnlock( hBuffer );
				return true;
			}

	return false;
}


#endif // Clipboard_h
