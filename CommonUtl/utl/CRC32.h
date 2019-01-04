#ifndef utl_Crc32_h
#define utl_Crc32_h
#pragma once

#include <hash_map>
#include "Path.h"


namespace utl
{
	// algorithms and lookup table for computing Crc32 - Cyclic Redundancy Checksum

	class CCrc32
	{
		CCrc32( void );
	public:
		static const CCrc32& Instance( void );

		const std::vector< UINT >& GetLookupTable( void ) const { return m_lookupTable; }

		// CRC32 checksum generators
		UINT ComputeCrc32( const BYTE* pBytes, size_t byteCount ) const;
		UINT ComputeCrc32( const char* pNarrowStr ) const { return ComputeCrc32( reinterpret_cast< const BYTE* >( pNarrowStr ), str::GetLength( pNarrowStr ) ); }
		UINT ComputeCrc32( const wchar_t* pWideStr ) const { return ComputeCrc32( reinterpret_cast< const BYTE* >( pWideStr ), str::GetLength( pWideStr ) * sizeof( wchar_t ) ); }

		UINT ComputeFileCrc32( const fs::CPath& filePath ) const throws_( CFileException* );
	private:
		void AddByte( UINT& rCrc32, const BYTE byteValue ) const { rCrc32 = ( rCrc32 >> 8 ) ^ m_lookupTable[ byteValue ^ ( rCrc32 & 0x000000FF )]; }
		void AddBytes( UINT& rCrc32, const BYTE* pBytes, size_t byteCount ) const;
	private:
		std::vector< UINT > m_lookupTable;		// the lookup table with constants generated based on s_polynomial, with entry for each byte value from 0 to 255
		static const UINT s_polynomial;
	};
}


namespace fs
{
	class CCrc32FileCache
	{
		CCrc32FileCache( void ) {}
	public:
		static CCrc32FileCache& Instance( void );

		bool IsEmpty( void ) const { return m_cachedChecksums.empty(); }
		void Clear( void ) { m_cachedChecksums.clear(); }

		UINT AcquireCrc32( const fs::CPath& filePath );
	private:
		UINT ComputeFileCrc32( const fs::CPath& filePath ) const throws_();
	private:
		typedef std::pair< UINT, CTime > ChecksumStampPair;		// Crc32 checksum, lastModifyTime

		stdext::hash_map< fs::CPath, ChecksumStampPair > m_cachedChecksums;
	};
}


#endif // utl_Crc32_h
