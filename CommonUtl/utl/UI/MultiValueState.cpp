
#include "pch.h"
#include "MultiValueState.h"
#include "EnumComboBox.h"
#include "HotKeyCtrlEx.h"
#include "PathItemEdit.h"
#include "TextEdit.h"
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace multi
{
	// CStringValue implementation

	bool CStringValue::UpdateCtrl( void ) const
	{
		ASSERT_PTR( m_pCtrl );
		const std::tstring* pText = m_state != MultipleValue ? &m_value : &GetMultipleValueTag();

		ui::EnableWindow( m_pCtrl->GetSafeHwnd(), !IsNullValue() );
		return m_pCtrl->SetText( *pText );
	}

	bool CStringValue::InputCtrl( void )
	{
		ASSERT_PTR( m_pCtrl );

		if ( ui::IsDisabled( m_pCtrl->GetSafeHwnd() ) || !m_pCtrl->GetModify() || m_pCtrl->InMultiValuesMode() )
			return false;

		m_value = m_pCtrl->GetText();
		m_state = !m_value.empty() ? SharedValue : NullValue;
		return true;
	}


	// CPathValue implementation

	bool CPathValue::UpdateCtrl( void ) const
	{
		ASSERT_PTR( m_pCtrl );

		bool changed = ui::EnableWindow( m_pCtrl->GetSafeHwnd(), !IsNullValue() );

		if ( !IsNullValue() )
			changed |= m_pCtrl->SetWritable( MultipleValue == m_state || ( SharedValue == m_state && !m_value.IsEmpty() && !m_value.IsGuidPath() ) );

		if ( MultipleValue == m_state )
			changed |= m_pCtrl->SetMultiValuesMode();
		else
		{
			changed |= m_value != m_pCtrl->GetShellPath();
			m_pCtrl->SetShellPath( m_value );
		}

		return changed;
	}

	bool CPathValue::InputCtrl( void )
	{
		ASSERT_PTR( m_pCtrl );

		if ( ui::IsDisabled( m_pCtrl->GetSafeHwnd() ) || !m_pCtrl->GetModify() || m_pCtrl->InMultiValuesMode() )
			return false;

		m_value = m_pCtrl->GetShellPath();
		m_state = !m_value.IsEmpty() ? SharedValue : NullValue;
		return true;
	}


	// CEnumValue implementation

	bool CEnumValue::UpdateCtrl( void ) const
	{
		ASSERT_PTR( m_pCtrl );

		ui::EnableWindow( m_pCtrl->GetSafeHwnd(), !IsNullValue() );

		if ( SharedValue == m_state )
			return m_pCtrl->SetValue( m_value );	// true if changed

		CComboBox* pComboBox = m_pCtrl;				// pointer to base class to gain visibility to GetCurSel/SetCurSel methods

		if ( CB_ERR == pComboBox->GetCurSel() )
			return false;		// no change: already no item is selected

		pComboBox->SetCurSel( CB_ERR );
		return true;
	}

	bool CEnumValue::InputCtrl( void )
	{
		ASSERT_PTR( m_pCtrl );

		if ( ui::IsDisabled( m_pCtrl->GetSafeHwnd() ) )
			return false;				// no input if disabled (NullValue)

		ASSERT( !IsNullValue() );

		int selIndex = m_pCtrl->GetValue();

		if ( CB_ERR == selIndex )
		{
			ASSERT( MultipleValue == m_state );
			return false;
		}

		ASSERT( m_pCtrl->IsValidValue( selIndex ) );

		if ( selIndex == m_value )
			return false;

		m_value = selIndex;
		m_state = SharedValue;
		return true;
	}


	// CHotKeyValue implementation

	bool CHotKeyValue::UpdateCtrl( void ) const
	{
		ASSERT_PTR( m_pCtrl );

		ui::EnableWindow( m_pCtrl->GetSafeHwnd(), !IsNullValue() );

		if ( IsNullValue() )
			return m_pCtrl->SetHotKey( 0 );
		else if ( IsMultipleValue() )
			return m_pCtrl->SetMultiValuesMode();

		return m_pCtrl->SetHotKey( m_value );
	}

	bool CHotKeyValue::InputCtrl( void )
	{
		ASSERT_PTR( m_pCtrl );

		if ( ui::IsDisabled( m_pCtrl->GetSafeHwnd() ) || !m_pCtrl->IsModified() || m_pCtrl->InMultiValuesMode() )
			return false;			// no input if disabled (null) or has multiple values or not modified

		WORD newValue = m_pCtrl->GetHotKey().AsWord();

		if ( newValue == m_value )
			return false;			// no hotkey change

		m_value = newValue;
		m_state = SharedValue;		// MultipleValue -> SharedValue
		return true;
	}


	// CFlagCheckState implementation

	bool CFlagCheckState::UpdateCtrl( void ) const
	{
		ASSERT( ::GetDlgItem( *m_pDlg, m_btnId ) != nullptr );

		UINT checkState = !IsNullValue() ? m_value : BST_UNCHECKED;
		bool changed = checkState != m_pDlg->IsDlgButtonChecked( m_btnId );

		m_pDlg->CheckDlgButton( m_btnId, checkState );
		ui::EnableControl( *m_pDlg, m_btnId, !IsNullValue() && m_editable );
		return changed;
	}

	bool CFlagCheckState::InputCtrl( void )
	{
		HWND hBtnCtrl = ::GetDlgItem( *m_pDlg, m_btnId );
		UINT checkState = m_pDlg->IsDlgButtonChecked( m_btnId );

		ASSERT_PTR( hBtnCtrl );

		if ( ui::IsDisabled( hBtnCtrl ) || BST_INDETERMINATE == checkState )
			return false;				// no input if disabled (NullValue) or indeterminate (MultipleValue)

		ASSERT( IsSharedValue() );

		if ( checkState == m_value )
			return false;

		m_value = checkState;
		//m_state = SharedValue;
		return true;
	}
}
