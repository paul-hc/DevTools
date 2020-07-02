#ifndef ErrorHandler_h
#define ErrorHandler_h
#pragma once


namespace utl
{
	enum ErrorHandling
	{
		CheckMode,
		ThrowMode,
		IgnoreMode			// used when testing for a access to a resource, in which case failure is not an error
	};
}


// configurable strategy for handling HRESULT error codes - can throw OLE exceptions in utl::ThrowMode
class CErrorHandler
{
public:
	CErrorHandler( utl::ErrorHandling handlingMode ) : m_handlingMode( handlingMode ) {}
	CErrorHandler( const CErrorHandler& src ) : m_handlingMode( src.m_handlingMode ) {}

	bool Handle( HRESULT hResult, bool* pAllGood = NULL ) const throws_( COleException* );

	utl::ErrorHandling GetHandlingMode( void ) const { return m_handlingMode; }
	void SetHandlingMode( utl::ErrorHandling handlingMode ) { m_handlingMode = handlingMode; }

	bool IsThrowMode( void ) const { return utl::ThrowMode == m_handlingMode; }
	bool IsIgnoreMode( void ) const { return utl::IgnoreMode == m_handlingMode; }

	// specialized singletons
	static const CErrorHandler* Checker( void );
	static const CErrorHandler* Thrower( void );
	static const CErrorHandler* Ignorer( void );
private:
	utl::ErrorHandling m_handlingMode;
};



class CScopedErrorHandling
{
public:
	CScopedErrorHandling( CErrorHandler* pHandler, utl::ErrorHandling handlingMode )
		: m_pHandler( pHandler )
		, m_oldHandlingMode( m_pHandler->GetHandlingMode() )
	{
		ASSERT_PTR( m_pHandler );
		m_pHandler->SetHandlingMode( handlingMode );
	}

	CScopedErrorHandling( CErrorHandler* pHandler, const CErrorHandler* pSrcHandler )
		: m_pHandler( pHandler )
		, m_oldHandlingMode( m_pHandler->GetHandlingMode() )
	{
		ASSERT_PTR( m_pHandler );
		m_pHandler->SetHandlingMode( pSrcHandler->GetHandlingMode() );
	}

	~CScopedErrorHandling()
	{
		m_pHandler->SetHandlingMode( m_oldHandlingMode );
	}
private:
	CErrorHandler* m_pHandler;
	utl::ErrorHandling m_oldHandlingMode;
};


#endif // ErrorHandler_h
