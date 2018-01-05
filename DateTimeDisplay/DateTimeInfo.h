#ifndef DateTimeInfo_h
#define DateTimeInfo_h
#pragma once

#include "utl/EnumTags.h"


struct CDateTimeInfo
{
	CDateTimeInfo( const std::tstring& text );

	std::tstring Format( void ) const;

	bool IsValidField( void ) const { return m_type != Null; }
	bool IsTimeField( void ) const { return Time == m_type || OleTime == m_type; }
	bool IsDurationField( void ) const { return Duration == m_type || OleDuration == m_type; }

	enum Type { Null, Time, OleTime, Duration, OleDuration };
	static const CEnumTags& GetTags_Type( void );
private:
	void ParseText( void );

	static bool IsValidNumber( const TCHAR* pText, bool& rIsDouble );
public:
	std::tstring m_text;
	Type m_type;
	CTime m_time;
	CTimeSpan m_duration;

	static const TCHAR m_format[];
};


struct CDurationInfo
{
	CDurationInfo( const CDateTimeInfo& start, const CDateTimeInfo& end )
		: m_isValid( start.IsTimeField() && end.IsTimeField() )
	{
		if ( m_isValid )
			m_duration = end.m_time - start.m_time;
	}

	std::tstring Format( void ) const;
public:
	bool m_isValid;
	CTimeSpan m_duration;
};


#endif // DateTimeInfo_h
