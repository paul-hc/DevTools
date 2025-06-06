#ifndef WindowInfoStore_h
#define WindowInfoStore_h
#pragma once

#include <unordered_map>


// workaround for Windows 10 UIPI: cache window caption and icons for slow windows; windows belonging to processes with different elevation respond slowly to WM_GETTEXT, WM_GETICON, etc
class CWindowInfoStore
{
	CWindowInfoStore( void ) {}
public:
	static CWindowInfoStore& Instance( void );

	bool IsEmpty( void ) const { return m_slowCache.empty(); }
	bool IsSlowWindow( HWND hWnd ) const { return FindInfo( hWnd ) != nullptr; }

	std::tstring LookupCaption( HWND hWnd );
	HICON LookupIcon( HWND hWnd );
	HICON CacheIcon( HWND hWnd );			// cache window icon regarding of slowness
private:
	struct CWindowInfo
	{
		CWindowInfo( void ) : m_hIcon( nullptr ) {}
		CWindowInfo( const std::tstring& caption, HWND hWnd, HICON hIcon );
	public:
		std::tstring m_caption;
		std::tstring m_briefInfo;
		HICON m_hIcon;
	};

	const CWindowInfo* FindInfo( HWND hWnd ) const;

	static std::tstring FormatContext( HWND hWnd, const TCHAR sectionName[] );

	static bool MustCacheIcon( HWND hWnd );
	static HICON QueryWndIcon( HWND hWnd );
	static HICON GetIcon( HWND hWnd );
private:
	std::unordered_map<HWND, CWindowInfo> m_slowCache;			// caches icons for windows that react slowly to WM_GETICON, e.g. some elevated processes running in Windows 10

	static double s_timeoutSecs;			// threshold for slow windows (0.5 seconds)
};


#endif // WindowInfoStore_h
