#ifndef WndUtils_h
#define WndUtils_h
#pragma once

#include <hash_map>
#include "utl/Process.h"
#include "utl/Utilities.h"


namespace wnd
{
	inline std::tstring FormatWindowHandle( HWND hWnd ) { return str::Format( _T("%08X"), hWnd ); }

	std::tstring GetWindowText( HWND hWnd );
	std::tstring FormatWindowTextLine( HWND hWnd, size_t maxLen = 64 );
	HICON GetWindowIcon( HWND hWnd );

	HWND GetTopLevelParent( HWND hWnd );

	// attached thread input
	bool ShowWindow( HWND hWnd, int cmdShow );
	bool Activate( HWND hWnd );
	bool MoveWindowUp( HWND hWnd );
	bool MoveWindowDown( HWND hWnd );
	bool MoveWindowToTop( HWND hWnd );
	bool MoveWindowToBottom( HWND hWnd );


	bool HasUIPI( void );								// Windows 8+; expect slow windows due to process isolation


	// workaround for Windows 10 UIPI: cache window caption and icons for slow windows; windows belonging to processes with different elevation respond slowly to WM_GETTEXT, WM_GETICON, etc
	class CWindowInfoStore
	{
		CWindowInfoStore( void ) {}
	public:
		static CWindowInfoStore& Instance( void );

		std::tstring LookupCaption( HWND hWnd );
		HICON LookupIcon( HWND hWnd );
	private:
		struct CWindowInfo
		{
			CWindowInfo( void ) : m_hIcon( NULL ) {}
			CWindowInfo( const std::tstring& caption, HICON hIcon ) : m_caption( caption ), m_hIcon( hIcon ) {}
		public:
			std::tstring m_caption;
			HICON m_hIcon;
		};

		const CWindowInfo* FindInfo( HWND hWnd ) const;

		static std::tstring FormatContext( HWND hWnd, const TCHAR sectionName[] );
		static HICON ExtractIcon( HWND hWnd );
	private:
		stdext::hash_map< HWND, CWindowInfo > m_slowCache;			// caches icons for windows that react slowly to WM_GETICON, e.g. some elevated processes running in Windows 10

		static double s_timeoutSecs;			// threshold for slow windows (0.5 seconds)
	};
}


#endif // WndUtils_h
