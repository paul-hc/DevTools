
#include "stdafx.h"
#include "ThrowMode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


bool CThrowMode::Good( HRESULT hResult, bool* pAllGood /*= NULL*/ ) const
{
	if ( HR_OK( hResult ) )
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
