#ifndef ClockStatic_h
#define ClockStatic_h
#pragma once

#include "ThemeStatic.h"
#include "Timer.h"
#include "WindowTimer.h"


class CClockStatic : public CRegularStatic
{
public:
	enum TimerId { ClockTimerId = 120 };
	enum Notification { ClockTicked = STN_DISABLE + 20 };		// via WM_COMMAND

	CClockStatic( unsigned int precision = 0, Style style = Static );
	virtual ~CClockStatic();

	// fractional digits
	unsigned int GetPrecision( void ) const { return m_precision; }
	void SetPrecision( unsigned int precision );
private:
	unsigned int m_precision;			// fractional digits (0 by default)

	CTimer m_timer;						// for elapsed time clock (in progress reporting)
	CWindowTimer m_clockTimer;

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
protected:
	afx_msg void OnTimer( UINT_PTR eventId );

	DECLARE_MESSAGE_MAP()
};


#endif // ClockStatic_h
