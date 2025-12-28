#ifndef InternalChange_h
#define InternalChange_h
#pragma once


// base for controls that inhibit notifications on internal changes

class CInternalChange
{
public:
	CInternalChange( void ) : m_internalChange( 0 ) {}
	~CInternalChange() { ASSERT( 0 == m_internalChange ); }

	bool IsInternalChange( void ) const { return m_internalChange != 0; }

	void AddInternalChange( void )
	{
		if ( 1 == ::InterlockedIncrement( &m_internalChange ) )
			OnFirstAddInternalChange();
	}

	void ReleaseInternalChange( void )
	{
		ASSERT( m_internalChange != 0 );
		if ( 0 == ::InterlockedDecrement( &m_internalChange ) )
			OnFinalReleaseInternalChange();
	}
protected:
	// overridables
	virtual void OnFirstAddInternalChange( void ) {}
	virtual void OnFinalReleaseInternalChange( void ) {}
private:
	ULONG m_internalChange;
};


struct CScopedInternalChange
{
	CScopedInternalChange( CInternalChange* pBaseCtrl, bool condition = true )
		: m_pBaseCtrl( condition ? pBaseCtrl : nullptr )
	{
		if ( m_pBaseCtrl != nullptr )		// allow for external conditional logic (possibly passing NULL)
			m_pBaseCtrl->AddInternalChange();
	}

	~CScopedInternalChange()
	{
		if ( m_pBaseCtrl != nullptr )
			m_pBaseCtrl->ReleaseInternalChange();
	}
private:
	CInternalChange* m_pBaseCtrl;
};


#endif // InternalChange_h
