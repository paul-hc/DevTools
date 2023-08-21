
#include "pch.h"
#include "ToolbarButtons.h"
#include "ControlBar_fwd.h"
#include "PopupMenus_fwd.h"
#include "utl/Algorithms.h"
#include "utl/EnumTags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace nosy
{
	struct CToolBarComboBoxButton_ : public CMFCToolBarComboBoxButton
	{
		// public access
		using CMFCToolBarComboBoxButton::m_dwStyle;
	};
}


namespace mfc
{
	void WriteComboItems( OUT CMFCToolBarComboBoxButton& rComboButton, const std::vector<std::tstring>& items )
	{
		rComboButton.RemoveAllItems();

		for ( std::vector<std::tstring>::const_iterator it = items.begin(); it != items.end(); ++it )
			rComboButton.AddItem( it->c_str() );
	}

	std::pair<bool, ui::ComboField> SetComboEditText( OUT CMFCToolBarComboBoxButton& rComboButton, const std::tstring& currText )
	{
		int oldSelPos = rComboButton.GetCurSel();
		int foundListPos = rComboButton.FindItem( currText.c_str() );
		DWORD comboStyle = mfc::ToolBarComboBoxButton_GetStyle( &rComboButton );
		CComboBox* pComboBox = rComboButton.GetComboBox();

		if ( foundListPos != CB_ERR )
			if ( oldSelPos == foundListPos )
				return std::make_pair( false, ui::BySel );		// no change in selection
			else if ( rComboButton.SelectItem( foundListPos, false ) != CB_ERR )
			{
				if ( pComboBox->GetSafeHwnd() != nullptr )
					pComboBox->UpdateWindow();					// force redraw when mouse is tracking (no WM_PAINT sent)

				return std::make_pair( true, ui::BySel );		// select existing item from the list
			}

		if ( EqMaskedValue( comboStyle, 0x0F, CBS_DROPDOWNLIST ) )
			return std::make_pair( false, ui::BySel );			// no selection hit (combo has no edit)

		// clear selected item to mark the dirty list state for the new item
		bool selChanged = oldSelPos != CB_ERR;					// had an old selection?
		if ( selChanged )
			rComboButton.SelectItem( CB_ERR, false );			// clear selection: will clear the edit text, but it will not notify CBN_EDITCHANGE

		// change edit text
		rComboButton.SetText( currText.c_str() );

		mfc::ToolBarButton_Redraw( &rComboButton );
		return std::make_pair( selChanged, ui::ByEdit );		// changed edit text
	}


	DWORD ToolBarComboBoxButton_GetStyle( const CMFCToolBarComboBoxButton* pComboButton )
	{
		return mfc::nosy_cast<nosy::CToolBarComboBoxButton_>( pComboButton )->m_dwStyle;
	}


	// CToolbarButtonsRefBinder implementation

	CToolbarButtonsRefBinder* CToolbarButtonsRefBinder::Instance( void )
	{
		static CToolbarButtonsRefBinder s_refBinder;

		return &s_refBinder;
	}

	void CToolbarButtonsRefBinder::RegisterPointer( UINT btnId, int ptrPos, const void* pRebindableData )
	{
		m_btnRebindPointers[ TBtnId_PtrPosPair( btnId, ptrPos ) ] = const_cast<void*>( pRebindableData );
	}

	void* CToolbarButtonsRefBinder::RebindPointerData( UINT btnId, int ptrPos ) const
	{
		return utl::FindValue( m_btnRebindPointers, TBtnId_PtrPosPair( btnId, ptrPos ) );
	}
}


namespace mfc
{
	IMPLEMENT_SERIAL( CEnumComboBoxButton, CMFCToolBarComboBoxButton, 1 )

	CEnumComboBoxButton::CEnumComboBoxButton( void )
		: CMFCToolBarComboBoxButton()
		, m_pEnumTags( nullptr )
	{
	}

	CEnumComboBoxButton::CEnumComboBoxButton( UINT btnId, const CEnumTags* pEnumTags, int width, DWORD dwStyle /*= CBS_DROPDOWNLIST*/ )
		: CMFCToolBarComboBoxButton( btnId, mfc::Button_FindImageIndex( btnId ), dwStyle, width )
		, m_pEnumTags( nullptr )
	{
		SetTags( pEnumTags );
	}

