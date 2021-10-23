#ifndef utl_Crc32_h
#define utl_Crc32_h
#pragma once

#include <limits>
#include <hash_map>
#include "Path.h"


namespace utl
{
	template< typename CrcT >
	class CChecksum
	{
		typedef typename CrcT::ChecksumT ChecksumT;
	public:
		CChecksum( void ) : m_crcTable( CrcT::Instance() ), m_crc( std::numeric_limits<ChecksumT>::max() ) {}

		void ProcessBytes( const void* pBuffer, size_t count ) { m_crcTable.AddBytes( m_crc, pBuffer, count ); }

		ChecksumT GetResult( void ) const { return ~m_crc; }
	private:
		CrcT m_crcTable;
		ChecksumT m_crc;
	};


	// Algorithms and lookup table for computing Crc32 - Cyclic Redundancy Checksum
	// Nore: in release build it's actually faster than boost::crc_32_type::process_bytes() defined in <boost/crc.hpp>
	//
	class CCrc32
	{
		CCrc32( void );
	public:
		typedef UINT ChecksumT;

		static const CCrc32& Instance( void );

		const std::vector< UINT >& GetLookupTable( void ) const { return m_lookupTable; }

		// CRC32 incremental checksum (usually starting with UINT_MAX)
		void AddBytes( ChecksumT& rChecksum, const void* pBuffer, size_t count ) const;
	private:
		void AddByte( ChecksumT& rCrc32, const BYTE byteValue ) const { rCrc32 = ( rCrc32 >> 8 ) ^ m_lookupTable[ byteValue ^ ( rCrc32 & 0x000000FF )]; }
	private:
		std::vector< UINT > m_lookupTable;		// the lookup table with constants generated based on s_polynomial, with entry for each byte value from 0 to 255
		static const UINT s_polynomial;
	};


	typedef CChecksum<CCrc32> TCrc32Checksum;
}


namespace crc32
{
	enum { FileBlockSize = 16 * KiloByte };		// read in 16384-byte (16KB) data blocks at a time; originally 4096-byte (4K) data blocks


	// CRC32 generator algorithms

	template< typename ValueT >
	UINT ComputeChecksum( const ValueT* pValues, size_t valueCount )
	{
		utl::TCrc32Checksum checksum;

		checksum.ProcessBytes( pValues, valueCount * sizeof( ValueT ) );
		return checksum.GetResult();
	}

	template< typename CharT >
	UINT ComputeStringChecksum( const CharT* pText ) { return ComputeChecksum( pText, str::GetLength( pText ) ); }

	UINT ComputeFileChecksum( const fs::CPath& filePath ) throws_( CFileException* );
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
