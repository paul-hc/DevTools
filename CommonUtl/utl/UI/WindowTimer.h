#ifndef WindowTimer_h
#define WindowTimer_h
#pragma once


class CWindowTimer
{
public:
	CWindowTimer( CWnd* pWnd, UINT_PTR timerId, UINT elapsedMs );
	~CWindowTimer();

	UINT_PTR GetTimerId( void ) const { return m_timerId; }
	UINT_PTR GetEventId( void ) const { ASSERT( IsStarted() ); return m_eventId; }

	UINT GetElapsed( void ) const { return m_elapsedMs; }
	void SetElapsed( UINT elapsedMs );

	bool IsStarted( void ) const { return m_eventId != 0 && m_pWnd->GetSafeHwnd() != NULL; }
	bool IsHit( UINT_PTR eventId ) const { return eventId == m_eventId && IsStarted(); }

	void Start( void );
	void Start( UINT elapsedMs );
	void Stop( void );

	void SetStarted( bool started ) { started ? Start() : Stop(); }
private:
	CWnd* m_pWnd;
	const UINT_PTR m_timerId;
	UINT m_elapsedMs;
	UINT_PTR m_eventId;			// it may be different than m_timerId
};


#include "WindowHook.h"


interface ISequenceTimerCallback
{
	virtual void OnSequenceStep( void ) = 0;
};


class CTimerSequenceHook : public CWindowHook
{
public:
	CTimerSequenceHook( HWND hWnd, ISequenceTimerCallback* pCallback, int eventId, unsigned int seqCount, int elapse );
	virtual ~CTimerSequenceHook();

	void Stop( void );
protected:
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );
private:
	ISequenceTimerCallback* m_pCallback;
	UINT_PTR m_eventId;
	unsigned int m_seqCount;
public:
	static const UINT WM_ENDTIMERSEQ;		// sent to the owner window after the sequence ends
};


#endif // WindowTimer_h
