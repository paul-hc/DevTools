
#include "stdafx.h"
#include "TextFileUtils.h"
#include "Path.h"
#include "EnumTags.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "TextFileUtils.hxx"


namespace io
{
	namespace bin
	{
		// buffer I/O for binary files

		void ReadAllFromFile( std::vector< char >& rBuffer, const fs::CPath& srcFilePath ) throws_( CRuntimeException )
		{
			std::ifstream ifs( srcFilePath.GetPtr(), std::ios_base::in | std::ios_base::binary );
			io::CheckOpenForReading( ifs, srcFilePath );

			rBuffer.assign( std::istreambuf_iterator<char>( ifs ),  std::istreambuf_iterator<char>() );		// verbatim buffer of chars
		}

		void WriteAllToFile( const fs::CPath& targetFilePath, const std::vector< char >& buffer ) throws_( CRuntimeException )
		{
			std::ofstream ofs( targetFilePath.GetPtr(), std::ios_base::out | std::ios_base::binary );
			io::CheckOpenForWriting( ofs, targetFilePath );

			if ( !buffer.empty() )
				ofs.write( &buffer.front(), buffer.size() );												// verbatim buffer of chars
		}
	}
}
