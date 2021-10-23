#ifndef utl_Crc32_h
#define utl_Crc32_h
#pragma once

#include <limits>
#include <hash_map>


namespace utl
{
	template< typename CrcT >
	class CChecksum
	{
		typedef typename CrcT::UnderlyingT UnderlyingT;
	public:
		CChecksum( void ) : m_crcTable( CrcT::Instance() ), m_crc( std::numeric_limits<UnderlyingT>::max() ) {}

		void ProcessBytes( const void* pBuffer, size_t count ) { m_crcTable.AddBytes( m_crc, pBuffer, count ); }

		UnderlyingT GetResult( void ) const { return ~m_crc; }
	private:
		CrcT m_crcTable;
		UnderlyingT m_crc;
	};


	// Algorithms and lookup table for computing Crc32 - Cyclic Redundancy Checksum
	// Nore: in release build it's actually faster than boost::crc_32_type::process_bytes() defined in <boost/crc.hpp>
	//
	class CCrc32
	{
		CCrc32( void );
	public:
		typedef UINT UnderlyingT;

		static const CCrc32& Instance( void );

		const std::vector< UINT >& GetLookupTable( void ) const { return m_lookupTable; }

		// CRC32 incremental checksum (usually starting with UINT_MAX)
		void AddBytes( UnderlyingT& rChecksum, const void* pBuffer, size_t count ) const;
	private:
		void AddByte( UnderlyingT& rCrc32, const BYTE byteValue ) const { rCrc32 = ( rCrc32 >> 8 ) ^ m_lookupTable[ byteValue ^ ( rCrc32 & 0x000000FF )]; }
	private:
		std::vector< UINT > m_lookupTable;		// the lookup table with constants generated based on s_polynomial, with entry for each byte value from 0 to 255
		static const UINT s_polynomial;
	};


	typedef CChecksum<CCrc32> TCrc32Checksum;
}


namespace func
{
	template< typename ChecksumType = utl::TCrc32Checksum >
	struct ComputeChecksum
	{
		void operator()( const void* pBuffer, size_t count )
		{
			m_checksum.ProcessBytes( pBuffer, count );
		}
	public:
		ChecksumType m_checksum;
	};
}


#include "Path.h"


namespace crc32
{
	// CRC32 generator algorithms (32 bit Cyclic Redundancy Check)

	template< typename ValueT >
	UINT ComputeChecksum( const ValueT* pValues, size_t valueCount )
	{
		utl::TCrc32Checksum checksum;

		checksum.ProcessBytes( pValues, valueCount * sizeof( ValueT ) );
		return checksum.GetResult();
	}

	template< typename CharT >
	UINT ComputeStringChecksum( const CharT* pText ) { return ComputeChecksum( pText, str::GetLength( pText ) ); }

	UINT ComputeFileChecksum( const fs::CPath& filePath ) throws_( CRuntimeException );
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


#ifdef USE_BOOST_CRC				// by default not defined to prevent Boost dependencies

#pragma warning( push, 3 )			// switch to warning level 3
#pragma warning( disable: 4245 )	// identifier was truncated to 'number' characters in the debug information
#include <boost/crc.hpp>	// for boost::crc_32_type
#pragma warning( pop )				// restore to the initial warning level


namespace func
{
	template< typename ChecksumType = boost::crc_32_type >
	struct ComputeBoostChecksum
	{
		void operator()( const void* pBuffer, size_t count )
		{
			m_checksum.process_bytes( pBuffer, count );
		}
	public:
		ChecksumType m_checksum;
	};
}


#endif //USE_BOOST_CRC


#endif // utl_Crc32_h
