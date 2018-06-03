#ifndef TouchItem_h
#define TouchItem_h
#pragma once

#include "utl/FileState.h"
#include "BasePathItem.h"


namespace app
{
	enum DateTimeField { ModifiedDate, CreatedDate, AccessedDate, _DateTimeFieldCount };

	const CTime& GetTimeField( const fs::CFileState& fileState, DateTimeField field );
	inline CTime& RefTimeField( fs::CFileState& rFileState, DateTimeField field ) { return const_cast< CTime& >( GetTimeField( rFileState, field ) ); }
}


class CTouchItem : public CBasePathItem
{
public:
	CTouchItem( const fs::CFileState& srcState );
	virtual ~CTouchItem();

	const fs::CFileState& GetSrcState( void ) const { return m_srcState; }
	const fs::CFileState& GetDestState( void ) const { return m_destState; }

	bool IsModified( void ) const { return m_destState.IsValid() && m_destState != m_srcState; }

	fs::CFileState& RefDestState( void ) { return m_destState; }
	void Reset( void ) { m_destState = m_srcState; }

	void SetDestTime( app::DateTimeField field, const CTime& dateTime );
	void SetDestAttributeFlag( CFile::Attribute attrFlag, bool on ) { SetFlag( m_destState.m_attributes, attrFlag, on ); }
private:
	const fs::CFileState m_srcState;
	fs::CFileState m_destState;
};


namespace multi
{
	// accumulates common values among multiple items, starting with an invalid value (uninitialized)

	class CDateTimeState
	{
	public:
		CDateTimeState( UINT ctrlId = 0, app::DateTimeField field = app::ModifiedDate, const CTime& dateTimeState = CTime() ) : m_ctrlId( ctrlId ), m_field( field ), m_dateTimeState( dateTimeState ) {}

		void Clear( void ) { m_dateTimeState = CTime(); }
		void SetInvalid( void ) { m_dateTimeState = s_invalid; }

		void Accumulate( const CTime& dateTime );

		void UpdateCtrl( CWnd* pDlg ) const;
		bool InputCtrl( CWnd* pDlg );

		bool CanApply( void ) const { REQUIRE( m_dateTimeState != s_invalid ); return m_dateTimeState.GetTime() != 0; }
		void Apply( CTouchItem* pTouchItem ) const { REQUIRE( CanApply() ); pTouchItem->SetDestTime( m_field, m_dateTimeState ); }
		bool WouldModify( const CTouchItem* pTouchItem ) const;
	public:
		UINT m_ctrlId;
		app::DateTimeField m_field;
	private:
		CTime m_dateTimeState;
		static const CTime s_invalid;
	};


	class CAttribCheckState
	{
	public:
		CAttribCheckState( UINT ctrlId = 0, CFile::Attribute attrFlag = CFile::normal, UINT checkState = BST_UNCHECKED ) : m_ctrlId( ctrlId ), m_attrFlag( attrFlag ), m_checkState( checkState ) {}

		void Clear( void ) { m_checkState = BST_UNCHECKED; }
		void SetInvalid( void ) { m_checkState = s_invalid; }

		void Accumulate( BYTE attributes );
		bool ApplyToAttributes( BYTE& rAttributes ) const;

		void UpdateCtrl( CWnd* pDlg ) const;
		bool InputCtrl( CWnd* pDlg );
		UINT GetChecked( CWnd* pDlg ) const { return pDlg->IsDlgButtonChecked( m_ctrlId ); }

		bool CanApply( void ) const { REQUIRE( m_checkState != s_invalid ); return m_checkState != BST_INDETERMINATE; }
		void Apply( CTouchItem* pTouchItem ) const { REQUIRE( CanApply() ); pTouchItem->SetDestAttributeFlag( m_attrFlag, BST_CHECKED == m_checkState ); }
	public:
		UINT m_ctrlId;
		CFile::Attribute m_attrFlag;
	private:
		UINT m_checkState;
		static const UINT s_invalid;
	};


	template< typename ContainerT >
	typename void ClearAll( ContainerT& rStates )
	{
		std::for_each( rStates.begin(), rStates.end(), std::mem_fun_ref( &typename ContainerT::value_type::Clear ) );
	}

	template< typename ContainerT >
	typename void SetInvalidAll( ContainerT& rStates )
	{
		std::for_each( rStates.begin(), rStates.end(), std::mem_fun_ref( &typename ContainerT::value_type::SetInvalid ) );
	}

	template< typename ContainerT >
	typename ContainerT::pointer FindWithCtrlId( ContainerT& rStates, UINT ctrlId )
	{
		for ( ContainerT::iterator itState = rStates.begin(); itState != rStates.end(); ++itState )
			if ( itState->m_ctrlId == ctrlId )
				return &*itState;

		return NULL;
	}

	BYTE EvalWouldBeAttributes( const std::vector< multi::CAttribCheckState >& attribCheckStates, const CTouchItem* pTouchItem );
}


#endif // TouchItem_h
