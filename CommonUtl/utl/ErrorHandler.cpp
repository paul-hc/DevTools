
#include "pch.h"
#include "ErrorHandler.h"
#include <comdef.h>			// _com_error

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CErrorHandler implementation

bool CErrorHandler::Handle( HRESULT hResult, bool* pAllGood /*= nullptr*/ ) const throws_( COleException* )
{
	if ( IsIgnoreMode() )
		return true;				// discard the error
	else if ( this != CScopedErrorHandler::GlobalHandler() )
	{	// safe to call utl::Audit() via HR_OK - no infinite recursion:
		if ( HR_OK( hResult ) )
			return true;			// all good
	}
	else if ( SUCCEEDED( hResult ) )	// processing in utl::Audit() via HR_OK => don't use HR_OK to avoid infinite recursion
		return true;				// all good

	if ( pAllGood != nullptr )
		*pAllGood = false;

	if ( !IsThrowMode() )
		return false;				// error handled, no exception to throw

	AfxThrowOleException( hResult );
}

const CErrorHandler* CErrorHandler::Checker( void )
{
	static const CErrorHandler s_checkHandler( utl::CheckMode );
	return &s_checkHandler;
}

const CErrorHandler* CErrorHandler::Thrower( void )
{
	static const CErrorHandler s_throwHandler( utl::ThrowMode );
	return &s_throwHandler;
}

const CErrorHandler* CErrorHandler::Ignorer( void )
{
	static const CErrorHandler s_ignoreHandler( utl::IgnoreMode );
	return &s_ignoreHandler;
}


// CScopedErrorHandler implementation

const CErrorHandler* CScopedErrorHandler::s_pCurrHandler = nullptr;
std::tstring CScopedErrorHandler::s_titleContext;

const CErrorHandler* CScopedErrorHandler::HandlerInstance( utl::ErrorHandling handlingMode )
{
	switch ( handlingMode )
	{
		default: ASSERT( false );
		case utl::CheckMode:	return CErrorHandler::Checker();
		case utl::ThrowMode:	return CErrorHandler::Thrower();
		case utl::IgnoreMode:	return CErrorHandler::Ignorer();
	}
}

std::tstring CScopedErrorHandler::DecorateErrorMessage( const std::tstring& errorMsg )
{
	if ( s_titleContext.empty() )
		return errorMsg;

	return s_titleContext + _T("\r\n\r\n") + errorMsg;
}


namespace utl
{
	// CErrorCode implementation

	const std::tstring& CErrorCode::FormatError( void ) const
	{
		static std::tstring s_errorMessage;		// not a thread-safe function

		if ( IsError() )
		{
			_com_error error( m_errorCode );
			s_errorMessage = error.ErrorMessage();
		}
		else
			s_errorMessage.clear();

		return s_errorMessage;
	}
}
