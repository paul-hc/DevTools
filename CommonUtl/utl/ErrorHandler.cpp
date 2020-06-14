
#include "stdafx.h"
#include "ErrorHandler.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


bool CErrorHandler::Handle( HRESULT hResult, bool* pAllGood /*= NULL*/ ) const throws_( COleException* )
{
	if ( IsIgnoreMode() || HR_OK( hResult ) )
		return true;				// all good
	else
	{
		if ( pAllGood != NULL )
			*pAllGood = false;

		if ( !IsThrowMode() )
			return false;
	}

	AfxThrowOleException( hResult );
}
