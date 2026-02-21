
#include "pch.h"
#include "EditLinkItem.h"
#include "utl/FlagTags.h"
#include "utl/UI/Shortcut.h"
#include "utl/UI/ImageProxies.h"
#include "utl/UI/ImageEdit.h"
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
