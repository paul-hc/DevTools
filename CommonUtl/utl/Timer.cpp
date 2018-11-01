
#include "stdafx.h"
#include "Timer.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


std::tstring CTimer::FormatElapsedSeconds( unsigned int precision /*= 3*/ ) const
{
	return num::FormatDouble( ElapsedSeconds(), precision, str::GetUserLocale() );
}
