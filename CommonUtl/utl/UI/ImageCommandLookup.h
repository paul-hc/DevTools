#ifndef ImageCommandLookup_h
#define ImageCommandLookup_h
#pragma once


#include <unordered_map>


namespace mfc
{
	class CImageCommandLookup		// provides CCommandManager reverse lookup: imageIndex -> command
	{
	public:
		CImageCommandLookup( void );

		UINT FindCommand( int imagePos ) const;
		const std::tstring* FindCommandName( UINT cmdId ) const;
		const std::tstring* FindCommandNameByPos( int imagePos ) const;
	private:
		void LoadCommandNames( void );
		void SetupFromMenus( void );
		void AddMenuCommands( const CMenu* pMenu, bool isPopup, const std::tstring& menuPath = str::GetEmpty() );
	private:
		std::unordered_map<int, UINT> m_imagePosToCommand;
		std::unordered_map<UINT, std::tstring> m_cmdToName;
	};
}


#endif // ImageCommandLookup_h
