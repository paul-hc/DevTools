#ifndef BaseFlagsCtrl_h
#define BaseFlagsCtrl_h
#pragma once

#include "utl/AccelTable.h"
#include "wnd/FlagStore.h"


#define ON_FN_FLAGSCHANGED( id, memberFxn ) ON_CONTROL( CBaseFlagsCtrl::FN_FLAGSCHANGED, id, memberFxn )


abstract class CBaseFlagsCtrl
{
protected:
	CBaseFlagsCtrl( CWnd* pCtrl );

	virtual void InitControl( void ) = 0;
	virtual void OutputFlags( void ) = 0;
public:
	enum NotifyCode { FN_FLAGSCHANGED = 0x0B00 };

	bool HasValidMask( void ) const { return !m_flagStores.empty() || m_flagsMask != 0; }

	const CFlagStore* GetFlagStore( void ) const { return 1 == m_flagStores.size() ? m_flagStores.front() : NULL; }
	void SetFlagStore( const CFlagStore* pFlagsStore );

	// total edit: multiple stores combined
	const std::vector< const CFlagStore* >& GetMultiFlagStores( void ) const { return m_flagStores; }
	void SetMultiFlagStores( const std::vector< const CFlagStore* >& flagStores, DWORD flagsMask = UINT_MAX );

	DWORD GetFlagsMask( void ) const { return m_flagsMask; }
	void SetFlagsMask( DWORD flagsMask );

	DWORD GetFlags( void ) const { return m_flags; }
	void SetFlags( DWORD flags );
	bool UserSetFlags( DWORD flags );

	void StripUsedFlags( DWORD* pUnknownFlags ) const { ASSERT_PTR( pUnknownFlags ); *pUnknownFlags &= ~m_flagsMask; }

	std::tstring Format( void ) const;
	std::tstring FormatTooltip( void ) const;
private:
	CWnd* m_pCtrl;
	std::vector< const CFlagStore* > m_flagStores;
	DWORD m_flagsMask;
	DWORD m_flags;
protected:
	CAccelTable m_accel;
};


#endif // BaseFlagsCtrl_h
