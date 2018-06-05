
#include "stdafx.h"
#include "DateTimeInfo.h"
#include <sstream>
#include "utl/StringUtilities.h"
#include "utl/TimeUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const TCHAR CDateTimeInfo::m_format[] = _T("%b %d (%a), %Y - %X");		// \x25cf=Black Circle


CDateTimeInfo::CDateTimeInfo( const std::tstring& text )
	: m_text( text )
	, m_type( Null )
{
	ParseText();
}

const CEnumTags& CDateTimeInfo::GetTags_Type( void )
{
	static const CEnumTags tags( _T("Null|Time (time_t)|OleTime (double)|Duration (time_t)|OLE Duration (double)") );
	return tags;
}

void CDateTimeInfo::ParseText( void )
{
	m_type = Null;
	str::Trim( m_text, _T(" \t\r\n") );

	const TCHAR* pText = m_text.c_str();

	enum Field { TimeField, DurationField };
	Field field = _T('#') == *pText ? DurationField : TimeField;

	if ( DurationField == field )
		while ( _istspace( *++pText ) )
		{
		}

	if ( _T('\0') == *pText )
		return;

	bool isDouble;
	if ( !IsValidNumber( pText, isDouble ) )
		return;

	if ( isDouble )
	{
		double d = 0.0;

		std::tstring text = pText;
		std::tistringstream stream( text );

		stream >> d;

		if ( d != 0.0 )
			try
			{
				switch ( field )
				{
					case TimeField:
					{
						COleDateTime oleDateTime( d );

						if ( COleDateTime::valid == oleDateTime.m_status )
						{
							m_type = OleTime;
							m_time = time_utl::FromOleTime( oleDateTime, time_utl::ThrowOnInvalid );
						}
						break;
					}
					case DurationField:
					{
						COleDateTimeSpan oleDuration( d );

						if ( COleDateTime::valid == oleDuration.m_status )
						{
							m_type = OleDuration;
							m_duration = CTimeSpan( 0, 0, 0, (int)oleDuration.GetTotalSeconds() );
						}
						break;
					}
				}
			}
			catch ( CException* pExc )
			{
				pExc->Delete();
			}
	}
	else
	{
		time_t t = _ttol( pText );

		switch ( field )
		{
			case TimeField:
				m_time = CTime( t );
				m_type = Time;
				break;
			case DurationField:
				m_duration = CTimeSpan( t );
				m_type = Duration;
				break;
		}
	}
}

bool CDateTimeInfo::IsValidNumber( const TCHAR* pText, bool& rIsDouble )
{
	rIsDouble = false;

	size_t digitCount = 0;
	size_t decimalPointCount = 0;
	size_t signCount = 0;
	size_t nonNumericCount = 0;

	for ( const TCHAR* pCursor = pText; *pCursor != _T('\0'); ++pCursor )
		if ( _istdigit( *pCursor ) )
			++digitCount;
		else if ( _T('.') == *pCursor )
			++decimalPointCount;
		else if ( _T('+') == *pCursor || _T('-') == *pCursor )
			++signCount;
		else if ( _T(' ') == *pCursor || _T('\t') == *pCursor )
			break;					// ignore any text comments after whitespace
		else
			++nonNumericCount;

	if ( digitCount != 0 && 0 == nonNumericCount )
		if ( signCount <= 1 && decimalPointCount <= 1 )
		{
			rIsDouble = 1 == decimalPointCount;
			return true;
		}

	return false;
}

std::tstring CDateTimeInfo::Format( void ) const
{
	std::tstring fieldText;
	if ( !m_text.empty() )
		switch ( m_type )
		{
			default:
				ASSERT( false );
			case Null:
				fieldText = _T("null");
				break;
			case Time:
			case OleTime:
				fieldText = m_time.Format( m_format );
				if ( OleTime == m_type )
					fieldText += str::Format( _T(" (%d)"), m_time.GetTime() );
				break;
			case Duration:
			case OleDuration:
				fieldText = time_utl::FormatDuration( m_duration );
				if ( OleDuration == m_type )
					fieldText += str::Format( _T(" (%d)"), m_duration.GetTimeSpan() );
				break;
		}

	return fieldText;
}


// CDurationInfo implementation

std::tstring CDurationInfo::Format( void ) const
{
	std::tstring durationText;
	if ( m_isValid )
		durationText = time_utl::FormatDuration( m_duration );
	return durationText;
}
