#ifndef Utilities_h
#define Utilities_h
#pragma once


namespace str
{
	template< typename Iterator, typename CharType >
	std::basic_string< CharType > Join( Iterator itFirstToken, Iterator itLastToken, const CharType* pSep )
	{	// works with any forward/reverse iterator
		std::basic_ostringstream< CharType > oss;
		for ( Iterator itItem = itFirstToken; itItem != itLastToken; ++itItem )
		{
			if ( itItem != itFirstToken )
				oss << pSep;
			oss << *itItem;
		}
		return oss.str();
	}

	// works with container of any value type that has stream insertor defined
	//
	template< typename ContainerType, typename CharType >
	inline std::basic_string< CharType > Join( const ContainerType& items, const CharType* pSep )
	{
		return Join( items.begin(), items.end(), pSep );
	}
}


namespace ui
{
	bool EnsureVisibleRect( CRect& rDest, const CRect& anchor, bool horiz = true, bool vert = true );

	// multiple monitors
	enum MonitorArea { Monitor, Workspace };	// Workspace means Desktop or work area

	CRect FindMonitorRect( HWND hWnd, MonitorArea area );
	CRect FindMonitorRectAt( const POINT& screenPoint, MonitorArea area );
	CRect FindMonitorRectAt( const RECT& screenRect, MonitorArea area );

	inline bool EnsureVisibleDesktopRect( CRect& rScreenRect, MonitorArea area = Workspace )
	{	// pull rScreenRect to the monitor desktop with most area
		return ui::EnsureVisibleRect( rScreenRect, FindMonitorRectAt( rScreenRect, area ) );
	}


	void SetRadio( CCmdUI* pCmdUI, BOOL checked );
}


#endif // Utilities_h
