#ifndef TextEncoding_h
#define TextEncoding_h
#pragma once

#include "Encoding.h"


namespace str
{
	std::wstring ToWide( const char32_t* p32 );
	str::wstring4 FromWide( const wchar_t* pWide );
}


std::ostream& operator<<( std::ostream& os, const str::char32_t* pText32 );
std::wostream& operator<<( std::wostream& os, const str::char32_t* pText32 );

inline std::ostream& operator<<( std::ostream& os, const str::wstring4& text32 ) { return os << text32.c_str(); }
inline std::wostream& operator<<( std::wostream& os, const str::wstring4& text32 ) { return os << text32.c_str(); }


namespace fs
{
	class CByteOrderMark		// BOM object for UTF encoding
	{
	public:
		explicit CByteOrderMark( Encoding encoding = ANSI_UTF8 ) { SetEncoding( encoding ); }

		Encoding GetEncoding( void ) const { return m_encoding; }
		void SetEncoding( Encoding encoding );

		bool IsEmpty( void ) const { return m_bom.empty(); }
		const std::vector< char >& Get( void ) const { return m_bom; }
		size_t GetCharCount( void ) const { return m_bom.size() / fs::GetCharByteCount( m_encoding ); }		// count of BOM in encoding_chars (not BYTES for wide encodings)

		bool operator==( const CByteOrderMark& right ) const { return m_encoding == right.m_encoding && m_bom == right.m_bom; }

		Encoding ReadFromFile( const fs::CPath& srcFilePath ) throws_( CRuntimeException );
		void WriteToFile( const fs::CPath& targetFilePath ) const throws_( CRuntimeException );

		template< typename istream_T >
		istream_T& SeekAfterBom( istream_T& is ) const;

		Encoding ParseBuffer( const std::vector< char >& filePrefix );
	private:
		static bool BomMatches( const std::vector< char >& filePrefix, const BYTE bom[], size_t bomCount );
		static bool BomEquals( const std::vector< char >& filePrefix, const BYTE bom[], size_t bomCount );
	private:
		Encoding m_encoding;
		std::vector< char > m_bom;
	private:
		static const BYTE s_bom_UTF8[ 3 ];		// { 0xEF, 0xBB, 0xBF };
		static const BYTE s_bom_UTF16_LE[ 2 ];	// { 0xFF, 0xFE }; aka UCS2 LE
		static const BYTE s_bom_UTF16_be[ 2 ];	// { 0xFE, 0xFF }; aka UCS2 BE
		static const BYTE s_bom_UTF32_LE[ 4 ];	// { 0xFF, 0xFE, 0x00, 0x00 };
		static const BYTE s_bom_UTF32_be[ 4 ];	// { 0x00, 0x00, 0xFE, 0xFF };

		enum { BomMinSize = COUNT_OF( s_bom_UTF16_LE ), BomMaxSize = COUNT_OF( s_bom_UTF32_LE ) };
	};


	// template code

	template< typename istream_T >
	istream_T& CByteOrderMark::SeekAfterBom( istream_T& is ) const
	{
		REQUIRE( fs::GetCharByteCount( m_encoding ) == sizeof( typename istream_T::char_type ) );		// compatible stream type?
		REQUIRE( is.good() );
		REQUIRE( 0 == is.tellg() );		// should be the first read

		is.seekg( GetCharCount() );		// position next read after BOM
		return is;
	}
}


#endif // TextEncoding_h
