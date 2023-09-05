#ifndef ImageCommandLookup_h
#define ImageCommandLookup_h
#pragma once


#include <unordered_map>


namespace mfc
{
	class CImageCommandLookup		// provides CCommandManager reverse lookup: imageIndex -> command
	{
		CImageCommandLookup( void );
	public:
		static CImageCommandLookup* Instance( void );

		UINT FindCommand( int imagePos ) const;
		const std::tstring* FindCommandName( UINT cmdId ) const;
		const std::tstring* FindCommandNameByPos( int imagePos ) const;

		void LoadCommandNames( void );
	private:
		void SetupFromMenus( void );
		void AddMenuCommands( const CMenu* pMenu, const std::tstring& menuPath = str::GetEmpty() );
	private:
		std::unordered_map<int, UINT> m_imagePosToCommand;
		std::unordered_map<UINT, std::tstring> m_cmdToName;
	};
}


#endif // ImageCommandLookup_h
