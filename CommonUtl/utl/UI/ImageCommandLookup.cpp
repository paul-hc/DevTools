
#include "pch.h"
#include "ImageCommandLookup.h"
#include "ControlBar_fwd.h"
#include "CmdTagStore.h"
#include "MenuUtilities.h"
#include "resource.h"			// for IDR_STD_UTL_UI_MENU
#include "utl/Algorithms.h"
#include <afxcommandmanager.h>
#include <afxmdiframewndex.h>
#include <afxframewndex.h>

#ifdef _DEBUG
namespace dbg { std::string MapToString( const CMap<UINT, UINT, int, int>& cmdToIndexMap ); }
#define new DEBUG_NEW
#endif


#define REGISTER_LITERAL( mapCmdToLiteral, id )		(mapCmdToLiteral)[ id ] = _T(#id)


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

		//TRACE( "\nCMFCToolBar::m_DefaultImages map - %s\n", dbg::MapToString( mfc::ToolBar_GetDefaultImages() ).c_str() );
		//TRACE( "\nCCommandManager::m_CommandIndex map - %s\n", dbg::MapToString( *pDefaultImageMap ).c_str() );

		UINT cmdId;
		int imageIndex;

		for ( const CMap<UINT, UINT, int, int>::CPair* pPair = pDefaultImageMap->PGetFirstAssoc(); pPair != nullptr; pPair = pDefaultImageMap->PGetNextAssoc( pPair ) )
		{
			cmdId = pPair->key;
			imageIndex = pPair->value;

			m_imagePosToCommand[imageIndex] = cmdId;
		}

		LoadCommandNames();
	}

	const CImageCommandLookup* CImageCommandLookup::Instance( void )
	{
		static CImageCommandLookup s_imageCmdLookup;
		return &s_imageCmdLookup;
	}

	UINT CImageCommandLookup::FindCommand( int imagePos ) const
	{
		return utl::FindValue( m_imagePosToCommand, imagePos );
	}

	const std::tstring* CImageCommandLookup::FindCommandName( UINT cmdId ) const
	{
		return utl::FindValuePtr( m_cmdToName, cmdId );
	}

	const std::tstring* CImageCommandLookup::FindCommandLiteral( UINT cmdId ) const
	{
		return utl::FindValuePtr( m_cmdToLiteral, cmdId );
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
				ui::CCmdTag cmdTag( it->second );

				m_cmdToName[ it->second ] = cmdTag.m_tooltipText;
			}

		RegisterCmdLiterals();
	}

	void CImageCommandLookup::SetupFromMenus( void )
	{
		{
			CMenu stdUiMenu;

			VERIFY( stdUiMenu.LoadMenu( IDR_STD_UTL_UI_MENU ) );
			AddMenuCommands( &stdUiMenu );
		}

		// add menu items from the menus of all application document templates
		if ( CDocManager* pDocManager = AfxGetApp()->m_pDocManager )
			// walk all templates in the application:
			for ( POSITION pos = pDocManager->GetFirstDocTemplatePosition(); pos != nullptr; )
				if ( const CMultiDocTemplate* pTemplate = checked_static_cast<const CMultiDocTemplate*>( pDocManager->GetNextDocTemplate( pos ) ) )
					if ( CMenu* pDocMenu = CMenu::FromHandle( pTemplate->m_hMenuShared ) )
						AddMenuCommands( pDocMenu );

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
			AddMenuCommands( pFrameMenu );
	}

	void CImageCommandLookup::AddMenuCommands( const CMenu* pMenu, const std::tstring& menuPath /*= str::GetEmpty()*/ )
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
					AddMenuCommands( pMenu->GetSubMenu( i ), itemText );
				else if ( nullptr == FindCommandName( cmdId ) )		// not registered yet?
				{
					ASSERT( !itemText.empty() );
					m_cmdToName[cmdId] = itemText;
				}
			}
		}
	}

	void CImageCommandLookup::RegisterCmdLiterals( void )
	{
		// afxres.h IDs:
		REGISTER_LITERAL( m_cmdToLiteral, AFX_IDS_APP_TITLE );
		REGISTER_LITERAL( m_cmdToLiteral, AFX_IDS_IDLEMESSAGE );
		REGISTER_LITERAL( m_cmdToLiteral, AFX_IDS_HELPMODEMESSAGE );
		REGISTER_LITERAL( m_cmdToLiteral, AFX_IDS_APP_TITLE_EMBEDDING );
		REGISTER_LITERAL( m_cmdToLiteral, AFX_IDS_COMPANY_NAME );
		REGISTER_LITERAL( m_cmdToLiteral, AFX_IDS_OBJ_TITLE_INPLACE );
	#if _MFC_VER > 0x0900		// newer MFC version?
		REGISTER_LITERAL( m_cmdToLiteral, AFX_IDS_APP_ID );
	#endif

		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_NEW );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_OPEN );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_CLOSE );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_SAVE );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_SAVE_AS );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_PAGE_SETUP );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_PRINT_SETUP );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_PRINT );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_PRINT_DIRECT );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_PRINT_PREVIEW );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_UPDATE );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_SAVE_COPY_AS );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_SEND_MAIL );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_NEW_FRAME );

		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_FIRST );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_FILE1 );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_FILE2 );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_FILE3 );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_FILE4 );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_FILE5 );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_FILE6 );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_FILE7 );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_FILE8 );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_FILE9 );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_FILE10 );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_FILE11 );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_FILE12 );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_FILE13 );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_FILE14 );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_FILE15 );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_FILE16 );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_MRU_LAST );

		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_CLEAR );
		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_CLEAR_ALL );
		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_COPY );
		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_CUT );
		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_FIND );
		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_PASTE );
		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_PASTE_LINK );
		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_PASTE_SPECIAL );
		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_REPEAT );
		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_REPLACE );
		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_SELECT_ALL );
		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_UNDO );
		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_REDO );

		REGISTER_LITERAL( m_cmdToLiteral, ID_WINDOW_NEW );
		REGISTER_LITERAL( m_cmdToLiteral, ID_WINDOW_ARRANGE );
		REGISTER_LITERAL( m_cmdToLiteral, ID_WINDOW_CASCADE );
		REGISTER_LITERAL( m_cmdToLiteral, ID_WINDOW_TILE_HORZ );
		REGISTER_LITERAL( m_cmdToLiteral, ID_WINDOW_TILE_VERT );
		REGISTER_LITERAL( m_cmdToLiteral, ID_WINDOW_SPLIT );

		REGISTER_LITERAL( m_cmdToLiteral, AFX_IDM_WINDOW_FIRST );
		REGISTER_LITERAL( m_cmdToLiteral, AFX_IDM_WINDOW_LAST );
		REGISTER_LITERAL( m_cmdToLiteral, AFX_IDM_FIRST_MDICHILD );

		REGISTER_LITERAL( m_cmdToLiteral, ID_APP_ABOUT );
		REGISTER_LITERAL( m_cmdToLiteral, ID_APP_EXIT );
		REGISTER_LITERAL( m_cmdToLiteral, ID_HELP_INDEX );
		REGISTER_LITERAL( m_cmdToLiteral, ID_HELP_FINDER );
		REGISTER_LITERAL( m_cmdToLiteral, ID_HELP_USING );
		REGISTER_LITERAL( m_cmdToLiteral, ID_CONTEXT_HELP );
		REGISTER_LITERAL( m_cmdToLiteral, ID_HELP );
		REGISTER_LITERAL( m_cmdToLiteral, ID_DEFAULT_HELP );
		REGISTER_LITERAL( m_cmdToLiteral, ID_NEXT_PANE );
		REGISTER_LITERAL( m_cmdToLiteral, ID_PREV_PANE );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FORMAT_FONT );

		REGISTER_LITERAL( m_cmdToLiteral, ID_OLE_INSERT_NEW );
		REGISTER_LITERAL( m_cmdToLiteral, ID_OLE_EDIT_LINKS );
		REGISTER_LITERAL( m_cmdToLiteral, ID_OLE_EDIT_CONVERT );
		REGISTER_LITERAL( m_cmdToLiteral, ID_OLE_EDIT_CHANGE_ICON );
		REGISTER_LITERAL( m_cmdToLiteral, ID_OLE_EDIT_PROPERTIES );
		REGISTER_LITERAL( m_cmdToLiteral, ID_OLE_VERB_FIRST );
		REGISTER_LITERAL( m_cmdToLiteral, ID_OLE_VERB_LAST );
	#if _MFC_VER > 0x0900		// newer MFC version?
		REGISTER_LITERAL( m_cmdToLiteral, ID_OLE_VERB_POPUP );
	#endif

		REGISTER_LITERAL( m_cmdToLiteral, AFX_ID_PREVIEW_CLOSE );
		REGISTER_LITERAL( m_cmdToLiteral, AFX_ID_PREVIEW_NUMPAGE );
		REGISTER_LITERAL( m_cmdToLiteral, AFX_ID_PREVIEW_NEXT );
		REGISTER_LITERAL( m_cmdToLiteral, AFX_ID_PREVIEW_PREV );
		REGISTER_LITERAL( m_cmdToLiteral, AFX_ID_PREVIEW_PRINT );
		REGISTER_LITERAL( m_cmdToLiteral, AFX_ID_PREVIEW_ZOOMIN );
		REGISTER_LITERAL( m_cmdToLiteral, AFX_ID_PREVIEW_ZOOMOUT );
		REGISTER_LITERAL( m_cmdToLiteral, ID_VIEW_TOOLBAR );
		REGISTER_LITERAL( m_cmdToLiteral, ID_VIEW_STATUS_BAR );
		REGISTER_LITERAL( m_cmdToLiteral, ID_VIEW_REBAR );
		REGISTER_LITERAL( m_cmdToLiteral, ID_VIEW_AUTOARRANGE );
		REGISTER_LITERAL( m_cmdToLiteral, ID_VIEW_SMALLICON );
		REGISTER_LITERAL( m_cmdToLiteral, ID_VIEW_LARGEICON );
		REGISTER_LITERAL( m_cmdToLiteral, ID_VIEW_LIST );
		REGISTER_LITERAL( m_cmdToLiteral, ID_VIEW_DETAILS );
		REGISTER_LITERAL( m_cmdToLiteral, ID_VIEW_LINEUP );
		REGISTER_LITERAL( m_cmdToLiteral, ID_VIEW_BYNAME );

		REGISTER_LITERAL( m_cmdToLiteral, ID_RECORD_FIRST );
		REGISTER_LITERAL( m_cmdToLiteral, ID_RECORD_LAST );
		REGISTER_LITERAL( m_cmdToLiteral, ID_RECORD_NEXT );
		REGISTER_LITERAL( m_cmdToLiteral, ID_RECORD_PREV );
		m_cmdToLiteral[ 0xFFFF ] = _T("IDC_STATIC");

		// utl_ui.rc literals:
		REGISTER_LITERAL( m_cmdToLiteral, ID_TRANSPARENT );
		REGISTER_LITERAL( m_cmdToLiteral, ID_SELECTED_COLOR_BUTTON );

		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_PROPERTIES );
		REGISTER_LITERAL( m_cmdToLiteral, ID_FILE_PROPERTIES );
		REGISTER_LITERAL( m_cmdToLiteral, ID_EXPAND );
		REGISTER_LITERAL( m_cmdToLiteral, ID_COLLAPSE );
		REGISTER_LITERAL( m_cmdToLiteral, ID_SHELL_SUBMENU );
		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_DETAILS );
		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_LIST_ITEMS );
		REGISTER_LITERAL( m_cmdToLiteral, ID_BROWSE_FILE );
		REGISTER_LITERAL( m_cmdToLiteral, ID_BROWSE_FOLDER );
		REGISTER_LITERAL( m_cmdToLiteral, ID_RESET_DEFAULT );
		REGISTER_LITERAL( m_cmdToLiteral, ID_REFRESH );
		REGISTER_LITERAL( m_cmdToLiteral, ID_OPTIONS );
		REGISTER_LITERAL( m_cmdToLiteral, ID_RUN_TESTS );
		REGISTER_LITERAL( m_cmdToLiteral, IDD_PASSWORD_DIALOG );

		REGISTER_LITERAL( m_cmdToLiteral, ID_ADD_ITEM );
		REGISTER_LITERAL( m_cmdToLiteral, ID_REMOVE_ITEM );
		REGISTER_LITERAL( m_cmdToLiteral, ID_REMOVE_ALL_ITEMS );
		REGISTER_LITERAL( m_cmdToLiteral, ID_RENAME_ITEM );
		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_ITEM );
		REGISTER_LITERAL( m_cmdToLiteral, ID_EDIT_SELECT_ALL );
		REGISTER_LITERAL( m_cmdToLiteral, ID_MOVE_UP_ITEM );
		REGISTER_LITERAL( m_cmdToLiteral, ID_MOVE_DOWN_ITEM );
		REGISTER_LITERAL( m_cmdToLiteral, ID_MOVE_TOP_ITEM );
		REGISTER_LITERAL( m_cmdToLiteral, ID_MOVE_BOTTOM_ITEM );
		REGISTER_LITERAL( m_cmdToLiteral, ID_SHUTTLE_UP );
		REGISTER_LITERAL( m_cmdToLiteral, ID_SHUTTLE_DOWN );
		REGISTER_LITERAL( m_cmdToLiteral, ID_SHUTTLE_TOP );
		REGISTER_LITERAL( m_cmdToLiteral, ID_SHUTTLE_BOTTOM );
		REGISTER_LITERAL( m_cmdToLiteral, ID_LIST_VIEW_ICON_LARGE );
		REGISTER_LITERAL( m_cmdToLiteral, ID_LIST_VIEW_ICON_SMALL );
		REGISTER_LITERAL( m_cmdToLiteral, ID_LIST_VIEW_LIST );
		REGISTER_LITERAL( m_cmdToLiteral, ID_LIST_VIEW_REPORT );
		REGISTER_LITERAL( m_cmdToLiteral, ID_LIST_VIEW_TILE );
	}
}


#ifdef _DEBUG

#include <map>

namespace dbg
{
	std::string MapToString( const CMap<UINT, UINT, int, int>& cmdToIndexMap )
	{
		std::map<int, UINT> cmdIds;

		for ( const CMap<UINT, UINT, int, int>::CPair* pPair = cmdToIndexMap.PGetFirstAssoc(); pPair != nullptr; pPair = cmdToIndexMap.PGetNextAssoc( pPair ) )
			cmdIds[pPair->value] = pPair->key;

		std::ostringstream oss;
		size_t i = 0;

		oss << "Total: " << cmdIds.size() << " map commands:" << std::endl;

		for ( std::map<int, UINT>::const_iterator it = cmdIds.begin(); it != cmdIds.end(); ++it )
		{
			if ( i++ != 0 )
				oss << _T(",") << std::endl;

			oss << '[' << it->first << "]=" << it->second;

			if ( it->second >= 0xE000 )
				oss << str::Format( _T("  0x%04X"), it->second );		// File commands from afxres.h, e.g. ID_FILE_NEW
		}
		return oss.str();
	}
}

#endif //_DEBUG
