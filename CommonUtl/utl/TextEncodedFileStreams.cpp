
#include "stdafx.h"
#include "TextEncodedFileStreams.h"
#include "RuntimeException.h"
#include "EnumTags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace io
{
	void __declspec( noreturn ) ThrowOpenForReading( const fs::CPath& filePath ) throws_( CRuntimeException )
	{
		throw CRuntimeException( str::Format( _T("Cannot open text file for reading: %s"), filePath.GetPtr() ) );
	}

	void __declspec( noreturn ) ThrowOpenForWriting( const fs::CPath& filePath ) throws_( CRuntimeException )
	{
		throw CRuntimeException( str::Format( _T("Cannot open text file for writing: %s"), filePath.GetPtr() ) );
	}

	void __declspec( noreturn ) ThrowUnsupportedEncoding( fs::Encoding encoding ) throws_( CRuntimeException )
	{
		throw CRuntimeException( str::Format( _T("Support for %s text file encoding not implemented"), fs::GetTags_Encoding().FormatUi( encoding ).c_str() ) );
	}


	// CBase_ifstream template specialization for reading char-s in text mode - only for ANSI_UTF8 encoding

	template<>
	void CBase_ifstream<char>::Open( const fs::CPath& filePath ) throws_( CRuntimeException )
	{
		m_filePath = filePath;
		m_lastCh = 0;

		// open in text mode - no translation required
		open( filePath.GetPtr(), std::ios::in, std::ios::_Openprot );
		io::CheckOpenForWriting( *this, m_filePath );

		Init();
	}

	template<>
	CBase_ifstream<char>::TBaseIStream& CBase_ifstream<char>::GetStream( void )		// only for text-mode standard output
	{
		return *this;
	}

	template<>
	char CBase_ifstream<char>::GetNext( void )
	{
		ASSERT( is_open() );
		ASSERT( !AtEnd() );

		char chr = 0;

		if ( !AtEnd() )
			chr = *m_itChar++;
		else
			ASSERT( false );		// at end?

		m_lastCh = chr;
		return chr;
	}

	template<>
	void CBase_ifstream<char>::GetStr( std::string& rText, char delim /*= 0*/ )
	{
		rText.clear();
		if ( delim != 0 )
			rText.reserve( m_streamCount );	// allocate space for the entire stream

		while ( !AtEnd() )
		{
			char chr = GetNext();

			if ( chr == delim )
				return;						// cut before the delimiter; current position after it

			rText.push_back( chr );
		}
	}


	// CBase_ofstream template specialization for writing char-s in text mode - only for ANSI_UTF8 encoding

	template<>
	void CBase_ofstream<char>::Open( const fs::CPath& filePath ) throws_( CRuntimeException )
	{
		m_filePath = filePath;
		m_lastCh = 0;
		m_bom.WriteToFile( m_filePath );

		// open in text/append mode (no translation required)
		open( m_filePath.GetPtr(), std::ios::out | std::ios::app, std::ios::_Openprot );
		io::CheckOpenForReading( *this, m_filePath );
	}

	template<>
	CBase_ofstream<char>::TBaseOStream& CBase_ofstream<char>::GetStream( void )
	{	// only for text-mode standard output
		return *this;
	}

	template<>
	void CBase_ofstream<char>::Append( char chr )
	{
		ASSERT( is_open() );
		put( chr );
		m_lastCh = chr;
	}

	template<>
	void CBase_ofstream<char>::Append( const char* pText, size_t count /*= utl::npos*/ )
	{
		ASSERT_PTR( pText );

		if ( utl::npos == count )
			count = str::GetLength( pText );

		if ( *pText != 0 && count != 0 )
		{
			write( pText, count );						// unformatted output
			m_lastCh = pText[ count - 1 ];				// keep track of the last character
		}
	}
}
