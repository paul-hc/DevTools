#ifndef WindowTimer_h
#define WindowTimer_h
#pragma once


class CWindowTimer
{
public:
	CWindowTimer( CWnd* pWnd, UINT_PTR timerId, UINT elapseMs );
	~CWindowTimer();

	UINT_PTR GetTimerId( void ) const { return m_timerId; }
	UINT_PTR GetEventId( void ) const { ASSERT( IsStarted() ); return m_eventId; }

	UINT GetElapsed( void ) const { return m_elapseMs; }
	void SetElapsed( UINT elapseMs );

	bool IsStarted( void ) const { return m_eventId != 0 && m_pWnd->GetSafeHwnd() != nullptr; }
	bool IsHit( UINT_PTR eventId ) const { return eventId == m_eventId && IsStarted(); }

	void Start( void );
	void Start( UINT elapseMs );
	void Stop( void );

	void SetStarted( bool started ) { started ? Start() : Stop(); }
private:
	CWnd* m_pWnd;
	const UINT_PTR m_timerId;
	UINT m_elapseMs;
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
	CTimerSequenceHook( ISequenceTimerCallback* pCallback );															// restartable timer, not auto-delete constructor
	CTimerSequenceHook( HWND hWnd, ISequenceTimerCallback* pCallback, int eventId, size_t seqCount, UINT elapseMs );	// auto-delete constructor
	virtual ~CTimerSequenceHook();

	void Start( HWND hWnd, int eventId, UINT elapseMs, size_t seqCount = 1 );
	void Stop( void );
protected:
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) override;
private:
	ISequenceTimerCallback* m_pCallback;
	UINT_PTR m_eventId;
	size_t m_seqCount;
public:
	static const UINT WM_ENDTIMERSEQ;		// sent to the owner window after the sequence ends
};


#endif // WindowTimer_h
