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
		static const CImageCommandLookup* Instance( void );

		UINT FindCommand( int imagePos ) const;
		const std::tstring* FindCommandName( UINT cmdId ) const;
		const std::tstring* FindCommandLiteral( UINT cmdId ) const;

		const std::tstring* FindCommandNameByPos( int imagePos ) const;
	private:
		void LoadCommandNames( void );

		void SetupFromMenus( void );
		void AddMenuCommands( const CMenu* pMenu, const std::tstring& menuPath = str::GetEmpty() );
		void RegisterCmdLiterals( void );		// just for MFC + UTL UI standard commands
	private:
		std::unordered_map<int, UINT> m_imagePosToCommand;
		std::unordered_map<UINT, std::tstring> m_cmdToName;
		std::unordered_map<UINT, std::tstring> m_cmdToLiteral;	// ID_FILE_OPEN, ID_REFRESH, etc
	};
}


#endif // ImageCommandLookup_h
