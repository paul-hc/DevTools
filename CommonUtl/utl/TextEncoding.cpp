
#include "stdafx.h"
#include "TextEncoding.h"
#include "TextFileIo.h"
#include "Path.h"
#include "RuntimeException.h"
#include "EnumTags.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


std::ostream& operator<<( std::ostream& os, const str::char32_t* pText32 )
{
	if ( NULL == pText32 )
		return os << "<NULL>";
	return os << str::ToWide( pText32 );
}

std::wostream& operator<<( std::wostream& os, const str::char32_t* pText32 )
{
	if ( NULL == pText32 )
		return os << "<NULL>";
	return os << str::ToWide( pText32 );
}


namespace fs
{
	size_t GetCharByteCount( Encoding encoding )
	{
		static const size_t s_byteCounts[] =
		{
			sizeof( char ), sizeof( char ),			// ANSI_UTF8, UTF8_bom
			sizeof( wchar_t ), sizeof( wchar_t ),	// UTF16_LE_bom, UTF16_be_bom
			sizeof( wchar_t ), sizeof( wchar_t )	// UTF32_LE_bom, UTF32_be_bom
		};

		ASSERT( encoding < _Encoding_Count );
		return s_byteCounts[ encoding ];
	}
}


namespace str
{
	// explicit specializations for char32_t

	template<>
	size_t GetLength( const char32_t* pText32 )
	{
		const char32_t* pEnd = pText32;
		while ( *pEnd++ != 0 )
		{
		}

		return std::distance( pText32, pEnd );
	}

	std::wstring ToWide( const char32_t* pText32 )
	{
		return std::wstring( pText32, pText32 + GetLength( pText32 ) );		// straight cast, e.g. for each: chWide = static_cast<wchar_t>( ch32 )
	}

	str::wstring4 FromWide( const wchar_t* pWide )
	{
		return str::wstring4( pWide, pWide + GetLength( pWide ) );
	}
}


namespace fs
{
	const CEnumTags& GetTags_Encoding( void )
	{
		static const CEnumTags s_tags( _T("ANSI/UTF8|UTF8|UTF16 LE (UCS2 LE)|UTF16 be (UCS2 BE)|UTF32 LE|UTF32 be"), _T("ANSI_UTF8|UTF8_bom|UTF16_LE_bom|UTF16_be_bom|UTF32_LE_bom|UTF32_be_bom") );
		return s_tags;
	}


	// CByteOrderMark implementation

	const BYTE CByteOrderMark::s_bom_UTF8[ 3 ]		= { 0xEF, 0xBB, 0xBF };
	const BYTE CByteOrderMark::s_bom_UTF16_LE[ 2 ]	= { 0xFF, 0xFE };
	const BYTE CByteOrderMark::s_bom_UTF16_be[ 2 ]	= { 0xFE, 0xFF };
	const BYTE CByteOrderMark::s_bom_UTF32_LE[ 4 ]	= { 0xFF, 0xFE, 0x00, 0x00 };
	const BYTE CByteOrderMark::s_bom_UTF32_be[ 4 ]	= { 0x00, 0x00, 0xFE, 0xFF };

	void CByteOrderMark::SetEncoding( Encoding encoding )
	{
		m_encoding = encoding;

		switch ( encoding )
		{
			default:			ASSERT( false );
			case ANSI_UTF8:		m_bom.clear(); break;
			case UTF8_bom:		m_bom.assign( s_bom_UTF8, END_OF( s_bom_UTF8 ) ); break;
			case UTF16_LE_bom:	m_bom.assign( s_bom_UTF16_LE, END_OF( s_bom_UTF16_LE ) ); break;
			case UTF16_be_bom:	m_bom.assign( s_bom_UTF16_be, END_OF( s_bom_UTF16_be ) ); break;
			case UTF32_LE_bom:	m_bom.assign( s_bom_UTF32_LE, END_OF( s_bom_UTF32_LE ) ); break;
			case UTF32_be_bom:	m_bom.assign( s_bom_UTF32_be, END_OF( s_bom_UTF32_be ) ); break;
		}
	}

	Encoding CByteOrderMark::ReadFromFile( const fs::CPath& srcFilePath ) throws_( CRuntimeException )
	{
		std::ifstream ifs( srcFilePath.GetPtr(), std::ios::in | std::ios::binary );
		io::CheckOpenForReading( ifs, srcFilePath );

		std::vector< char > filePrefix( BomMaxSize );

		ifs.read( &filePrefix.front(), BomMaxSize );
		filePrefix.resize( static_cast<size_t>( ifs.gcount() ) );

		if ( ifs.eof() )			// file smaller in size than BomMaxSize?
			ifs.clear();			// allow seeking past the BOM - otherwise seekg() fails

		SetEncoding( ParseBuffer( filePrefix ) );
		return m_encoding;
	}

