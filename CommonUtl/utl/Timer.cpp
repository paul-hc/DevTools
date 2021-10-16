
#include "stdafx.h"
#include "Timer.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const TCHAR CTimer::s_fmtSeconds[] = _T("%s seconds");
const TCHAR CTimer::s_fmtTimeSpan[] = _T(" (%s)");

std::tstring CTimer::FormatSeconds( double elapsedSeconds, unsigned int precision /*= 3*/, const TCHAR fmtSeconds[] /*= s_fmtSeconds*/ )
{
	return str::Format( fmtSeconds, num::FormatDouble( elapsedSeconds, precision, str::GetUserLocale() ).c_str() );
}

std::tstring CTimer::FormatTimeSpan( time_t seconds )
{
	enum
	{
		SecondsPerMinute = 60, MinutesPerHour = 60, HoursPerDay = 24,
		SecondsPerHour = SecondsPerMinute * 60, SecondsPerDay = SecondsPerHour * 24
	};

	CTime timespan( 2018, 11, 1,
		( seconds / SecondsPerHour ) % HoursPerDay,
		( seconds / SecondsPerMinute ) % MinutesPerHour,
		seconds % SecondsPerMinute );

	static const TCHAR s_fmtHourMinSec[] = _T("%#Hh:%Mm:%Ss");

	if ( seconds >= SecondsPerDay )
		return str::Format( _T("%d days "), seconds / SecondsPerDay ) + timespan.Format( s_fmtHourMinSec ).GetString();
	else if ( seconds >= SecondsPerHour )
		return timespan.Format( s_fmtHourMinSec ).GetString();
	else if ( seconds >= SecondsPerMinute )
		return timespan.Format( _T("%#Mm:%Ss") ).GetString();

	return timespan.Format( _T("%#Ss") ).GetString();
}

std::tstring CTimer::FormatElapsedTimeSpan( double elapsedSeconds, unsigned int precision /*= 0*/, const TCHAR* pFmtTimeSpan /*= s_fmtTimeSpan*/ )
{
	std::tstring text = FormatSeconds( elapsedSeconds, precision );

	if ( elapsedSeconds >= 60.0 && !str::IsEmpty( pFmtTimeSpan ) )
		text += str::Format( pFmtTimeSpan, FormatTimeSpan( static_cast< time_t >( elapsedSeconds ) ).c_str() );

	return text;
}
