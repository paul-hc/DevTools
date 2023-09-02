
#include "pch.h"
#include "ImageCommandLookup.h"
#include "CmdInfoStore.h"
#include "MenuUtilities.h"
#include "utl/Algorithms.h"
#include <afxcommandmanager.h>
#include <afxmdiframewndex.h>
#include <afxframewndex.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace nosy
{
	struct CCommandManager_ : public CCommandManager
	{
		// public access
		const CMap<UINT, UINT, int, int>* GetCommandIndex( void ) const { return &m_CommandIndex; }
	};
}


namespace mfc
{
	CImageCommandLookup::CImageCommandLookup( void )
	{
		if ( nullptr == GetCmdMgr() )
			return;

		// build the reverse map: imageIndex -> command
		const CMap<UINT, UINT, int, int>* pDefaultImageMap = mfc::nosy_cast<nosy::CCommandManager_>( GetCmdMgr() )->GetCommandIndex();

		for ( POSITION pos = pDefaultImageMap->GetStartPosition(); pos != nullptr; )
		{
			UINT cmdId;
			int imageIndex;

			pDefaultImageMap->GetNextAssoc( pos, cmdId, imageIndex );
			m_imagePosToCommand[imageIndex] = cmdId;
		}

		LoadCommandNames();
	}

	UINT CImageCommandLookup::FindCommand( int imagePos ) const
	{
		return utl::FindValue( m_imagePosToCommand, imagePos );
	}

	const std::tstring* CImageCommandLookup::FindCommandName( UINT cmdId ) const
	{
		return utl::FindValuePtr( m_cmdToName, cmdId );
	}

	const std::tstring* CImageCommandLookup::FindCommandNameByPos( int imagePos ) const
	{
		if ( UINT cmdId = FindCommand( imagePos ) )
			if ( const std::tstring* pCmdName = FindCommandName( cmdId ) )
				return pCmdName;

		return nullptr;
	}

	void CImageCommandLookup::LoadCommandNames( void )
	{
		SetupFromMenus();

		for ( std::unordered_map<int, UINT>::const_iterator it = m_imagePosToCommand.begin(); it != m_imagePosToCommand.end(); ++it )
			if ( nullptr == FindCommandName( it->second ) )		// not registered yet?
			{
				ui::CCmdInfo cmdTags( it->second );

				m_cmdToName[it->second] = cmdTags.m_tooltipText;
			}
	}

	void CImageCommandLookup::SetupFromMenus( void )
	{
		// add menu items from the menus of all application document templates
		if ( CDocManager* pDocManager = AfxGetApp()->m_pDocManager )
			// walk all templates in the application:
			for ( POSITION pos = pDocManager->GetFirstDocTemplatePosition(); pos != nullptr; )
				if ( const CMultiDocTemplate* pTemplate = checked_static_cast<const CMultiDocTemplate*>( pDocManager->GetNextDocTemplate( pos ) ) )
					if ( CMenu* pDocMenu = CMenu::FromHandle( pTemplate->m_hMenuShared ) )
						AddMenuCommands( pDocMenu, FALSE );

		// add commands from the default menu:
		CWnd* pMainWnd = AfxGetMainWnd();
		CMenu* pFrameMenu = nullptr;

		if ( CFrameWnd* pMainFrame = dynamic_cast<CFrameWnd*>( pMainWnd ) )
			pFrameMenu = CMenu::FromHandle( pMainFrame->m_hMenuDefault );
		else
		{
			const CMFCMenuBar* pMenuBar = nullptr;

			if ( CMDIFrameWndEx* pMainMdiFrame = dynamic_cast<CMDIFrameWndEx*>( pMainWnd ) )
				pMenuBar = pMainMdiFrame->GetMenuBar();
			else if ( CFrameWndEx* pMainFrameEx = dynamic_cast<CFrameWndEx*>( pMainWnd ) )
				pMenuBar = pMainFrameEx->GetMenuBar();

			if ( pMenuBar != nullptr )
				pFrameMenu = CMenu::FromHandle( pMenuBar->GetDefaultMenu() );
		}

		if ( nullptr == pFrameMenu )
			pFrameMenu = AfxGetMainWnd()->GetMenu();

		if ( pFrameMenu != nullptr )
			AddMenuCommands( pFrameMenu, false );
	}

	void CImageCommandLookup::AddMenuCommands( const CMenu* pMenu, bool isPopup, const std::tstring& menuPath /*= str::GetEmpty()*/ )
	{
		ASSERT_PTR( pMenu );

		for ( int i = 0, count = pMenu->GetMenuItemCount(); i != count; ++i )
		{
			UINT cmdId = pMenu->GetMenuItemID( i );

			if ( cmdId != 0 )		// not a separator?
			{
				std::tstring itemText = ui::GetMenuItemText( pMenu, i, MF_BYPOSITION );
				size_t posTab = itemText.find_first_of( _T("<\t") );

				if ( posTab != std::tstring::npos )
					itemText.erase( posTab );

				str::Replace( itemText, _T("&"), _T("") );
				str::Trim( itemText );

				if ( !menuPath.empty() )
					itemText = menuPath + _T(" > ") + itemText;

				if ( -1 == cmdId )		// sub-menu?
					AddMenuCommands( pMenu->GetSubMenu( i ), isPopup, itemText );
				else if ( nullptr == FindCommandName( cmdId ) )		// not registered yet?
					m_cmdToName[cmdId] = itemText;
			}
		}
	}
}
