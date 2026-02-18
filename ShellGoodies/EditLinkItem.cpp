
#include "pch.h"
#include "EditLinkItem.h"
#include "utl/EnumTags.h"
#include "utl/FlagTags.h"
#include "utl/UI/Shortcut.h"
#include "utl/UI/EnumComboBox.h"
#include "utl/UI/PathItemEdit.h"
#include "utl/UI/TextEdit.h"
#include "utl/UI/ImageProxies.h"
#include "utl/UI/WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CEditLinkItem implementation

CEditLinkItem::CEditLinkItem( const fs::CPath& linkPath, IShellLink* pShellLink )
	: CShortcutItem( linkPath, pShellLink )
	, m_destShortcut( GetShortcut() )
{
}

CEditLinkItem::CEditLinkItem( const fs::CPath& linkPath, const shell::CShortcut& srcShortcut )
	: CShortcutItem( linkPath, srcShortcut )
	, m_destShortcut( srcShortcut )
{
}

CEditLinkItem::~CEditLinkItem()
{
}

CEditLinkItem* CEditLinkItem::LoadLinkItem( const fs::CPath& linkPath )
{
	CComPtr<IShellLink> pShellLink = shell::LoadLinkFromFile( linkPath.GetPtr() );

	return new CEditLinkItem( linkPath, pShellLink );
}

bool CEditLinkItem::HasInvalidTarget( bool isSrc ) const
{
	const shell::CShortcut& shortcut = isSrc ? GetSrcShortcut() : GetDestShortcut();

	return shortcut.HasTarget() && !shortcut.IsValidTarget();
}

bool CEditLinkItem::HasInvalidWorkDir( bool isSrc ) const
{
	const fs::TDirPath& workDirPath = isSrc ? GetSrcShortcut().GetWorkDirPath() : GetDestShortcut().GetWorkDirPath();

	return !workDirPath.IsEmpty() && !shell::ShellFolderExist( workDirPath.GetPtr() );
}

bool CEditLinkItem::HasInvalidIcon( bool isSrc ) const
{
	const fs::CPath& iconPath = isSrc ? GetSrcShortcut().GetIconLocation().m_path : GetDestShortcut().GetIconLocation().m_path;

	return !iconPath.IsEmpty() && !shell::ShellItemExist( iconPath.GetPtr() );
}


namespace fmt
{
	std::tstring FormatEditLinkEntry( const fs::CPath& linkPath, const shell::CShortcut& srcShortcut, const shell::CShortcut& destShortcut )
	{
		return linkPath.Get()
			+ _T(": {")
			+ shell::CShortcut::GetTags_Fields().FormatKey( srcShortcut.GetDiffFields( destShortcut ) )
			+ _T("}");
	}
}


namespace multi
{
	const CEnumTags& GetTags_MultiValueState( void )
	{
		static const CEnumTags s_tags( _T("<null>|<shared value>|<different options>") );
		return s_tags;
	}

	const std::tstring& GetMultipleValueTag( void )
	{
		return GetTags_MultiValueState().FormatUi( MultipleValue );
	}


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

		if ( ui::IsDisabled( m_pCtrl->GetSafeHwnd() ) || !m_pCtrl->GetModify() )
			return false;

		std::tstring value = m_pCtrl->GetText();
		MultiValueState state = m_state;

		if ( GetTags_MultiValueState().ParseUiAs( state, value ) )
			if ( MultipleValue == state )
			{	// ignore input from "<different options>" tag
				m_state = MultipleValue;
				return false;		// multiple values tag: no real input
			}

