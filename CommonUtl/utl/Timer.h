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
private:
	std::clock_t m_startTime;
};


#endif // Timer_h
