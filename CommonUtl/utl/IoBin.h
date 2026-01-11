#ifndef IoBin_h
#define IoBin_h
#pragma once

#include "Io_fwd.h"
#include "AppTools.h"
#include "RuntimeException.h"


namespace io
{
	namespace bin
	{
		// buffer I/O for binary files

		void WriteAllToFile( const fs::CPath& targetFilePath, const std::vector<char>& buffer ) throws_( CRuntimeException );
		void ReadAllFromFile( std::vector<char>& rBuffer, const fs::CPath& srcFilePath ) throws_( CRuntimeException );
	}


	namespace bin
	{
		// byte block reading algorithms not throwing exceptions

		template< typename BlockFunc >
		BlockFunc ReadCFile_NoThrow( const fs::CPath& srcFilePath, BlockFunc blockFunc ) throws_()
		{	// MFC: CFile-based binary read - about 5%-10% faster that the ifstream read version on average
			CFile file;
			CFileException exc;

			if ( file.Open( srcFilePath.GetPtr(), CFile::modeRead | CFile::typeBinary | CFile::shareDenyWrite, &exc ) )
			{
				std::vector<char> buffer( io::FileBlockSize );
				char* pBuffer = utl::Data( buffer );

				for ( UINT readCount; ( readCount = file.Read( pBuffer, io::FileBlockSize ) ) != 0; )
					blockFunc( pBuffer, readCount );

				file.Close();
			}
			else
				app::TraceException( &exc );

			return blockFunc;
		}


		template< typename BlockFunc >
		BlockFunc ReadFileStream_NoThrow( const fs::CPath& srcFilePath, BlockFunc blockFunc ) throws_()
		{	// STL: ifstream-based binary read - slightly slower than the CFile version
			std::ifstream ifs( srcFilePath.GetPtr(), std::ios_base::binary );

			if ( ifs.is_open() )
			{
				std::vector<char> buffer( io::FileBlockSize );
				char* pBuffer = utl::Data( buffer );

				while ( ifs.read( pBuffer, COUNT_OF( buffer ) ) )
					blockFunc( pBuffer, static_cast<size_t>( ifs.gcount() ) );
			}
			else
				TRACE_FL( _T(" * Cannot open file for reading: %s\n"), srcFilePath.GetPtr() );

			return blockFunc;
		}
	}


	namespace bin
	{
		// byte block reading algorithms that throw file exceptions

		template< typename BlockFunc >
		BlockFunc ReadCFile( const fs::CPath& srcFilePath, BlockFunc blockFunc ) throws_( CRuntimeException )
		{	// MFC: CFile-based binary read - about 5%-10% faster that the ifstream read version on average
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


		template< typename BlockFunc >
		BlockFunc ReadFileStream( const fs::CPath& srcFilePath, BlockFunc blockFunc ) throws_( CRuntimeException )
		{	// STL: ifstream-based binary read - slightly slower than the CFile version
			std::ifstream ifs( srcFilePath.GetPtr(), std::ios_base::binary );
			char buffer[ io::FileBlockSize ];

			if ( !ifs.is_open() )
				ThrowOpenForReading( srcFilePath );

			while ( ifs.read( buffer, COUNT_OF( buffer ) ) )
				blockFunc( buffer, ifs.gcount() );

			return blockFunc;
		}
	}


	enum FileReadMethod
	{
		ReadCFile,			// via CFile: 5%-10% faster that the ifstream read version on average
		ReadFileStream,		// via ifstream: slightly slower
	};

	namespace bin
	{
		// byte block reading algorithms not throwing exceptions

		template< typename BlockFunc >
		BlockFunc ReadFile_NoThrow( const fs::CPath& srcFilePath, BlockFunc blockFunc, io::FileReadMethod fileReadMethod ) throws_()
		{
			if ( io::ReadCFile == fileReadMethod )
				return ReadCFile_NoThrow( srcFilePath, blockFunc );
			else if ( io::ReadFileStream == fileReadMethod )
				return ReadFileStream_NoThrow( srcFilePath, blockFunc );

			ASSERT( false );		// read method?
			return blockFunc;
		}

		template< typename BlockFunc >
		BlockFunc ReadFile_Throw( const fs::CPath& srcFilePath, BlockFunc blockFunc, io::FileReadMethod fileReadMethod ) throws_( CRuntimeException )
		{
			if ( io::ReadCFile == fileReadMethod )
				return ReadCFile( srcFilePath, blockFunc );
			else if ( io::ReadFileStream == fileReadMethod )
				return ReadFileStream( srcFilePath, blockFunc );

			ASSERT( false );		// read method?
			return blockFunc;
		}
	}
}


#endif // IoBin_h
