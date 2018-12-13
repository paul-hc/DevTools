#ifndef TimeUtils_h
#define TimeUtils_h
#pragma once

#include "Range.h"
#include "atlcomtime.h"		// for COleDateTime


namespace time_utl
{
	const COleDateTime& GetNullOleDateTime( void );
	const Range< COleDateTime > GetNullOleDateTimeRange( void );


	inline bool IsValid( const CTime& dateTime ) { return dateTime.GetTime() != 0; }
	inline bool IsValidRange( const Range< CTime >& timeRange ) { return IsValid( timeRange.m_start ) && IsValid( timeRange.m_end ) && timeRange.IsNormalized(); }

	inline bool IsValid( const COleDateTime& oleDateTime ) { return COleDateTime::valid == oleDateTime.m_status && oleDateTime.m_dt != 0.0; }
	inline bool IsValidRange( const Range< COleDateTime >& timeRange ) { return IsValid( timeRange.m_start ) && IsValid( timeRange.m_end ) && timeRange.IsNormalized(); }
}


namespace time_utl
{
	enum DaylightSavingsTimeUsage { UseSystemDefault = -1, UseStandardTime, UseDst };
	enum CheckInvalid { NullOnInvalid, ThrowOnInvalid };

	CTime FromOleTime( const COleDateTime& oleTime, CheckInvalid checkInvalid = NullOnInvalid, DaylightSavingsTimeUsage dstUsage = UseSystemDefault ) throws_( COleException );
	inline COleDateTime ToOleTime( const CTime& stdTime ) { return IsValid( stdTime ) ? COleDateTime( stdTime.GetTime() ) : GetNullOleDateTime(); }

	inline Range< CTime > FromOleTimeRange( const Range< COleDateTime >& oleTimeRange ) { return Range< CTime >( FromOleTime( oleTimeRange.m_start ), FromOleTime( oleTimeRange.m_end ) ); }
	inline Range< COleDateTime > ToOleTimeRange( const Range< CTime >& stdTimeRange ) { return Range< COleDateTime >( ToOleTime( stdTimeRange.m_start ), ToOleTime( stdTimeRange.m_end ) ); }
	inline Range< COleDateTime > ToOleTimeRange( const CTime& start, const CTime& end ) { return ToOleTimeRange( Range< CTime >( start, end ) ); }

	inline CTimeSpan FromOleTimeSpan( const COleDateTimeSpan& oleDuration ) { return CTimeSpan( static_cast< time_t >( oleDuration.GetTotalSeconds() ) ); }
	inline COleDateTimeSpan ToOleTimeSpan( const CTimeSpan& duration ) { return COleDateTimeSpan( 0, 0, 0, static_cast< int >( duration.GetTimeSpan() ) ); }


	enum DateTimeFormatType { ShortDate, ShortDateTime, LongDate, LongDateTime };

    std::tstring GetLocaleDateTimeFormat( DateTimeFormatType formatType );

	std::tstring FormatLocaleDateTime( const COleDateTime& rDateTime );
	std::tstring FormatLocaleDateTime( const CTime& rDateTime );

	std::tstring FormatDuration( CTimeSpan duration );
	inline std::tstring FormatDuration( const COleDateTimeSpan& oleDuration ) { return FormatDuration( FromOleTimeSpan( oleDuration ) ); }
}


namespace time_utl
{
	extern const TCHAR s_outFormat[];			// example: "27-12-2017 19:54:20"
	extern const TCHAR s_outFormatUi[];			// example: "3-Dec-2017 17:30:00"
	extern const TCHAR s_parseFormat[];			// example: "27-12-2017 19:54:20"

	std::tstring FormatTimestamp( const CTime& dt, const TCHAR format[] = s_outFormat );
	CTime ParseTimestamp( const std::tstring& text, const TCHAR format[] = s_parseFormat );
}


inline std::ostream& operator<<( std::ostream& oss, const CTime& dt )
{
	return oss << time_utl::FormatTimestamp( dt );
}

inline std::wostream& operator<<( std::wostream& oss, const CTime& dt )
{
	return oss << time_utl::FormatTimestamp( dt );
}


#endif // TimeUtils_h
