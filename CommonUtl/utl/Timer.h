#ifndef Timer_h
#define Timer_h
#pragma once


class CTimer
{
public:
	CTimer( void ) : m_startTime( std::clock() ) {}

	clock_t Elapsed( void ) const { return std::clock() - m_startTime; }
	double ElapsedSeconds( void ) const { return double( std::clock() - m_startTime ) / CLOCKS_PER_SEC; }

	std::tstring FormatElapsedSeconds( unsigned int precision = 3 ) const;

	void Restart( void ) { m_startTime = std::clock(); }
private:
	std::clock_t m_startTime;			// miliseconds resolution (CLOCKS_PER_SEC=1000)
};


#endif // Timer_h
