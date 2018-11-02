#ifndef Timer_h
#define Timer_h
#pragma once


class CTimer
{
public:
	CTimer( void ) : m_startTime( std::clock() ) {}

	clock_t Elapsed( void ) const { return std::clock() - m_startTime; }
	double ElapsedSeconds( void ) const { return double( std::clock() - m_startTime ) / CLOCKS_PER_SEC; }

	void Restart( void ) { m_startTime = std::clock(); }

	std::tstring FormatElapsedSeconds( unsigned int precision = 3, const TCHAR fmtSeconds[] = s_fmtSeconds ) const { FormatSeconds( ElapsedSeconds(), precision, fmtSeconds ); }
	std::tstring FormatElapsedDuration( unsigned int precision = 0, const TCHAR* pFmtTimeSpan = s_fmtTimeSpan ) const { return FormatElapsedTimeSpan( ElapsedSeconds(), precision, pFmtTimeSpan ); }

	static std::tstring FormatSeconds( double elapsedSeconds, unsigned int precision, const TCHAR fmtSeconds[] = s_fmtSeconds );
	static std::tstring FormatElapsedTimeSpan( double elapsedSeconds, unsigned int precision = 0, const TCHAR* pFmtTimeSpan = s_fmtTimeSpan );		// "N seconds ([D days ][H:]M::SS)"
	static std::tstring FormatTimeSpan( time_t elapsedSeconds );																					// "[D days ][H:]M::SS"
private:
	std::clock_t m_startTime;			// miliseconds resolution (CLOCKS_PER_SEC=1000)
public:
	static const TCHAR s_fmtSeconds[];
	static const TCHAR s_fmtTimeSpan[];
};


#endif // Timer_h
