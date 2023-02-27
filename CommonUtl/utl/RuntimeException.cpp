
#include "pch.h"
#include "RuntimeException.h"
#include "AppTools.h"
#include <iostream>


CRuntimeException::CRuntimeException( void )
{
}

CRuntimeException::CRuntimeException( const std::string& message )
	: m_message( message )
{
}

CRuntimeException::CRuntimeException( const std::wstring& message )
	: m_message( str::ToUtf8( message.c_str() ) )
{
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
