#ifndef PostCall_h
#define PostCall_h
#pragma once

#include "WindowHook.h"


// abstract base for short-lived dynamic objects that execute a delayed method call

abstract class CBasePostCall : public CWindowHook
{
public:
	virtual ~CBasePostCall();
protected:
	CBasePostCall( CWnd* pWnd );		// self-deleting

	virtual void OnCall( void ) = 0;
private:
	bool PostCall( CWnd* pWnd );

	// base overrides
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );
private:
	static const UINT WM_DELAYED_CALL;
};


// void WndType::WndMethod( void );

template< typename WndType, typename WndMethod >
class CPostCall : public CBasePostCall
{
public:
	CPostCall( WndType* pWnd, WndMethod pMethod )
		: CBasePostCall( pWnd ), m_pWnd( pWnd ), m_pMethod( pMethod )
	{
	}
protected:
	virtual void OnCall( void )
	{
		(m_pWnd->*m_pMethod)();
	}
private:
	WndType* m_pWnd;
	WndMethod m_pMethod; // the member function pointer
};


// void WndType::WndMethod( Arg1 arg );

template< typename WndType, typename WndMethod, typename Arg1 >
class CPostCallArg1 : public CBasePostCall
{
public:
	CPostCallArg1( WndType* pWnd, WndMethod pMethod, Arg1 arg1 )
		: CBasePostCall( pWnd ), m_pWnd( pWnd ), m_pMethod( pMethod ), m_arg1( arg1 )
	{
	}
protected:
	virtual void OnCall( void )
	{
		(m_pWnd->*m_pMethod)( m_arg1 );
	}
private:
	WndType* m_pWnd;
	WndMethod m_pMethod; // pointer to method
	Arg1 m_arg1;
};


// void WndType::WndMethod( Arg1 arg, Arg2 arg2 );

template< typename WndType, typename WndMethod, typename Arg1, typename Arg2 >
class CPostCallArg2 : public CBasePostCall
{
public:
	CPostCallArg2( WndType* pWnd, WndMethod pMethod, Arg1 arg1, Arg2 arg2 )
		: CBasePostCall( pWnd ), m_pWnd( pWnd ), m_pMethod( pMethod ), m_arg1( arg1 ), m_arg2( arg2 )
	{
	}
protected:
	virtual void OnCall( void )
	{
		(m_pWnd->*m_pMethod)( m_arg1, m_arg2 );
	}
private:
	WndType* m_pWnd;
	WndMethod m_pMethod; // pointer to method
	Arg1 m_arg1;
	Arg2 m_arg2;
};


// void WndType::WndMethod( Arg1 arg, Arg2 arg2, Arg3 arg3 );

template< typename WndType, typename WndMethod, typename Arg1, typename Arg2, typename Arg3 >
class CPostCallArg3 : public CBasePostCall
{
public:
	CPostCallArg3( WndType* pWnd, WndMethod pMethod, Arg1 arg1, Arg2 arg2, Arg3 arg3 )
		: CBasePostCall( pWnd ), m_pWnd( pWnd ), m_pMethod( pMethod ), m_arg1( arg1 ), m_arg2( arg2 ), m_arg3( arg3 )
	{
	}
protected:
	virtual void OnCall( void )
	{
		(m_pWnd->*m_pMethod)( m_arg1, m_arg2, m_arg3 );
	}
private:
	WndType* m_pWnd;
	WndMethod m_pMethod; // pointer to method
	Arg1 m_arg1;
	Arg2 m_arg2;
	Arg3 m_arg3;
};


namespace ui
{
	// utilities for delayed method calls

	template< typename WndType, typename WndMethod >
	void PostCall( WndType* pWnd, WndMethod pMethod )
	{
		new CPostCall< WndType, WndMethod >( pWnd, pMethod );
	}

	template< typename WndType, typename WndMethod, typename Arg1 >
	void PostCall( WndType* pWnd, WndMethod pMethod, Arg1 arg1 )
	{
		new CPostCallArg1< WndType, WndMethod, Arg1 >( pWnd, pMethod, arg1 );
	}

	template< typename WndType, typename WndMethod, typename Arg1, typename Arg2 >
	void PostCall( WndType* pWnd, WndMethod pMethod, Arg1 arg1, Arg2 arg2 )
	{
		new CPostCallArg2< WndType, WndMethod, Arg1, Arg2 >( pWnd, pMethod, arg1, arg2 );
	}

	template< typename WndType, typename WndMethod, typename Arg1, typename Arg2, typename Arg3 >
	void PostCall( WndType* pWnd, WndMethod pMethod, Arg1 arg1, Arg2 arg2, Arg3 arg3 )
	{
		new CPostCallArg3< WndType, WndMethod, Arg1, Arg2, Arg3 >( pWnd, pMethod, arg1, arg2, arg3 );
	}
}


#endif // PostCall_h
