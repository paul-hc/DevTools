#ifndef ThrowMode_h
#define ThrowMode_h
#pragma once


// base for configurable classes that can throw exceptions or return error codes

class CThrowMode
{
protected:
	CThrowMode( bool throwMode ) : m_throwMode( throwMode ), m_ignoreMode( false ) {}
public:
	bool IsThrowMode( void ) const { return m_throwMode; }
	void SetThrowMode( bool throwMode = true ) { m_throwMode = throwMode; }

	bool IsIgnoreMode( void ) const { return m_ignoreMode; }
	void SetIgnoreMode( bool ignoreMode = true ) { m_ignoreMode = ignoreMode; }
protected:
	bool Good( HRESULT hResult, bool* pAllGood = NULL ) const;
private:
	bool m_throwMode;
	bool m_ignoreMode;			// used when testing for a access to a resource, in which case failure is not an error
};


class CNoThrow : public CThrowMode
{
public:
	CNoThrow( void ) : CThrowMode( false ) {}

	using CThrowMode::Good;
};


class CPushThrowMode
{
public:
	CPushThrowMode( CThrowMode* pThrower, bool throwMode )
		: m_pThrower( pThrower )
		, m_oldThrowMode( m_pThrower->IsThrowMode() )
	{
		ASSERT_PTR( m_pThrower );
		m_pThrower->SetThrowMode( throwMode );
	}

	CPushThrowMode( CThrowMode* pThrower, const CThrowMode* pSrcThrower )
		: m_pThrower( pThrower )
		, m_oldThrowMode( m_pThrower->IsThrowMode() )
	{
		ASSERT_PTR( m_pThrower );
		m_pThrower->SetThrowMode( pSrcThrower->IsThrowMode() );
	}

	~CPushThrowMode()
	{
		m_pThrower->SetThrowMode( m_oldThrowMode );
	}
private:
	CThrowMode* m_pThrower;
	bool m_oldThrowMode;
};


class CPushIgnoreMode
{
public:
	CPushIgnoreMode( CThrowMode* pThrower )
		: m_pThrower( pThrower )
		, m_oldIgnoreMode( m_pThrower->IsIgnoreMode() )
	{
		ASSERT_PTR( m_pThrower );
		m_pThrower->SetIgnoreMode( true );
	}

	~CPushIgnoreMode()
	{
		m_pThrower->SetIgnoreMode( m_oldIgnoreMode );
	}
private:
	CThrowMode* m_pThrower;
	bool m_oldIgnoreMode;
};


#endif // ThrowMode_h
