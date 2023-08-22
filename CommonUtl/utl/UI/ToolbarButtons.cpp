
#include "pch.h"
#include "ToolbarButtons.h"
#include "ControlBar_fwd.h"
#include "PopupMenus_fwd.h"
#include "WndUtils.h"
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
	void WriteComboItems( OUT CMFCToolBarComboBoxButton* pComboBtn, const std::vector<std::tstring>& items )
	{
		ASSERT_PTR( pComboBtn );

		pComboBtn->RemoveAllItems();

		for ( std::vector<std::tstring>::const_iterator it = items.begin(); it != items.end(); ++it )
			pComboBtn->AddItem( it->c_str() );
	}

	std::pair<bool, ui::ComboField> SetComboEditText( OUT CMFCToolBarComboBoxButton* pComboBtn, const std::tstring& newText )
	{
		ASSERT_PTR( pComboBtn );

		int selIndex = pComboBtn->FindItem( newText.c_str() );
		std::pair<bool, ui::ComboField> result( newText != pComboBtn->GetText(), selIndex != CB_ERR ? ui::BySel : ui::ByEdit );

		if ( result.first )		// text will change?
		{
			pComboBtn->SelectItem( selIndex, false );		// select/unselect index preemptively, so that CMFCToolBarComboBoxButton::SetText() does not send notification
			pComboBtn->SetText( newText.c_str() );
		}

		return result;
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
			mfc::WriteComboItems( this, m_pEnumTags->GetUiTags() );
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
		, m_pStockTags( nullptr )
	{
	}

	CStockValuesComboBoxButton::CStockValuesComboBoxButton( UINT btnId, const ui::IStockTags* pStockTags, int width, DWORD dwStyle /*= CBS_DROPDOWN | CBS_DISABLENOSCROLL*/ )
		: CMFCToolBarComboBoxButton( btnId, mfc::Button_FindImageIndex( btnId ), dwStyle, width )
		, m_pStockTags( nullptr )
	{
		SetTags( pStockTags );
	}

	CStockValuesComboBoxButton::~CStockValuesComboBoxButton()
	{
		// Note: after this button get deleted, the editor host interface pointer stored in m_pStockTags will be dangling.
		//	That's ok as long as the destination button updates the editor host interface pointer to itself - via SetTags().
	}

	void CStockValuesComboBoxButton::SetTags( const ui::IStockTags* pStockTags )
	{
		m_pStockTags = pStockTags;
		mfc::CToolbarButtonsRefBinder::Instance()->RegisterPointer( m_nID, 0, m_pStockTags );

		if ( m_pStockTags != nullptr )
		{
			std::vector<std::tstring> tags;
			m_pStockTags->QueryStockTags( tags );
			mfc::WriteComboItems( this, tags );
		}
		else
			RemoveAllItems();
	}

	bool CStockValuesComboBoxButton::OutputTag( const std::tstring& tag )
	{
		std::pair<bool, ui::ComboField> result = mfc::SetComboEditText( this, tag );

		if ( HasFocus() && GetHwnd() != nullptr )
			m_pWndCombo->SetEditSel( 0, -1 );

		return result.first;		// true if changed
	}

	void CStockValuesComboBoxButton::OnInputError( void ) const
	{
		// give parent a chance to restore previous valid value
		if ( m_pWndParent->GetSafeHwnd() != nullptr )
			ui::SendCommand( m_pWndParent->GetOwner()->GetSafeHwnd(), m_nID, ui::CN_INPUTERROR, GetComboBox()->GetSafeHwnd() );
	}

	void CStockValuesComboBoxButton::OnChangeParentWnd( CWnd* pWndParent )
	{
		__super::OnChangeParentWnd( pWndParent );

		if ( nullptr == m_pStockTags )		// parent toolbar is loading state (de-serializing)?
			mfc::CToolbarButtonsRefBinder::Instance()->RebindPointer( m_pStockTags, m_nID, 0 );
	}

	void CStockValuesComboBoxButton::CopyFrom( const CMFCToolBarButton& src )
	{
		__super::CopyFrom( src );

		const CStockValuesComboBoxButton& srcButton = (const CStockValuesComboBoxButton&)src;

		SetTags( srcButton.m_pStockTags );		// will store the editor host pointer to this
		ASSERT_PTR( m_pStockTags );
	}
}
