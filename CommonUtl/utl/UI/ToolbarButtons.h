#ifndef ToolbarButtons_h
#define ToolbarButtons_h
#pragma once

#include "utl/StdHashValue.h"
#include "ui_fwd.h"
#include <afxtoolbarcomboboxbutton.h>
#include <unordered_map>


class CEnumTags;


namespace mfc
{
	void WriteComboItems( OUT CMFCToolBarComboBoxButton& rComboButton, const std::vector<std::tstring>& items );
	std::pair<bool, ui::ComboField> SetComboEditText( OUT CMFCToolBarComboBoxButton& rComboButton, const std::tstring& currText );

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


interface IValueTags
{
	virtual void QueryStockTags( OUT std::vector<std::tstring>& rTags ) const = 0;
	virtual std::tstring GetTag( void ) const = 0;
	virtual bool StoreTag( const std::tstring& tag ) = 0;
};


template< typename ValueT >
interface IValueAdapter
{
	virtual std::tstring FormatValue( const ValueT& value ) const = 0;
	virtual bool ParseValue( OUT ValueT* pOutValue, const std::tstring& text ) const = 0;
};


template< typename ValueT >
class CStockValues : public IValueTags
{
public:
	CStockValues( IValueAdapter<ValueT> pAdapter, ValueT value, const TCHAR* pStockTags )
		: m_pAdapter( pAdapter )
		, m_value( value )
	{
		SplitStockTags( pStockTags );
	}

	~CStockValues() {}

	ValueT GetValue( void ) const { return m_value; }
	void SetValue( ValueT value ) { m_value = value; }


	// IValueTags interface

	virtual void QueryStockTags( OUT std::vector<std::tstring>& rTags ) const implement
	{
		rTags.reserve( m_stockValues.size() );
		for ( typename std::vector<ValueT>::const_iterator itValue = m_stockValues.begin(); itValue != m_stockValues.end(); ++itValue )
			rTags.push_back( m_pAdapter->FormatValue( *itValue ) );
	}

	virtual std::tstring GetTag( void ) const implement
	{
		return m_pAdapter->FormatValue( m_value );
	}

	virtual bool StoreTag( const std::tstring& tag ) implement
	{
		return m_pAdapter->ParseValue( &m_value, tag );
	}


	void SplitStockTags( const TCHAR* pStockTags )
	{
		std::vector<std::tstring> stockTags;
		str::Split( stockTags, pStockTags, _T("|") );

		m_stockValues.reserve( stockTags.size() );
		for ( std::vector<std::tstring>::const_iterator itTag = stockTags.begin(); itTag != stockTags.end(); ++itTag )
		{
			ValueT value;

			if ( m_pAdapter->ParseValue( &value, *itTag ) )
				m_stockValues.push_back( value );
			else
				ASSERT( false );		// parsing error in pStockTags?
		}
	}
private:
	IValueAdapter<ValueT> m_pAdapter;
	ValueT m_value;
	std::vector<ValueT> m_stockValues;
};


namespace mfc
{
	class CStockValuesComboBoxButton : public CMFCToolBarComboBoxButton
	{
		DECLARE_SERIAL( CStockValuesComboBoxButton )
	protected:
		CStockValuesComboBoxButton( void );
	public:
		CStockValuesComboBoxButton( UINT btnId, IValueTags* pValueTags, int width, DWORD dwStyle = CBS_DROPDOWN | CBS_DISABLENOSCROLL );
		virtual ~CStockValuesComboBoxButton();

		const IValueTags* GetTags( void ) const { return m_pValueTags; }
		void SetTags( IValueTags* pValueTags );

		// base overrides
		virtual void OnChangeParentWnd( CWnd* pWndParent );
		virtual BOOL CanBeStretched( void ) const { return true; }
	protected:
		virtual void CopyFrom( const CMFCToolBarButton& src ) overrides(CMFCToolBarComboBoxButton);
	private:
		rebound IValueTags* m_pValueTags;
	};
}


#endif // ToolbarButtons_h
