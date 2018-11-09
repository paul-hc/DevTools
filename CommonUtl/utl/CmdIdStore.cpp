
#include "stdafx.h"
#include "CmdIdStore.h"
#include "MenuUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	void CCmdIdStore::RegisterCommands( HMENU hMenu, RecursionDepth depth /*= Deep*/ )
	{
		ASSERT_PTR( ::IsMenu( hMenu ) );

		for ( int i = 0, count = ::GetMenuItemCount( hMenu ); i != count; ++i )
		{
			MENUITEMINFO itemInfo;

			if ( ui::GetMenuItemInfo( &itemInfo, hMenu, i ) )
			{
				if ( itemInfo.wID != 0 && !HasFlag( itemInfo.fType, MFT_SEPARATOR /*| MFT_MENUBARBREAK | MFT_MENUBREAK*/ ) )
					m_cmdIds.insert( ui::ToCmdId( itemInfo.wID ) );
			}
			else
				ASSERT( false );

			if ( Deep == depth && itemInfo.hSubMenu != NULL )
				RegisterCommands( itemInfo.hSubMenu, Deep );		// trace the sub-menu
		}
	}

	size_t CCmdIdStore::Union( const CCmdIdStore& store )
	{
		size_t oldSize = m_cmdIds.size();

		for ( stdext::hash_set< int >::const_iterator itCmdId = store.m_cmdIds.begin(); itCmdId != store.m_cmdIds.end(); ++itCmdId )
			m_cmdIds.insert( *itCmdId );

		return m_cmdIds.size() - oldSize;		// added count
	}

	size_t CCmdIdStore::Subtract( const CCmdIdStore& store )
	{
		size_t oldSize = m_cmdIds.size();

		for ( stdext::hash_set< int >::const_iterator itCmdId = store.m_cmdIds.begin(); itCmdId != store.m_cmdIds.end(); ++itCmdId )
			m_cmdIds.erase( *itCmdId );

		return m_cmdIds.size() - oldSize;		// subtracted count
	}
}