	void CByteOrderMark::WriteToFile( const fs::CPath& targetFilePath ) const throws_( CRuntimeException )
	{
		std::ofstream ofs( targetFilePath.GetPtr(), std::ios::out | std::ios::trunc | std::ios::binary );
		io::CheckOpenForWriting( ofs, targetFilePath );

		if ( !IsEmpty() )
			ofs.write( &m_bom.front(), m_bom.size() );
	}

	Encoding CByteOrderMark::ParseBuffer( const std::vector< char >& filePrefix )
	{
		// match in descending order of BOM size
		if ( filePrefix.size() >= BomMinSize )
			if ( BomMatches( filePrefix, ARRAY_PAIR( s_bom_UTF32_LE ) ) )
				return UTF32_LE_bom;
			else if ( BomMatches( filePrefix, ARRAY_PAIR( s_bom_UTF32_be ) ) )
				return UTF32_be_bom;
			else if ( BomMatches( filePrefix, ARRAY_PAIR( s_bom_UTF8 ) ) )
				return UTF8_bom;
			else if ( BomMatches( filePrefix, ARRAY_PAIR( s_bom_UTF16_LE ) ) )
				return UTF16_LE_bom;
			else if ( BomMatches( filePrefix, ARRAY_PAIR( s_bom_UTF16_be ) ) )
				return UTF16_be_bom;

		return ANSI_UTF8;
	}

	bool CByteOrderMark::BomMatches( const std::vector< char >& filePrefix, const BYTE bom[], size_t bomCount )
	{
		// since filePrefix usually reads more characters than required, we have a match if the bom matches the leading part of the prefix
		return
			!filePrefix.empty() &&
			filePrefix.size() >= bomCount &&
			pred::Equal == std::memcmp( &filePrefix.front(), bom, std::min( bomCount, filePrefix.size() ) );
	}

	bool CByteOrderMark::BomEquals( const std::vector< char >& filePrefix, const BYTE bom[], size_t bomCount )
	{
		return
			filePrefix.size() == bomCount &&
			pred::Equal == std::memcmp( &filePrefix.front(), bom, bomCount );
	}
}


namespace io
{
	void __declspec(noreturn) ThrowUnsupportedEncoding( fs::Encoding encoding ) throws_( CRuntimeException )
	{
		throw CRuntimeException( str::Format( _T("Support for %s text file encoding not implemented"), fs::GetTags_Encoding().FormatUi( encoding ).c_str() ) );
	}


	ITextEncoder* MakeTextEncoder( io::IWriter* pWriter, fs::Encoding fileEncoding )
	{
		switch ( fileEncoding )
		{
			case fs::ANSI_UTF8:
			case fs::UTF8_bom:
				return new CUtf8Encoder( pWriter );
			case fs::UTF16_LE_bom:
				return new CStraightTextEncoder( pWriter );
			case fs::UTF16_be_bom:
				return new CSwapBytesTextEncoder( pWriter );
			default:
				io::ThrowUnsupportedEncoding( fileEncoding );
		}
	}


	// CStraightTextEncoder implementation

	io::IWriter* CStraightTextEncoder::GetWriter( void )
	{
		return m_pWriter;
	}

	void CStraightTextEncoder::WriteEncoded( const char* pText, size_t charCount )
	{
		m_pWriter->WriteString( pText, charCount );
	}

	void CStraightTextEncoder::WriteEncoded( const wchar_t* pText, size_t charCount )
	{
		m_pWriter->WriteString( pText, charCount );
	}


	// CSwapBytesTextEncoder implementation

	void CSwapBytesTextEncoder::WriteEncoded( const wchar_t* pText, size_t charCount )
	{	// make a copy, and swap bytes
		if ( charCount != 0 )
		{
			m_buffer.assign( pText, pText + charCount );
			std::for_each( m_buffer.begin(), m_buffer.end(), func::SwapBytes<endian::Little, endian::Big>() );
			__super::WriteEncoded( &m_buffer.front(), charCount );
		}
	}


	void CUtf8Encoder::WriteEncoded( const wchar_t* pText, size_t charCount )
	{	// make a UTF8 copy conversion
		if ( charCount != 0 )
		{
			m_utf8 = str::ToUtf8( pText, charCount );
			__super::WriteEncoded( &m_utf8[ 0 ], m_utf8.size() );
		}
	}
}
