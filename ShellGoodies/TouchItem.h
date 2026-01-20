#ifndef TouchItem_h
#define TouchItem_h
#pragma once

#include "utl/FileState.h"
#include "utl/FileStateItem.h"


class CTouchItem : public CFileStateItem
{
public:
	CTouchItem( const fs::CFileState& srcState );
	virtual ~CTouchItem();

	const fs::CFileState& GetSrcState( void ) const { return GetState(); }
	const fs::CFileState& GetDestState( void ) const { return m_destState; }

	bool IsModified( void ) const { return m_destState.IsValid() && m_destState != GetSrcState(); }

	fs::CFileState& RefDestState( void ) { return m_destState; }
	void Reset( void ) { m_destState = GetSrcState(); }
private:
	fs::CFileState m_destState;
};


namespace multi
{
	// accumulates common values among multiple items, starting with an invalid value (uninitialized)

	class CDateTimeState
	{
	public:
		CDateTimeState( UINT ctrlId = 0, fs::TimeField timeField = fs::ModifiedDate, const CTime& dateTimeState = CTime() )
			: m_ctrlId( ctrlId ), m_timeField( timeField ), m_dateTimeState( dateTimeState ) {}

		void Clear( void ) { m_dateTimeState = CTime(); }
		void SetInvalid( void ) { m_dateTimeState = s_invalid; }

		bool Reset( const CTime& dateTime ) { return utl::ModifyValue( m_dateTimeState, dateTime ); }
		void Accumulate( const CTime& dateTime );

		bool UpdateCtrl( CWnd* pDlg ) const;
		bool InputCtrl( CWnd* pDlg );

		bool CanApply( void ) const { REQUIRE( m_dateTimeState != s_invalid ); return m_dateTimeState.GetTime() != 0; }
		void Apply( fs::CFileState& rFileState ) const { REQUIRE( CanApply() ); rFileState.SetTimeField( m_dateTimeState, m_timeField ); }
		bool WouldModify( const CTouchItem* pTouchItem ) const;
	public:
		UINT m_ctrlId;
		fs::TimeField m_timeField;
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

		bool UpdateCtrl( CWnd* pDlg ) const;
		bool InputCtrl( CWnd* pDlg );
		UINT GetChecked( CWnd* pDlg ) const { return pDlg->IsDlgButtonChecked( m_ctrlId ); }

		bool CanApply( void ) const { REQUIRE( m_checkState != s_invalid ); return m_checkState != BST_INDETERMINATE; }
		void Apply( fs::CFileState& rFileState ) const { REQUIRE( CanApply() ); SetFlag( rFileState.m_attributes, m_attrFlag, BST_CHECKED == m_checkState ); }
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
		std::for_each( rStates.begin(), rStates.end(), std::mem_fn( &typename ContainerT::value_type::Clear ) );
	}

	template< typename ContainerT >
	typename void SetInvalidAll( ContainerT& rStates )
	{
		std::for_each( rStates.begin(), rStates.end(), std::mem_fn( &typename ContainerT::value_type::SetInvalid ) );
	}

	template< typename ContainerT >
	typename ContainerT::pointer FindWithCtrlId( ContainerT& rStates, UINT ctrlId )
	{
		for ( typename ContainerT::value_type& rState: rStates )
			if ( rState.m_ctrlId == ctrlId )
				return &rState;

		return nullptr;
	}

	BYTE EvalWouldBeAttributes( const std::vector<multi::CAttribCheckState>& attribCheckStates, const CTouchItem* pTouchItem );
}


namespace func
{
	struct AsSrcState
	{
		const fs::CFileState& operator()( const CTouchItem* pItem ) const { return pItem->GetSrcState(); }
	};


	typedef AsCreationTime TAsSrcCreationTime;
	typedef AsModifyTime TAsSrcModifyTime;
	typedef AsAccessTime TAsSrcAccessTime;


	struct AsDestCreationTime
	{
		const CTime& operator()( const CTouchItem* pTouchItem ) const { return pTouchItem->GetDestState().m_creationTime; }
	};

	struct AsDestModifyTime
	{
		const CTime& operator()( const CTouchItem* pTouchItem ) const { return pTouchItem->GetDestState().m_modifTime; }
	};

	struct AsDestAccessTime
	{
		const CTime& operator()( const CTouchItem* pTouchItem ) const { return pTouchItem->GetDestState().m_accessTime; }
	};
}


#endif // TouchItem_h
