
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

		return sign + str::Unsplit( components, _T(", ") );
	}

} //namespace time_utl
