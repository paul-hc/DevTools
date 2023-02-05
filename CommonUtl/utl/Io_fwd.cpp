
#include "pch.h"
#include "Io_fwd.h"
#include "Path.h"
#include "RuntimeException.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace io
{
	void __declspec(noreturn) ThrowOpenForReading( const fs::CPath& filePath ) throws_( CRuntimeException )
	{
		throw CRuntimeException( str::Format( _T("Cannot open text file for reading: %s"), filePath.GetPtr() ) );
	}

	void __declspec(noreturn) ThrowOpenForWriting( const fs::CPath& filePath ) throws_( CRuntimeException )
	{
		throw CRuntimeException( str::Format( _T("Cannot open text file for writing: %s"), filePath.GetPtr() ) );
	}
}
