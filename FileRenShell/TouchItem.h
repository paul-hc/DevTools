#ifndef TouchItem_h
#define TouchItem_h
#pragma once

#include "utl/ISubject.h"
#include "utl/FileState.h"
#include "FileWorkingSet_fwd.h"


class CTouchItem : public utl::ISubject
{
public:
	CTouchItem( TFileStatePair* pStatePair, fmt::PathFormat pathFormat );
	virtual ~CTouchItem();

	const fs::CPath& GetKeyPath( void ) const { return GetSrcState().m_fullPath; }
	const fs::CFileState& GetSrcState( void ) const { return m_pStatePair->first; }
	const fs::CFileState& GetDestState( void ) const { return m_pStatePair->second; }

	bool IsModified( void ) const { return m_pStatePair->second.IsValid() && m_pStatePair->second != m_pStatePair->first; }

	// utl::ISubject interface
	virtual std::tstring GetCode( void ) const;
	virtual std::tstring GetDisplayCode( void ) const;

	void SetDestTime( app::DateTimeField field, const CTime& dateTime );
	void SetDestAttributeFlag( CFile::Attribute attrFlag, bool on ) { SetFlag( m_pStatePair->second.m_attributes, attrFlag, on ); }

	struct ToKeyPath
	{
		const fs::CPath& operator()( const fs::CPath& path ) const { return path; }
		const fs::CPath& operator()( const CTouchItem* pItem ) const { return pItem->GetKeyPath(); }
	};
private:
	TFileStatePair* m_pStatePair;
	const std::tstring m_displayPath;
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
}


#endif // TouchItem_h
