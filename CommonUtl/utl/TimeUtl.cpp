
#include "stdafx.h"
#include "TimeUtl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace time_utl
{
	static COleDateTime GetNullOleDateTime( void )
	{
		COleDateTime nullDateTime;							// default contructor sets status to valid
		nullDateTime.SetStatus( COleDateTime::null );		// we need to make it null
		return nullDateTime;
	}


	const COleDateTime nullOleDateTime = GetNullOleDateTime();
	const Range< COleDateTime > nullOleRange( nullOleDateTime, nullOleDateTime );
}


namespace time_utl
{
	CTime FromOleTime( const COleDateTime& oleTime, CheckInvalid checkInvalid /*= NullOnInvalid*/, DaylightSavingsTimeUsage dstUsage /*= UseSystemDefault*/ ) throws_( COleException )
	{
		if ( !IsValid( oleTime ) )
			return CTime();

		SYSTEMTIME sysTime;
		if ( !oleTime.GetAsSystemTime( sysTime ) )
		{
			ASSERT( false );
			return CTime();
		}

		struct tm atm;
		atm.tm_sec = sysTime.wSecond;
		atm.tm_min = sysTime.wMinute;
		atm.tm_hour = sysTime.wHour;
		atm.tm_mday = sysTime.wDay;
		atm.tm_mon = sysTime.wMonth - 1;		// 0 based
		atm.tm_year = sysTime.wYear - 1900;		// 1900 based
		atm.tm_isdst = dstUsage;

		__time64_t time = _mktime64( &atm );
		if ( -1 == time )						// indicates an illegal input time
			if ( ThrowOnInvalid == checkInvalid )
				AtlThrow( E_INVALIDARG );
			else
				return CTime( 0 );

		return CTime( sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, dstUsage );
	}

	std::tstring GetLocaleDateTimeFormat( DateTimeFormatType formatType )
	{
		LCID lcid = ::GetUserDefaultLCID();

		TCHAR dateFormat[ 256 ];
		GetLocaleInfo( lcid, LongDate == formatType || LongDateTime == formatType ? LOCALE_SLONGDATE : LOCALE_SSHORTDATE, dateFormat, 256 );

		if ( ShortDate == formatType || LongDate == formatType )
			return dateFormat;

		TCHAR timeFormat[256];
		GetLocaleInfo( lcid, LOCALE_STIMEFORMAT, timeFormat, 256 );

		return dateFormat + std::tstring( _T(" ") ) + timeFormat;
	}

	std::tstring FormatLocaleDateTime( const COleDateTime& rDateTime )
	{
		if ( rDateTime.GetStatus() != COleDateTime::valid )
			return _T("");

		return rDateTime.Format( _T("%x %X") ).GetString();
	}

	std::tstring FormatLocaleDateTime( const CTime& rDateTime )
	{
		if ( !IsValid( rDateTime ) )
			return _T("");

		return rDateTime.Format( _T("%x %X") ).GetString();
	}

	std::tstring FormatDuration( CTimeSpan duration )
	{
		std::tstring sign;
		std::vector< std::tstring > components;

		if ( duration.GetTimeSpan() < 0 )
		{
			sign = _T("-");
			duration = -duration.GetTimeSpan();
		}

		long days = (long)duration.GetDays();
		if ( days != 0 )
			components.push_back( str::Format( _T("%d days"), days ) );

		long hours = duration.GetHours();
		if ( hours != 0 )
			components.push_back( str::Format( _T("%d hours"), hours ) );

		long minutes = duration.GetMinutes();
		if ( minutes != 0 )
			components.push_back( str::Format( _T("%d minutes"), minutes ) );

		long seconds = duration.GetSeconds();
		if ( seconds != 0 || components.empty() )
			components.push_back( str::Format( _T("%d seconds"), seconds ) );

		return sign + str::Join( components, _T(", ") );
	}
}


namespace time_utl
{
	enum CalendarMonth { January =  1, February, March, April, May, June, July, August, September, October, November, December };

	enum ConversionUnit
	{
		SecondsPerMinute = 60,
		MinutesPerHour   = 60,
		HoursPerDay      = 24,

		SecondsPerHour   = SecondsPerMinute * MinutesPerHour,
		MinutesPerDay    = MinutesPerHour * HoursPerDay,
		SecondsPerDay    = SecondsPerMinute * MinutesPerHour * HoursPerDay,

		DaysPerWeek		 =  7,
		MonthsPerYear	 = 12
	};


	bool IsLeapYear( const int year )
	{
        return ( ( year % 4 == 0 ) && ( year % 100 != 0 ) || ( year % 400 == 0 ) );
	}

	int DaysInMonth( const int year, const int month )
	{
		ASSERT( month >= January && month <= December );

		switch ( month )
		{
            case January:
            case March:
            case May:
            case July:
            case August:
            case October:
            case December:
                return 31;
            case April:
            case June:
            case September:
            case November:
                return 30;
            case February:
                return IsLeapYear( year ) ? 29 : 28;
            default:
                ASSERT( false );
                return -1;
        }
	}
}


namespace time_utl
{
	const TCHAR s_outFormat[] = _T("%d-%m-%Y %H:%M:%S");				// example: "27-12-2017 19:54:20"
	const TCHAR s_parseFormat[] = _T("%2u-%2u-%4u %2u:%2u:%2u");		// example: "27-12-2017 19:54:20"

	std::tstring FormatTimestamp( const CTime& dt, const TCHAR format[] /*= s_outFormat*/ )
	{
		std::tstring text;
		if ( dt.GetTime() != 0 )
			text = dt.Format( format ).GetString();
		return text;
	}

	CTime ParseTimestamp( const std::tstring& text, const TCHAR format[] /*= s_parseFormat*/ )
	{
		// this fails in non-US locale
		int year,
			month,		// 1-based, compatible with CalendarMonth enumeration constants
			day,		// 1-based, 1 .. 28/29/30/31
			hour,		// 0-based, 0 .. 23
			minute,		// 0-based, 0 .. 59
			second;		// 0-based, 0 .. 59

		if ( 6 == _stscanf( text.c_str(), format, &day, &month, &year, &hour, &minute, &second ) &&
			 year >= 1900 &&
			 ( month >= January && month <= December ) &&
			 ( day >= 1 && day <= DaysInMonth( year, month ) ) &&
			 ( hour >= 0 && hour < HoursPerDay ) &&
			 ( minute >= 0 && minute < MinutesPerHour ) &&
			 ( second >= 0 && second < SecondsPerMinute ) )
		{
			return CTime( year, month, day, hour, minute, second );
		}

		return CTime();
	}
}
