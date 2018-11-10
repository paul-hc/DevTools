
#include "stdafx.h"
#include "ObjectCtrlBase.h"

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


CObjectCtrlBase::CObjectCtrlBase( UINT ctrlAccelId /*= 0*/ )
	: m_pSubjectAdapter( NULL )
{
	SetSubjectAdapter( ui::CDisplayCodeAdapter::Instance() );

	if ( ctrlAccelId != 0 )
		m_ctrlAccel.Load( ctrlAccelId );
}

void CObjectCtrlBase::SetSubjectAdapter( ui::ISubjectAdapter* pSubjectAdapter )
{
	ASSERT_PTR( pSubjectAdapter );
	m_pSubjectAdapter = pSubjectAdapter;
}

bool CObjectCtrlBase::IsInternalCmdId( int cmdId ) const
{
	return m_internalCmdIds.ContainsId( cmdId );
}
