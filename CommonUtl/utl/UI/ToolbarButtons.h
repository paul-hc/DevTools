#ifndef ToolbarButtons_h
#define ToolbarButtons_h
#pragma once

#include "utl/StdHashValue.h"
#include "ui_fwd.h"
#include "DataAdapters.h"
#include <afxtoolbarcomboboxbutton.h>
#include <unordered_map>


class CEnumTags;


namespace mfc
{
	std::tstring GetComboSelText( const CMFCToolBarComboBoxButton* pComboBtn, ui::ComboField byField = ui::BySel );
	std::pair<bool, ui::ComboField> SetComboEditText( OUT CMFCToolBarComboBoxButton* pComboBtn, const std::tstring& newText );
	void WriteComboItems( OUT CMFCToolBarComboBoxButton* pComboBtn, const std::vector<std::tstring>& items );

	DWORD ToolBarComboBoxButton_GetStyle( const CMFCToolBarComboBoxButton* pComboButton );


	// App singleton that stores all custom toolbar button pointers that must be re-bound when CToolBar::LoadState() - deserializing.
	// The custom button class must override CMyCustomButton::OnChangeParentWnd() and rebind the pointer data-members, e.g. "CEnumTags* m_pEnumTags".
	// IMP: the client code is repsonsible that stored pointers must be valid during the lifetime of custom buttons!

	class CToolbarButtonsRefBinder
	{
		CToolbarButtonsRefBinder( void ) {}
	public:
		static CToolbarButtonsRefBinder* Instance( void );

		void RegisterPointer( UINT btnId, int ptrPos, const void* pRebindableData );

		template< typename Type >
		bool RebindPointer( OUT Type*& rPtr, UINT btnId, int ptrPos ) const
		{
			rPtr = reinterpret_cast<Type*>( RebindPointerData( btnId, ptrPos ) );
			ENSURE( nullptr == rPtr || is_a<Type>( rPtr ) );		// we didn't mess-up the ptrPos sub-key?
			return rPtr != nullptr;
		}
	private:
		void* RebindPointerData( UINT btnId, int ptrPos ) const;
	private:
		typedef std::pair<UINT, int> TBtnId_PtrPosPair;			// <btnId, ptrPos> - e.g. ptrPos is 0 for pointer data-member 1, 1 for pointer data-member 2
		typedef std::unordered_map<TBtnId_PtrPosPair, void*, utl::CPairHasher> TRebindMap;

		TRebindMap m_btnRebindPointers;		// <BtnId_PtrPosPair, pointer> - NULL data pointers are allowed as valid values
	};
}


namespace mfc
{
	class CEnumComboBoxButton : public CMFCToolBarComboBoxButton
	{
		DECLARE_SERIAL( CEnumComboBoxButton )
	protected:
		CEnumComboBoxButton( void );
	public:
		CEnumComboBoxButton( UINT btnId, const CEnumTags* pEnumTags, int width, DWORD dwStyle = CBS_DROPDOWNLIST );
		virtual ~CEnumComboBoxButton();

		bool HasValidValue( void ) const { return GetCurSel() != -1; }
		int GetValue( void ) const;
		bool SetValue( int value );

		template< typename EnumType >
		EnumType GetEnum( void ) const { return (EnumType)GetValue(); }

		template<>
		bool GetEnum<bool>( void ) const { return GetValue() != 0; }

		const CEnumTags* GetTags( void ) const { return m_pEnumTags; }
		void SetTags( const CEnumTags* pEnumTags );

		// base overrides
		virtual void OnChangeParentWnd( CWnd* pWndParent );
		virtual BOOL CanBeStretched( void ) const { return true; }
	protected:
		virtual void CopyFrom( const CMFCToolBarButton& src ) overrides(CMFCToolBarComboBoxButton);
	private:
		rebound const CEnumTags* m_pEnumTags;
	};
}


namespace mfc
{
	class CStockValuesComboBoxButton : public CMFCToolBarComboBoxButton
	{
		DECLARE_SERIAL( CStockValuesComboBoxButton )
	protected:
		CStockValuesComboBoxButton( void );
	public:
		CStockValuesComboBoxButton( UINT btnId, const ui::IStockTags* pStockTags, int width, DWORD dwStyle = CBS_DROPDOWN | CBS_DISABLENOSCROLL );
		virtual ~CStockValuesComboBoxButton();

		const ui::IStockTags* GetTags( void ) const { return m_pStockTags; }

		// input/output
		template< typename ValueT >
		bool OutputValue( ValueT value );

		template< typename ValueT >
		bool InputValue( OUT ValueT* pOutValue, ui::ComboField byField, bool showErrors = true ) const;		// ui::BySel for CBN_SELCHANGE, ui::ByEdit
	private:
		template< typename ValueT >
		typename const ui::CStockTags<ValueT>* GetTagsAs( void ) const { return checked_static_cast< const ui::CStockTags<ValueT>* >( m_pStockTags ); }

		void SetTags( const ui::IStockTags* pStockTags );

		bool OutputTag( const std::tstring& tag );
		void OnInputError( void ) const;

		// base overrides
	public:
		virtual void OnChangeParentWnd( CWnd* pWndParent );
		virtual BOOL CanBeStretched( void ) const { return true; }
	protected:
		virtual void CopyFrom( const CMFCToolBarButton& src ) overrides(CMFCToolBarComboBoxButton);
	private:
		rebound const ui::IStockTags* m_pStockTags;
	};


	// CStockValuesComboBoxButton template code

	template< typename ValueT >
	bool CStockValuesComboBoxButton::OutputValue( ValueT value )
	{
		if ( const ui::CStockTags<ValueT>* pStockTags = GetTagsAs<ValueT>() )
			return OutputTag( pStockTags->FormatValue( value ) );

		ASSERT( false );
		return false;
	}

	template< typename ValueT >
	bool CStockValuesComboBoxButton::InputValue( OUT ValueT* pOutValue, ui::ComboField byField, bool showErrors /*= true*/ ) const
	{
		if ( const ui::CStockTags<ValueT>* pStockTags = GetTagsAs<ValueT>() )
		{
			std::tstring currentTag = mfc::GetComboSelText( this, byField );
			if ( currentTag.empty() )
				return false;			// transient empty text, e.g. when the combo list is still dropped down, etc

			if ( pStockTags->ParseValue( pOutValue, currentTag ) )
				if ( pStockTags->IsValidValue( *pOutValue ) )
					return true;

			OnInputError();		// give owner a chance to restore previous valid value

			if ( showErrors )
				return ui::ShowInputError( GetComboBox(), pStockTags->FormatValidationError(), MB_ICONERROR );		// invalid input
		}
		else
			ASSERT( false );

		return false;
	}
}


#endif // ToolbarButtons_h
