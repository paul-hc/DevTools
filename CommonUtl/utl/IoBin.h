#ifndef IoBin_h
#define IoBin_h
#pragma once

#include "Io_fwd.h"
#include "RuntimeException.h"


namespace io
{
	namespace bin
	{
		// buffer I/O for binary files

		void WriteAllToFile( const fs::CPath& targetFilePath, const std::vector< char >& buffer ) throws_( CRuntimeException );
		void ReadAllFromFile( std::vector< char >& rBuffer, const fs::CPath& srcFilePath ) throws_( CRuntimeException );
	}


	namespace bin
	{
		// MFC: CFile-based binary read - about 10% faster that the ifstream read version on average
		//
		template< typename BlockFunc >
		BlockFunc ReadCFile( const fs::CPath& srcFilePath, BlockFunc blockFunc ) throws_( CRuntimeException )
		{
			try
			{
				CFile file( srcFilePath.GetPtr(), CFile::modeRead | CFile::typeBinary | CFile::shareDenyWrite );
				char buffer[ io::FileBlockSize ];
				
				for ( UINT readCount; ( readCount = file.Read( buffer, io::FileBlockSize ) ) != 0; )
					blockFunc( buffer, readCount );

				file.Close();
				return blockFunc;
			}
			catch ( CFileException* pExc )
			{
				CRuntimeException::ThrowFromMfc( pExc );		// throw a CRuntimeException instead
			}
		}


		// STL: ifstream-based binary read - slightly slower than the CFile version
		//
		template< typename BlockFunc >
		BlockFunc ReadFileStream( const fs::CPath& srcFilePath, BlockFunc blockFunc ) throws_( CRuntimeException )
		{
			std::ifstream ifs( srcFilePath.GetPtr(), std::ios_base::binary );
			if ( !ifs.is_open() )
				ThrowOpenForReading( srcFilePath );

			char buffer[ io::FileBlockSize ];

			while ( ifs )
			{
				ifs.read( buffer, COUNT_OF( buffer ) );
				blockFunc( buffer, ifs.gcount() );
			}
			return blockFunc;
		}
	}
}


#endif // IoBin_h
