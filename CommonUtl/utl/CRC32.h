#ifndef CRC32_h
#define CRC32_h
#pragma once


namespace crc32
{
	// lookup table for computing CRC32 - Cyclic Redundancy Checksum

	class CCRC32Table
	{
		CCRC32Table( void );
	public:
		static const CCRC32Table& Instance( void );

		const std::vector< UINT >& GetLookupTable( void ) const { return m_lookupTable; }

		// CRC32 checksum generators
		UINT ComputeCrc32( const BYTE* pBytes, size_t byteCount ) const;
		UINT ComputeCrc32( const char* pNarrowStr ) const { return ComputeCrc32( reinterpret_cast< const BYTE* >( pNarrowStr ), str::GetLength( pNarrowStr ) ); }
		UINT ComputeCrc32( const wchar_t* pWideStr ) const { return ComputeCrc32( reinterpret_cast< const BYTE* >( pWideStr ), str::GetLength( pWideStr ) * sizeof( wchar_t ) ); }
		UINT ComputeFileCrc32( const TCHAR* pFilePath ) const throws_( CFileException* );
	private:
		void AddByte( UINT& rCrc32, const BYTE byteValue ) const { rCrc32 = ( rCrc32 >> 8 ) ^ m_lookupTable[ byteValue ^ ( rCrc32 & 0x000000FF )]; }
		void AddBytes( UINT& rCrc32, const BYTE* pBytes, size_t byteCount ) const;
	private:
		std::vector< UINT > m_lookupTable;		// the lookup table with constants generated based on s_polynomial, with entry for each byte value from 0 to 255
		static const UINT s_polynomial;
	};
}


#endif // CRC32_h
