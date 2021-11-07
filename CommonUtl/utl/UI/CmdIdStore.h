#ifndef CmdIdStore_h
#define CmdIdStore_h
#pragma once

#include <set>
#include "utl/vector_map.h"


namespace ui
{
	class CCmdIdStore
	{
	public:
		CCmdIdStore( HMENU hMenu = NULL, RecursionDepth depth = Deep ) { if ( hMenu != NULL ) RegisterCommands( hMenu, depth ); }

		bool IsEmpty( void ) const { return m_cmdIds.empty(); }

		bool ContainsId( int cmdId ) const { return m_cmdIds.find( cmdId ) != m_cmdIds.end(); }
		const std::set< int >& GetIds( void ) const { return m_cmdIds; }
		std::pair<int, int> GetMinMaxIds( void ) const;

		void RegisterCommands( HMENU hMenu, RecursionDepth depth = Deep );

		size_t Union( const CCmdIdStore& store );
		size_t Subtract( const CCmdIdStore& store );
	private:
		std::set< int > m_cmdIds;
	};


	// cached lookup message map handlers for a given command notification (WM_COMMAND/WM_NOTIFY)
	//
	class CHandledNotificationsCache : private utl::noncopyable
	{
	public:
		CHandledNotificationsCache( CCmdTarget* pCmdTarget ) : m_pCmdTarget( pCmdTarget ) { ASSERT_PTR( m_pCmdTarget ); }

		bool HandlesWmCommand( int cmdId, UINT cmdNotifyCode ) { return HandlesMessage( cmdId, WM_COMMAND, cmdNotifyCode ); }
		bool HandlesWmNotify( int cmdId, UINT wmNotifyCode ) { return HandlesMessage( cmdId, WM_NOTIFY, wmNotifyCode ); }

		bool HandlesMessage( int cmdId, UINT message, UINT notifyCode );
	private:
		struct CHandlerKey			// for the lack of std::tuple
		{
			CHandlerKey( void ) : m_cmdId( 0 ), m_message( 0 ), m_notifyCode( 0 ) {}
			CHandlerKey( int cmdId, UINT message, UINT notifyCode ) : m_cmdId( cmdId ), m_message( message ), m_notifyCode( notifyCode ) {}

			bool operator<( const CHandlerKey& right ) const;
		public:
			int m_cmdId;
			UINT m_message;			// WM_COMMAND, WM_NOTIFY, etc
			UINT m_notifyCode;
		};
	private:
		CCmdTarget* m_pCmdTarget;
		utl::vector_map< CHandlerKey, bool > m_handledNotifyCodes;		// <handler_key, handled>
	};
}


#endif // CmdIdStore_h
