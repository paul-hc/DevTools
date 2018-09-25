
#include "stdafx.h"
#include "CRC32.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace crc32
{
	// CCRC32Table implementation

	const UINT CCRC32Table::s_polynomial = 0xEDB88320;		// this is the official polynomial used by CRC32 in PKZip; often times is reversed as 0x04C11DB7.

	CCRC32Table::CCRC32Table( void )
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

	const CCRC32Table& CCRC32Table::Instance( void )
	{
		static CCRC32Table s_table;
		return s_table;
	}

	void CCRC32Table::AddBytes( UINT& rCrc32, const BYTE* pBytes, size_t byteCount ) const
	{
		ASSERT( 0 == byteCount || pBytes != NULL );

		for ( size_t i = 0; i != byteCount; ++i )
			AddByte( rCrc32, pBytes[ i ] );
	}

	UINT CCRC32Table::ComputeCrc32( const BYTE* pBytes, size_t byteCount ) const
	{
		UINT crc32CheckSum = UINT_MAX;

		AddBytes( crc32CheckSum, pBytes, byteCount );

		crc32CheckSum = ~crc32CheckSum;
		return crc32CheckSum;
	}

	UINT CCRC32Table::ComputeFileCrc32( const TCHAR* pFilePath ) const throws_( CFileException* )
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
