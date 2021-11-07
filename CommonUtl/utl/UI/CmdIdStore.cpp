
#include "stdafx.h"
#include "CmdIdStore.h"
#include "MenuUtilities.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	// CCmdIdStore implementation

	std::pair<int, int> CCmdIdStore::GetMinMaxIds( void ) const
	{
		std::pair<int, int> minMaxPair( 0, 0 );
		if ( !IsEmpty() )
		{
			std::set< int >::const_iterator itLast = m_cmdIds.end();

			minMaxPair.first = *m_cmdIds.begin();
			minMaxPair.second = *--itLast;
		}
		return minMaxPair;
	}

	void CCmdIdStore::RegisterCommands( HMENU hMenu, RecursionDepth depth /*= Deep*/ )
	{
		ASSERT_PTR( ::IsMenu( hMenu ) );

		for ( int i = 0, count = ::GetMenuItemCount( hMenu ); i != count; ++i )
		{
			ui::MENUITEMINFO_BUFF itemInfo;

			if ( itemInfo.GetMenuItemInfo( hMenu, i ) )
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

		for ( std::set< int >::const_iterator itCmdId = store.m_cmdIds.begin(); itCmdId != store.m_cmdIds.end(); ++itCmdId )
			m_cmdIds.insert( *itCmdId );

		return m_cmdIds.size() - oldSize;		// added count
	}

	size_t CCmdIdStore::Subtract( const CCmdIdStore& store )
	{
		size_t oldSize = m_cmdIds.size();

		for ( std::set< int >::const_iterator itCmdId = store.m_cmdIds.begin(); itCmdId != store.m_cmdIds.end(); ++itCmdId )
			m_cmdIds.erase( *itCmdId );

		return m_cmdIds.size() - oldSize;		// subtracted count
	}


	// CHandledNotificationsCache implementation

	bool CHandledNotificationsCache::HandlesMessage( int cmdId, UINT message, UINT notifyCode )
	{
		const CHandlerKey handlerKey( cmdId, message, notifyCode );
		utl::vector_map< CHandlerKey, bool >::const_iterator itFound = m_handledNotifyCodes.find( handlerKey );

		if ( itFound != m_handledNotifyCodes.end() )
			return itFound->second;

		bool isHandled = ui::ContainsMessageHandler( m_pCmdTarget, message, notifyCode, cmdId );

		m_handledNotifyCodes[ handlerKey ] = isHandled;
		return isHandled;
	}

	bool CHandledNotificationsCache::CHandlerKey::operator<( const CHandlerKey& right ) const
	{
		pred::CompareResult result = pred::Compare_Scalar( m_cmdId, right.m_cmdId );

		if ( pred::Equal == result )
			result = pred::Compare_Scalar( m_message, right.m_message );

		if ( pred::Equal == result )
			result = pred::Compare_Scalar( m_notifyCode, right.m_notifyCode );

		return pred::Less == result;
	}
}
