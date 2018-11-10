#ifndef CmdIdStore_h
#define CmdIdStore_h
#pragma once

#include <set>


namespace ui
{
	class CCmdIdStore
	{
	public:
		CCmdIdStore( HMENU hMenu = NULL, RecursionDepth depth = Deep ) { if ( hMenu != NULL ) RegisterCommands( hMenu, depth ); }

		bool IsEmpty( void ) const { return m_cmdIds.empty(); }

		bool ContainsId( int cmdId ) const { return m_cmdIds.find( cmdId ) != m_cmdIds.end(); }
		const std::set< int >& GetIds( void ) const { return m_cmdIds; }
		std::pair< int, int > GetMinMaxIds( void ) const;

		void RegisterCommands( HMENU hMenu, RecursionDepth depth = Deep );

		size_t Union( const CCmdIdStore& store );
		size_t Subtract( const CCmdIdStore& store );
	private:
		std::set< int > m_cmdIds;
	};
}


#endif // CmdIdStore_h
