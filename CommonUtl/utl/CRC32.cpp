
#include "stdafx.h"
#include "Crc32.h"
#include "FileSystem.h"
#include "IoBin.h"
#include "EnumTags.h"
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

			for ( int j = 8; j-- != 0; )
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

	void CCrc32::AddBytes( TUnderlying& rChecksum, const void* pBuffer, size_t count ) const
	{
		ASSERT( 0 == count || pBuffer != NULL );

		for ( const BYTE* pByte = reinterpret_cast<const BYTE*>( pBuffer ); count-- != 0; ++pByte )
			AddByte( rChecksum, *pByte );
	}
}


namespace crc32
{
	// CRC generator algorithms

	UINT ComputeFileChecksum( const fs::CPath& filePath )
	{
		UINT checksum = io::bin::ReadCFile_NoThrow( filePath, func::ComputeChecksum<utl::TCrc32Checksum>() ).m_checksum.GetResult();

		//TRACE( _T("crc32::ComputeFileChecksum(%s)=%08X\n"), filePath.GetPtr(), checksum );
		return checksum;
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
		stdext::hash_map< fs::CPath, CStamp >::iterator itFound = m_cachedChecksums.find( filePath );
		if ( itFound != m_cachedChecksums.end() )		// found cached?
		{
			fs::FileExpireStatus status = CheckExpireStatus( filePath, itFound->second.m_fileSize, itFound->second.m_modifyTime );

			//TRACE( _T("CCrc32FileCache::AcquireCrc32( %s ) - '%s'\n"), filePath.GetPtr(), fs::GetTags_FileExpireStatus().FormatKey( status ).c_str() );
			switch ( status )
			{
				case fs::FileNotExpired:
					return itFound->second.m_crc32Checksum;		// cache hit
				case fs::ExpiredFileModified:
					m_cachedChecksums.erase( itFound );			// file modified in the meantime: refresh CRC
					break;
				case fs::ExpiredFileDeleted:
					m_cachedChecksums.erase( itFound );			// file has been deleted
					return 0;
			}
		}

		if ( UINT crc32Checksum = crc32::ComputeFileChecksum( filePath ) )
		{
			m_cachedChecksums[ filePath ] = CStamp( crc32Checksum, fs::GetFileSize( filePath.GetPtr() ), fs::ReadLastModifyTime( filePath ) );
			return crc32Checksum;
		}

		return 0;
	}

	fs::FileExpireStatus CCrc32FileCache::CheckExpireStatus( const fs::CPath& filePath, UINT64 fileSize, const CTime& modifyTime )
	{
		fs::FileExpireStatus status = fs::CheckExpireStatus( filePath, modifyTime );

		if ( fs::FileNotExpired == status )
		{	// fix issue in unit tests execution in quick sequence, when the modifyTime doesn't seem modified due to time resolution of 1s: also factor-in file size
			if ( fileSize != fs::GetFileSize( filePath.GetPtr() ) )		// file content was modified?
				return fs::ExpiredFileModified;							// we have an expired file (content changed)
		}

		return status;
	}
}
