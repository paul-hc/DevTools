
#include "pch.h"
#include "RuntimeException.h"
#include "AppTools.h"
#include <iostream>


namespace hlp
{
	std::string MakeSrcFileMessage( const std::string& message, const char* pSrcFilePath, int lineNumber )
	{
		ASSERT_PTR( pSrcFilePath );
		fs::CPath filePath( str::FromAnsi( pSrcFilePath ) );

		// convert "C:\dev\DevTools\CommonUtl\utl\Command.cpp" to "utl/Command.cpp"
		filePath = path::StripDirPrefix( filePath, filePath.GetParentPath().GetParentPath() );
		str::Replace( filePath.Ref(), _T("\\"), _T("/") );

		std::ostringstream os;
		os << pSrcFilePath << '(' << lineNumber << ") : " << message;
		return os.str();
	}
}


CRuntimeException::CRuntimeException( void )
{
}

CRuntimeException::CRuntimeException( const std::string& message, const char* pSrcFilePath /*= nullptr*/, int lineNumber /*= 0*/ )
	: m_message( message )
{
	if ( pSrcFilePath != nullptr )
		m_message = hlp::MakeSrcFileMessage( m_message, pSrcFilePath, lineNumber );
}

CRuntimeException::CRuntimeException( const std::wstring& message, const char* pSrcFilePath /*= nullptr*/, int lineNumber /*= 0*/ )
	: m_message( str::ToUtf8( message.c_str() ) )
{
	if ( pSrcFilePath != nullptr )
		m_message = hlp::MakeSrcFileMessage( m_message, pSrcFilePath, lineNumber );
}

CRuntimeException::~CRuntimeException() throw()
{
}

std::tstring CRuntimeException::GetMessage( void ) const throw()
{
	#ifdef _UNICODE
		return str::FromUtf8( m_message.c_str() );
	#else
		return m_message;
	#endif
}

void CRuntimeException::ReportError( void ) const throw()
{
	if ( HasMessage() )
		app::ReportException( *this );
}

std::tstring CRuntimeException::MessageOf( const std::exception& exc ) throw()
{
	return str::FromUtf8( exc.what() );
}

void __declspec(noreturn) CRuntimeException::ThrowFromMfc( CException* pExc ) throws_( CRuntimeException )
{
	ASSERT_PTR( pExc );

	std::tstring message = mfc::CRuntimeException::MessageOf( *pExc );
	pExc->Delete();

	throw CRuntimeException( message );
}


namespace mfc
{
	// CRuntimeException implementation

	CRuntimeException::CRuntimeException( const std::tstring& message )
		: CException()
		, m_message( message )
	{
	}

	CRuntimeException::~CRuntimeException()
	{
	}

	BOOL CRuntimeException::GetErrorMessage( TCHAR* pMessage, UINT count, UINT* pHelpContext /*= nullptr*/ ) const
	{
		ASSERT_PTR( pMessage );

		if ( pHelpContext != nullptr )
			*pHelpContext = 0;

		_tcsncpy( pMessage, m_message.c_str(), count - 1 );
		pMessage[ count - 1 ] = _T('\0');
		return TRUE;
	}

	std::tstring CRuntimeException::MessageOf( const CException& exc ) throw()
	{
		TCHAR message[ 1024 ];
		exc.GetErrorMessage( message, COUNT_OF( message ) );
		return message;
	}


	// CUserAbortedException implementation

	int CUserAbortedException::ReportError( UINT mbType /*= MB_OK*/, UINT messageId /*= 0*/ )
	{
		mbType, messageId;
		return 1;
	}
}
