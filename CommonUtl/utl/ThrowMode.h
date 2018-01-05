#ifndef ThrowMode_h
#define ThrowMode_h
#pragma once


// base for configurable classes that can throw exceptions or return error codes

class CThrowMode
{
protected:
	CThrowMode( bool throwMode ) : m_throwMode( throwMode ) {}
public:
	bool IsThrowMode( void ) const { return m_throwMode; }
	void SetThrowMode( bool throwMode = true ) { m_throwMode = throwMode; }
protected:
	bool Good( HRESULT hResult, bool* pAllGood = NULL ) const;
private:
	bool m_throwMode;
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


#endif // ThrowMode_h
