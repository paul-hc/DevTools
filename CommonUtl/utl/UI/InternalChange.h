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
	CScopedInternalChange( CInternalChange* pBaseCtrl ) : m_pBaseCtrl( pBaseCtrl ) { ASSERT_PTR( m_pBaseCtrl ); m_pBaseCtrl->AddInternalChange(); }
	~CScopedInternalChange() { m_pBaseCtrl->ReleaseInternalChange(); }
private:
	CInternalChange* m_pBaseCtrl;
};


#endif // InternalChange_h
