
#include "pch.h"
#include "TouchItem.h"
#include "utl/UI/DateTimeControl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTouchItem implementation

CTouchItem::CTouchItem( const fs::CFileState& srcState )
	: CFileStateItem( srcState )
	, m_destState( srcState )
{
}

CTouchItem::~CTouchItem()
{
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

	bool CDateTimeState::UpdateCtrl( CWnd* pDlg ) const
	{
		REQUIRE( m_dateTimeState != s_invalid );		// accumulated?

		CDateTimeControl* pDateTimeCtrl = checked_static_cast<CDateTimeControl*>( pDlg->GetDlgItem( m_ctrlId ) );
		ASSERT_PTR( pDateTimeCtrl );
		bool changed = m_dateTimeState != pDateTimeCtrl->GetDateTime();

		CScopedInternalChange internalChange( pDateTimeCtrl );
		VERIFY( pDateTimeCtrl->SetDateTime( m_dateTimeState ) );

		return changed;
	}

	bool CDateTimeState::InputCtrl( CWnd* pDlg )
	{
		CDateTimeControl* pDateTimeCtrl = checked_static_cast<CDateTimeControl*>( pDlg->GetDlgItem( m_ctrlId ) );
		ASSERT_PTR( pDateTimeCtrl );

		m_dateTimeState = pDateTimeCtrl->GetDateTime();
		return CanApply();
	}

	bool CDateTimeState::WouldModify( const CTouchItem* pTouchItem ) const
	{
		return
			CanApply() &&
			pTouchItem->GetDestState().GetTimeField( m_timeField ) != m_dateTimeState;
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

	bool CAttribCheckState::ApplyToAttributes( BYTE& rAttributes ) const
	{
		if ( !CanApply() )
			return false;

		SetFlag( rAttributes, m_attrFlag, BST_CHECKED == m_checkState );
		return true;
	}

	bool CAttribCheckState::UpdateCtrl( CWnd* pDlg ) const
	{
		REQUIRE( m_checkState != s_invalid );		// accumulated?
		bool changed = m_checkState != GetChecked( pDlg );

		pDlg->CheckDlgButton( m_ctrlId, m_checkState );
		return changed;
	}

	bool CAttribCheckState::InputCtrl( CWnd* pDlg )
	{
		m_checkState = GetChecked( pDlg );
		return CanApply();
	}


	// algorithms

	BYTE EvalWouldBeAttributes( const std::vector<multi::CAttribCheckState>& attribCheckStates, const CTouchItem* pTouchItem )
	{
		BYTE attributes = pTouchItem->GetDestState().m_attributes;		// start with existing value

		for ( const multi::CAttribCheckState& attribState: attribCheckStates )
			attribState.ApplyToAttributes( attributes );

		return attributes;
	}
}
