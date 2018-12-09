
#include "stdafx.h"
#include "Crc32.h"
#include "FileSystem.h"
#include "BaseApp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace utl
{
	// CCrc32 implementation

	const UINT CCrc32::s_polynomial = 0xEDB88320;		// this is the official polynomial used by CRC32 in PKZip; often times is reversed as 0x04C11DB7.

	CCrc32::CCrc32( void )
		: m_lookupTable( 256 )
	{
		for ( int i = 0; i != 256; ++i )
		{
			UINT crcValue = i;

			for ( int j = 8; j != 0; j-- )
			{
				if ( crcValue & 1 )
					crcValue = ( crcValue >> 1 ) ^ s_polynomial;
				else
					crcValue >>= 1;
			}

			m_lookupTable[ i ] = crcValue;
		}
	}

	const CCrc32& CCrc32::Instance( void )
	{
		static CCrc32 s_table;
		return s_table;
	}

	void CCrc32::AddBytes( UINT& rCrc32, const BYTE* pBytes, size_t byteCount ) const
	{
		ASSERT( 0 == byteCount || pBytes != NULL );

		for ( size_t i = 0; i != byteCount; ++i )
			AddByte( rCrc32, pBytes[ i ] );
	}

	UINT CCrc32::ComputeCrc32( const BYTE* pBytes, size_t byteCount ) const
	{
		UINT crc32CheckSum = UINT_MAX;

		AddBytes( crc32CheckSum, pBytes, byteCount );

		crc32CheckSum = ~crc32CheckSum;
		return crc32CheckSum;
	}

	UINT CCrc32::ComputeFileCrc32( const TCHAR* pFilePath ) const throws_( CFileException* )
	{
		UINT crc32CheckSum = UINT_MAX;

		CFile file( pFilePath, CFile::modeRead | CFile::shareDenyWrite );

		enum { BlockSize = 4096 * 4 };		// read in 16384-byte (16K) data blocks at a time; originally 4096-byte (4K) data blocks

		std::vector< BYTE > buffer( BlockSize );
		BYTE* pBuffer = &buffer.front();
		UINT readCount;
		
		do
		{
			readCount = file.Read( pBuffer, BlockSize );
			AddBytes( crc32CheckSum, pBuffer, readCount );
		}
		while ( readCount > 0 );

		file.Close();

		crc32CheckSum = ~crc32CheckSum;
		return crc32CheckSum;
	}
}


namespace fs
{
	// CCrc32FileCache implementation

	CCrc32FileCache& CCrc32FileCache::Instance( void )
	{
		static CCrc32FileCache s_cache;
		return s_cache;
	}

	UINT CCrc32FileCache::AcquireCrc32( const fs::CPath& filePath )
	{
		stdext::hash_map< fs::CPath, ChecksumStampPair >::iterator itFound = m_cachedChecksums.find( filePath );
		if ( itFound != m_cachedChecksums.end() )		// found cached?
		{
			switch ( fs::CheckExpireStatus( filePath, itFound->second.second ) )
			{
				case fs::ExpiredFileModified:
					itFound->second.first = ComputeFileCrc32( filePath );
					if ( 0 == itFound->second.first )		// error
					{
						m_cachedChecksums.erase( itFound );
						break;
					}
					// fall-through
				case fs::FileNotExpired:
					return itFound->second.first;
				case fs::ExpiredFileDeleted:
					m_cachedChecksums.erase( itFound );
					break;
			}
		}
		else
		{
			if ( UINT crc32Checksum = ComputeFileCrc32( filePath ) )
			{
				m_cachedChecksums[ filePath ] = ChecksumStampPair( crc32Checksum, fs::ReadLastModifyTime( filePath ) );
				return crc32Checksum;
			}
		}

		return 0;
	}

	UINT CCrc32FileCache::ComputeFileCrc32( const fs::CPath& filePath ) const throws_()
	{
		try
		{
			return utl::CCrc32::Instance().ComputeFileCrc32( filePath.GetPtr() );
		}
		catch ( CFileException* pExc )
		{
			app::TraceException( *pExc );
			pExc->Delete();
			return 0;						// error
		}
	}
}
