
#include "stdafx.h"
#include "TextEncodedFileStreams.h"
#include "RuntimeException.h"
#include "EnumTags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace io
{
	void __declspec( noreturn ) ThrowOpenForReading( const fs::CPath& filePath ) throws_( CRuntimeException )
	{
		throw CRuntimeException( str::Format( _T("Cannot open text file for reading: %s"), filePath.GetPtr() ) );
	}

	void __declspec( noreturn ) ThrowOpenForWriting( const fs::CPath& filePath ) throws_( CRuntimeException )
	{
		throw CRuntimeException( str::Format( _T("Cannot open text file for writing: %s"), filePath.GetPtr() ) );
	}

	void __declspec( noreturn ) ThrowUnsupportedEncoding( fs::Encoding encoding ) throws_( CRuntimeException )
	{
		throw CRuntimeException( str::Format( _T("Support for %s text file encoding not implemented"), fs::GetTags_Encoding().FormatUi( encoding ).c_str() ) );
	}
}
