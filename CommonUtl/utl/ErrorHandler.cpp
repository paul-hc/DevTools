
#include "pch.h"
#include "ErrorHandler.h"
#include <comdef.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


bool CErrorHandler::Handle( HRESULT hResult, bool* pAllGood /*= nullptr*/ ) const throws_( COleException* )
{
	if ( IsIgnoreMode() || HR_OK( hResult ) )
		return true;				// all good
	else
	{
		if ( pAllGood != nullptr )
			*pAllGood = false;

		if ( !IsThrowMode() )
			return false;
	}

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
