
#include "stdafx.h"
#include "BaseObjectCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	// CCodeAdapter implementation

	ui::ISubjectAdapter* CCodeAdapter::Instance( void )
	{
		static CCodeAdapter codeAdapter;
		return &codeAdapter;
	}

	std::tstring CCodeAdapter::FormatCode( const utl::ISubject* pSubject ) const
	{
		return utl::GetSafeCode( pSubject );
	}


	// CDisplayCodeAdapter implementation

	ui::ISubjectAdapter* CDisplayCodeAdapter::Instance( void )
	{
		static CDisplayCodeAdapter displayCodeAdapter;
		return &displayCodeAdapter;
	}

	std::tstring CDisplayCodeAdapter::FormatCode( const utl::ISubject* pSubject ) const
	{
		return utl::GetSafeDisplayCode( pSubject );
	}
}


CBaseObjectCtrl::CBaseObjectCtrl( UINT accelId /*= 0*/ )
	: m_pSubjectAdapter( NULL )
{
	SetSubjectAdapter( ui::CDisplayCodeAdapter::Instance() );
	if ( accelId != 0 )
		m_accel.Load( accelId );
}

void CBaseObjectCtrl::SetSubjectAdapter( ui::ISubjectAdapter* pSubjectAdapter )
{
	ASSERT_PTR( pSubjectAdapter );
	m_pSubjectAdapter = pSubjectAdapter;
}
