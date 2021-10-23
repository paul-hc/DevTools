
#include "stdafx.h"
#include "Crc32.h"
#include "FileSystem.h"
#include "AppTools.h"

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

	void CCrc32::AddBytes( ChecksumT& rChecksum, const void* pBuffer, size_t count ) const
	{
		ASSERT( 0 == count || pBuffer != NULL );

		for ( const BYTE* pByte = reinterpret_cast<const BYTE*>( pBuffer ); count-- != 0; ++pByte )
			AddByte( rChecksum, *pByte );
	}
}


namespace crc32
{
	// CRC generator algorithms

	UINT ComputeFileChecksum( const fs::CPath& filePath ) throws_( CFileException* )
	{
		utl::TCrc32Checksum checksum;

		CFile file( filePath.GetPtr(), CFile::modeRead | CFile::shareDenyWrite );

		std::vector< BYTE > buffer( crc32::FileBlockSize );
		BYTE* pBuffer = &buffer.front();
		UINT readCount;
		
		do
		{
			readCount = file.Read( pBuffer, crc32::FileBlockSize );
			checksum.ProcessBytes( pBuffer, readCount );
		}
		while ( readCount > 0 );

		file.Close();

		return checksum.GetResult();
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
			return crc32::ComputeFileChecksum( filePath );
		}
		catch ( CFileException* pExc )
		{
			app::TraceException( pExc );
			pExc->Delete();
			return 0;						// error
		}
	}
}
