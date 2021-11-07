#ifndef IdeUtilities_h
#define IdeUtilities_h
#pragma once

#include "utl/Path.h"


class CEnumTags;


namespace ide
{
	CWnd* GetFocusWindow( void );
	CWnd* GetMainWindow( CWnd* pStartingWnd = GetFocusWindow() );

	enum IdeType { VC_60, VC_71to90, VC_110plus };

	IdeType FindIdeType( CWnd* pMainWnd = GetMainWindow() );


	class CScopedWindow
	{
	public:
		CScopedWindow( void );
		~CScopedWindow();

		CWnd* GetFocusWnd( void ) const { return m_pFocusWnd; }
		CWnd* GetMainWnd( void ) const { return m_pMainWnd; }

		bool IsValid( void ) const { return m_pFocusWnd != NULL; }
		bool HasDifferentThread( void ) const { return m_hasDifferentThread; }
		IdeType GetType( void ) const { return m_ideType; }
		std::tstring FormatInfo( void ) const;

		UINT TrackPopupMenu( CMenu& rMenu, CPoint screenPos, UINT flags = TPM_RIGHTBUTTON );

		static std::tstring FormatWndInfo( HWND hWnd );
		static const CEnumTags& GetTags_IdeType( void );
	private:
		bool RestoreFocusWnd( void );
	private:
		CWnd* m_pFocusWnd;
		CWnd* m_pMainWnd;
		bool m_hasDifferentThread;			// VC 7.1+: macros (this thread) are executed on a different thread than the IDE windows
		IdeType m_ideType;
	};


	CPoint GetMouseScreenPos( void );
	std::pair<HMENU, int> FindPopupMenuWithCommand( HWND hWnd, UINT commandID );


	std::tstring GetRegistryPath_VC6( const TCHAR entry[] );
	std::tstring GetRegistryPath_VC71( const TCHAR entry[] );
	const std::tstring& GetVC71InstallDir( void );


	namespace vs6
	{
		fs::CPath GetCommonDirPath( bool trailSlash = true );
		fs::CPath GetMacrosDirPath( bool trailSlash = true );
		fs::CPath GetVC98DirPath( bool trailSlash = true );
	}
}


#endif // IdeUtilities_h
