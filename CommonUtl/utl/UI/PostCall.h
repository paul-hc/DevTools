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


// void HostType::HostMethod( void );

template< typename HostType, typename HostMethod >
class CPostCall : public CBasePostCall
{
public:
	CPostCall( HostType* pHost, HostMethod pMethod )
		: CBasePostCall( dynamic_cast< CWnd* >( pHost ) ), m_pHost( pHost ), m_pMethod( pMethod )		// use dynamic_cast so that HostType could be a different base of the host object
	{
	}
protected:
	virtual void OnCall( void )
	{
		(m_pHost->*m_pMethod)();
	}
private:
	HostType* m_pHost;
	HostMethod m_pMethod;		// pointer to method
};


// void HostType::HostMethod( Arg1 arg );

template< typename HostType, typename HostMethod, typename Arg1 >
class CPostCallArg1 : public CBasePostCall
{
public:
	CPostCallArg1( HostType* pHost, HostMethod pMethod, Arg1 arg1 )
		: CBasePostCall( dynamic_cast< CWnd* >( pHost ) ), m_pHost( pHost ), m_pMethod( pMethod ), m_arg1( arg1 )
	{
	}
protected:
	virtual void OnCall( void )
	{
		(m_pHost->*m_pMethod)( m_arg1 );
	}
private:
	HostType* m_pHost;
	HostMethod m_pMethod;		// pointer to method
	Arg1 m_arg1;
};


// void HostType::HostMethod( Arg1 arg, Arg2 arg2 );

template< typename HostType, typename HostMethod, typename Arg1, typename Arg2 >
class CPostCallArg2 : public CBasePostCall
{
public:
	CPostCallArg2( HostType* pHost, HostMethod pMethod, Arg1 arg1, Arg2 arg2 )
		: CBasePostCall( dynamic_cast< CWnd* >( pHost ) ), m_pHost( pHost ), m_pMethod( pMethod ), m_arg1( arg1 ), m_arg2( arg2 )
	{
	}
protected:
	virtual void OnCall( void )
	{
		(m_pHost->*m_pMethod)( m_arg1, m_arg2 );
	}
private:
	HostType* m_pHost;
	HostMethod m_pMethod;		// pointer to method
	Arg1 m_arg1;
	Arg2 m_arg2;
};


// void HostType::HostMethod( Arg1 arg, Arg2 arg2, Arg3 arg3 );

template< typename HostType, typename HostMethod, typename Arg1, typename Arg2, typename Arg3 >
class CPostCallArg3 : public CBasePostCall
{
public:
	CPostCallArg3( HostType* pHost, HostMethod pMethod, Arg1 arg1, Arg2 arg2, Arg3 arg3 )
		: CBasePostCall( dynamic_cast< CWnd* >( pHost ) ), m_pHost( pHost ), m_pMethod( pMethod ), m_arg1( arg1 ), m_arg2( arg2 ), m_arg3( arg3 )
	{
	}
protected:
	virtual void OnCall( void )
	{
		(m_pHost->*m_pMethod)( m_arg1, m_arg2, m_arg3 );
	}
private:
	HostType* m_pHost;
	HostMethod m_pMethod;		// pointer to method
	Arg1 m_arg1;
	Arg2 m_arg2;
	Arg3 m_arg3;
};


namespace ui
{
	// utilities for delayed method calls

	template< typename HostType, typename HostMethod >
	void PostCall( HostType* pHost, HostMethod pMethod )
	{
		new CPostCall< HostType, HostMethod >( pHost, pMethod );
	}

	template< typename HostType, typename HostMethod, typename Arg1 >
	void PostCall( HostType* pHost, HostMethod pMethod, Arg1 arg1 )
	{
		new CPostCallArg1< HostType, HostMethod, Arg1 >( pHost, pMethod, arg1 );
	}

	template< typename HostType, typename HostMethod, typename Arg1, typename Arg2 >
	void PostCall( HostType* pHost, HostMethod pMethod, Arg1 arg1, Arg2 arg2 )
	{
		new CPostCallArg2< HostType, HostMethod, Arg1, Arg2 >( pHost, pMethod, arg1, arg2 );
	}

	template< typename HostType, typename HostMethod, typename Arg1, typename Arg2, typename Arg3 >
	void PostCall( HostType* pHost, HostMethod pMethod, Arg1 arg1, Arg2 arg2, Arg3 arg3 )
	{
		new CPostCallArg3< HostType, HostMethod, Arg1, Arg2, Arg3 >( pHost, pMethod, arg1, arg2, arg3 );
	}
}


#endif // PostCall_h