	CEnumComboBoxButton::~CEnumComboBoxButton()
	{
	}

	int CEnumComboBoxButton::GetValue( void ) const
	{
		ASSERT_PTR( m_pEnumTags );

		int selIndex = GetCurSel();
		if ( -1 == selIndex )
		{
			ASSERT( HasValidValue() );
			return -1;
		}

		return m_pEnumTags->GetSelValue<int>( selIndex );
	}

	bool CEnumComboBoxButton::SetValue( int value )
	{
		ASSERT_PTR( m_pEnumTags );

		if ( HasValidValue() && value == GetValue() )
			return false;

		VERIFY( SelectItem( m_pEnumTags->GetTagIndex( value ), false ) );
		return true;
	}

	void CEnumComboBoxButton::SetTags( const CEnumTags* pEnumTags )
	{
		m_pEnumTags = pEnumTags;
		mfc::CToolbarButtonsRefBinder::Instance()->RegisterPointer( m_nID, 0, m_pEnumTags );

		if ( m_pEnumTags != nullptr )
			mfc::WriteComboItems( *this, m_pEnumTags->GetUiTags() );
		else
			RemoveAllItems();
	}

	void CEnumComboBoxButton::OnChangeParentWnd( CWnd* pWndParent )
	{
		__super::OnChangeParentWnd( pWndParent );

		if ( nullptr == m_pEnumTags )		// parent toolbar is loading state (de-serializing)?
			mfc::CToolbarButtonsRefBinder::Instance()->RebindPointer( m_pEnumTags, m_nID, 0 );
	}

	void CEnumComboBoxButton::CopyFrom( const CMFCToolBarButton& src ) overrides(CMFCToolBarComboBoxButton)
	{
		__super::CopyFrom( src );

		const CEnumComboBoxButton& srcButton = (const CEnumComboBoxButton&)src;

		m_pEnumTags = srcButton.m_pEnumTags;
		ASSERT_PTR( m_pEnumTags );
	}
}


namespace mfc
{
	// CStockValuesComboBoxButton implementation

	IMPLEMENT_SERIAL( CStockValuesComboBoxButton, CMFCToolBarComboBoxButton, 1 )

	CStockValuesComboBoxButton::CStockValuesComboBoxButton( void )
		: CMFCToolBarComboBoxButton()
		, m_pValueTags( nullptr )
	{
	}

	CStockValuesComboBoxButton::CStockValuesComboBoxButton( UINT btnId, IValueTags* pValueTags, int width, DWORD dwStyle /*= CBS_DROPDOWN | CBS_DISABLENOSCROLL*/ )
		: CMFCToolBarComboBoxButton( btnId, mfc::Button_FindImageIndex( btnId ), dwStyle, width )
		, m_pValueTags( nullptr )
	{
		SetTags( pValueTags );
	}

	CStockValuesComboBoxButton::~CStockValuesComboBoxButton()
	{
	}

	void CStockValuesComboBoxButton::SetTags( IValueTags* pValueTags )
	{
		m_pValueTags = pValueTags;
		mfc::CToolbarButtonsRefBinder::Instance()->RegisterPointer( m_nID, 0, m_pValueTags );

		if ( m_pValueTags != nullptr )
		{
			std::vector<std::tstring> tags;
			m_pValueTags->QueryStockTags( tags );
			mfc::WriteComboItems( *this, tags );
		}
		else
			RemoveAllItems();
	}

	void CStockValuesComboBoxButton::OnChangeParentWnd( CWnd* pWndParent )
	{
		__super::OnChangeParentWnd( pWndParent );

		if ( nullptr == m_pValueTags )		// parent toolbar is loading state (de-serializing)?
			mfc::CToolbarButtonsRefBinder::Instance()->RebindPointer( m_pValueTags, m_nID, 0 );
	}

	void CStockValuesComboBoxButton::CopyFrom( const CMFCToolBarButton& src )
	{
		__super::CopyFrom( src );

		const CStockValuesComboBoxButton& srcButton = (const CStockValuesComboBoxButton&)src;

		m_pValueTags = srcButton.m_pValueTags;
		ASSERT_PTR( m_pValueTags );
	}
}
