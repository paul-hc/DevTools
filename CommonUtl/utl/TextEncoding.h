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


namespace io
{
	void __declspec(noreturn) ThrowUnsupportedEncoding( fs::Encoding encoding ) throws_( CRuntimeException );
	void __declspec(noreturn) ThrowOpenForReading( const fs::CPath& filePath ) throws_( CRuntimeException );
	void __declspec(noreturn) ThrowOpenForWriting( const fs::CPath& filePath ) throws_( CRuntimeException );


	// does low-level writing to output; implemented by text writers (e.g. console, files, output streams)
	//
	interface IWriter : public utl::IMemoryManaged
	{
		virtual void WriteString( const char* pText, size_t charCount ) = 0;
		virtual void WriteString( const wchar_t* pText, size_t charCount ) = 0;
	};
}


namespace io
{
	// writes encoded strings to an IWriter-based output; handles UTF conversions (multi-byte, wide) and byte-swapping
	//
	interface ITextEncoder : public utl::IMemoryManaged
	{
		virtual io::IWriter* GetWriter( void ) = 0;
		virtual void WriteEncoded( const char* pText, size_t charCount ) = 0;
		virtual void WriteEncoded( const wchar_t* pText, size_t charCount ) = 0;
	};


	ITextEncoder* MakeTextEncoder( io::IWriter* pWriter, fs::Encoding fileEncoding );


	class CStraightTextEncoder : public ITextEncoder
	{	// no encoding, just invokes the writer
	public:
		CStraightTextEncoder( io::IWriter* pWriter ) : m_pWriter( pWriter ) { ASSERT_PTR( m_pWriter ); }

		// ITextEncoder interface
		virtual io::IWriter* GetWriter( void );
		virtual void WriteEncoded( const char* pText, size_t charCount );
		virtual void WriteEncoded( const wchar_t* pText, size_t charCount );
	private:
		io::IWriter* m_pWriter;
	};


	class CSwapBytesTextEncoder : public CStraightTextEncoder
	{	// does wchar_t byte-swapping for UTF16_be_bom
	public:
		CSwapBytesTextEncoder( io::IWriter* pWriter ) : CStraightTextEncoder( pWriter ) {}

		// base overrides
		virtual void WriteEncoded( const wchar_t* pText, size_t charCount );
	private:
		std::vector< wchar_t > m_buffer;		// excluding the EOS
	};


	class CUtf8Encoder : public CStraightTextEncoder
	{	// does wchar_t conversion to UTF8
	public:
		CUtf8Encoder( io::IWriter* pWriter ) : CStraightTextEncoder( pWriter ) {}

		// base overrides
		virtual void WriteEncoded( const wchar_t* pText, size_t charCount );
	private:
		std::string m_utf8;
	};
}


namespace io
{
	template< typename CharT >
	size_t WriteTranslatedText( io::ITextEncoder* pTextEncoder, const CharT* pText, size_t charCount ) throws_( CRuntimeException )
	{	// translate text to binary mode line by line; returns the number of translated lines
		ASSERT_PTR( pTextEncoder );

		const CharT eol[] = { '\r', '\n', 0 };		// binary mode line-end
		size_t lineCount = 0;

		for ( const CharT* pEnd = pText + charCount; pText != pEnd; ++lineCount )
		{
			const CharT* pEol = str::FindTokenEnd( pText, eol );

			ASSERT_PTR( pEol );
			pEol = std::min( pEol, pEnd );			// limit to specified length

			size_t lineLength = std::distance( pText, pEol );

			pTextEncoder->WriteEncoded( pText, lineLength );

			pText = pEol;
			if ( pText != pEnd )					// not the last line?
			{
				pTextEncoder->WriteEncoded( eol, COUNT_OF( eol ) - 1 );			// write the translated line-end (excluding the EOS)
				pText = str::SkipLineEnd( pText );
			}
		}

		return lineCount;
	}

	template< typename CharT >
	void WriteEncodedContents( io::IWriter* pWriter, fs::Encoding fileEncoding, const CharT* pText, size_t charCount = utl::npos ) throws_( CRuntimeException )
	{	// writes the entire contents to a file, including the required BOM, also performing line-end translation "\n" -> "\r\n"
		ASSERT_PTR( pWriter );

		fs::CByteOrderMark bom( fileEncoding );

		if ( !bom.IsEmpty() )
			pWriter->WriteString( &bom.Get().front(), bom.Get().size() );		// write the BOM (Byte Order Mark)

		std::auto_ptr<io::ITextEncoder> pTextEncoder( io::MakeTextEncoder( pWriter, fileEncoding ) );		// handles the line-end translation and byte swapping

		str::SettleLength( charCount, pText );
		io::WriteTranslatedText( pTextEncoder.get(), pText, charCount );
	}
}


#endif // TextEncoding_h