		m_value.swap( value );
		m_state = !m_value.empty() ? SharedValue : NullValue;
		return true;
	}


	// CPathValue implementation

	bool CPathValue::UpdateCtrl( void ) const
	{
		ASSERT_PTR( m_pCtrl );

		static const shell::TPath s_multiplePath = GetMultipleValueTag();
		const shell::TPath* pShellPath = m_state != MultipleValue ? &m_value : &s_multiplePath;

		bool changed = *pShellPath != m_pCtrl->GetShellPath();

		m_pCtrl->SetShellPath( *pShellPath );

		if ( !IsNullValue() )
			m_pCtrl->SetWritable( MultipleValue == m_state || ( SharedValue == m_state && !pShellPath->IsEmpty() && !pShellPath->IsGuidPath() ) );

		ui::EnableWindow( m_pCtrl->GetSafeHwnd(), !IsNullValue() );
		return changed;
	}

	bool CPathValue::InputCtrl( void )
	{
		ASSERT_PTR( m_pCtrl );

		if ( ui::IsDisabled( m_pCtrl->GetSafeHwnd() ) || !m_pCtrl->GetModify() )
			return false;

		shell::TPath pathValue = m_pCtrl->GetShellPath();
		MultiValueState state = m_state;

		if ( GetTags_MultiValueState().ParseUiAs( state, pathValue.Ref() ) )
			if ( MultipleValue == state )
			{	// ignore input from "<different options>" tag
				m_state = MultipleValue;
				return false;
			}

		m_value.Swap( pathValue );
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
		WORD vKeyCode, modifiers;

		m_pCtrl->GetHotKey( vKeyCode, modifiers );

		WORD oldValue = MAKEWORD( vKeyCode, modifiers );

		ui::EnableWindow( m_pCtrl->GetSafeHwnd(), !IsNullValue() );

		if ( IsSharedValue() )
			m_pCtrl->SetHotKey( LOBYTE( m_value ), HIBYTE( m_value ) );
		else
			m_pCtrl->SetHotKey( 0, 0 );

		return m_value != oldValue;
	}

	bool CHotKeyValue::InputCtrl( void )
	{
		ASSERT_PTR( m_pCtrl );

		if ( ui::IsDisabled( m_pCtrl->GetSafeHwnd() ) )
			return false;				// no input if disabled

		WORD vKeyCode, modifiers;
		m_pCtrl->GetHotKey( vKeyCode, modifiers );

		m_value = MAKEWORD( vKeyCode, modifiers );
		if ( m_value != 0 )
			m_state = SharedValue;		// MultipleValue -> SharedValue

		return IsSharedValue();
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


namespace single
{
	// CIconLocationValue implementation

	CIconLocationValue::~CIconLocationValue()
	{
	}

	void CIconLocationValue::Clear( void )
	{
		if ( !Set( shell::CIconLocation(), fs::CPath() ) )
		{
			ui::EnableWindow( m_pImageEdit->GetSafeHwnd(), !IsNullValue() );

			if ( m_pLargeIconStatic != nullptr )
				ui::EnableWindow( m_pLargeIconStatic->GetSafeHwnd(), !IsNullValue() );
		}
	}

	bool CIconLocationValue::Set( const shell::CIconLocation& iconLocation, const fs::CPath& linkPath )
	{
		bool changed = utl::ModifyValue( m_iconLocation, iconLocation );

		changed |= utl::ModifyValue( m_linkPath, linkPath );
		if ( !changed )
			return false;

		m_smallIcon.Clear();
		m_largeIcon.Clear();

		m_pImageEdit->SetText( m_iconLocation.Format() );

		HICON hSmallIcon = nullptr, hLargeIcon = nullptr;

		if ( !m_iconLocation.IsEmpty() )
		{	// extract the custom icon at index; returns the number of icons extracted (1 in this case)
			if ( 0 == ::ExtractIconEx( m_iconLocation.m_path.GetPtr(), m_iconLocation.m_index, &hLargeIcon, &hSmallIcon, 1 ) )
				TRACE( _T( "* CIconLocationValue::Set() - ::ExtractIconEx() failed for icon location '%'\n" ), m_iconLocation.Format().c_str() );
		}
		else
		{
			hSmallIcon = shell::ExtractShellIcon( linkPath.GetPtr(), SHGFI_SMALLICON );
			hLargeIcon = shell::ExtractShellIcon( linkPath.GetPtr(), SHGFI_LARGEICON );
		}
		m_smallIcon.StoreIcon( hSmallIcon );
		m_largeIcon.StoreIcon( hLargeIcon );

		m_pImageEdit->SetImageProxy( m_smallIcon.IsValid() ? new CIconProxy( &m_smallIcon ) : nullptr );

		if ( m_pLargeIconStatic != nullptr )
		{
			m_pLargeIconStatic->SetIcon( m_largeIcon.IsValid() ? m_largeIcon.GetHandle() : m_smallIcon.GetHandle() );
			ui::EnableWindow( m_pLargeIconStatic->GetSafeHwnd(), !IsNullValue() );
		}

		ui::EnableWindow( m_pImageEdit->GetSafeHwnd(), !IsNullValue() );

		return changed;
	}

	bool CIconLocationValue::PickIcon( void )
	{
		if ( IsNullValue() )
			return false;			// no input if null (should be disabled)

		shell::CIconLocation iconLocation = m_iconLocation;
		TCHAR iconPath[ MAX_PATH ];

		_tcscpy( iconPath, iconLocation.m_path.GetPtr() );

		if ( !PickIconDlg( m_pImageEdit->GetParent()->GetSafeHwnd(), ARRAY_SPAN( iconPath ), &iconLocation.m_index ) )
			return false;

		iconLocation.m_path.Set( iconPath );
		return Set( iconLocation, m_linkPath );
	}
}
