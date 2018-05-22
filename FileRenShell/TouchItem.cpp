
#include "stdafx.h"
#include "TouchItem.h"
#include "utl/FmtUtils.h"
#include "utl/DateTimeControl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTouchItem implementation

CTouchItem::CTouchItem( TFileStatePair* pStatePair, fmt::PathFormat pathFormat )
	: m_pStatePair( safe_ptr( pStatePair ) )
	, m_displayPath( fmt::FormatPath( pStatePair->first.m_fullPath, pathFormat ) )
{
}

CTouchItem::~CTouchItem()
{
}

std::tstring CTouchItem::GetCode( void ) const
{
	return m_pStatePair->first.m_fullPath.Get();
}

std::tstring CTouchItem::GetDisplayCode( void ) const
{
	return m_displayPath;
}

void CTouchItem::SetDestTime( app::DateTimeField field, const CTime& dateTime )
{
	app::RefTimeField( m_pStatePair->second, field ) = dateTime;
}


namespace multi
{
	// CDateTimeState implementation

	const CTime CDateTimeState::s_invalid( -1 );

	void CDateTimeState::Accumulate( const CTime& dateTime )
	{
		if ( s_invalid == m_dateTimeState.GetTime() )	// uninitialized?
			m_dateTimeState = dateTime;					// first init
		else if ( m_dateTimeState.GetTime() != 0 )		// not indeterminate?
			if ( dateTime != m_dateTimeState )			// different value
				m_dateTimeState = CTime();				// switch to indeterminate
	}

	void CDateTimeState::UpdateCtrl( CWnd* pDlg ) const
	{
		REQUIRE( m_dateTimeState != s_invalid );		// accumulated?

		CDateTimeControl* pDateTimeCtrl = checked_static_cast< CDateTimeControl* >( pDlg->GetDlgItem( m_ctrlId ) );
		ASSERT_PTR( pDateTimeCtrl );

		CScopedInternalChange internalChange( pDateTimeCtrl );
		VERIFY( pDateTimeCtrl->SetDateTime( m_dateTimeState ) );
	}

	bool CDateTimeState::InputCtrl( CWnd* pDlg )
	{
		CDateTimeControl* pDateTimeCtrl = checked_static_cast< CDateTimeControl* >( pDlg->GetDlgItem( m_ctrlId ) );
		ASSERT_PTR( pDateTimeCtrl );

		m_dateTimeState = pDateTimeCtrl->GetDateTime();
		return CanApply();
	}


	// CAttribCheckState implementation

	const UINT CAttribCheckState::s_invalid = UINT_MAX;

	void CAttribCheckState::Accumulate( BYTE attributes )
	{
		bool flag = HasFlag( attributes, m_attrFlag );

		if ( UINT_MAX == m_checkState )							// uninitialized?
			m_checkState = flag ? BST_CHECKED : BST_UNCHECKED;	// first init
		else if ( m_checkState != BST_INDETERMINATE )			// not indeterminate?
			if ( flag != ( BST_CHECKED == m_checkState ) )		// different value
				m_checkState = BST_INDETERMINATE;				// switch to indeterminate
	}

	void CAttribCheckState::UpdateCtrl( CWnd* pDlg ) const
	{
		REQUIRE( m_checkState != s_invalid );		// accumulated?

		pDlg->CheckDlgButton( m_ctrlId, m_checkState );
	}

	bool CAttribCheckState::InputCtrl( CWnd* pDlg )
	{
		m_checkState = GetChecked( pDlg );
		return CanApply();
	}
}
