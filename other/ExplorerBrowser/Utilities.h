#ifndef Utilities_h
#define Utilities_h
#pragma once


template< typename FieldType >
inline bool HasFlag( FieldType field, unsigned int flag )
{
	return ( field & flag ) != 0;
}

template< typename FieldType >
inline bool EqFlag( FieldType field, unsigned int flag )
{
	return flag == ( field & flag );
}

template< typename FieldType >
inline bool EqMaskedValue( FieldType field, unsigned int mask, unsigned int value )
{
	return value == ( field & mask );
}

template< typename FieldType >
inline void ClearFlag( FieldType& rField, unsigned int flag )
{
	rField &= ~flag;
}

template< typename FieldType >
inline void SetFlag( FieldType& rField, unsigned int flag, bool on = true )
{
	if ( on )
		rField |= flag;
	else
		rField &= ~flag;
}

template< typename FieldType >
inline void ToggleFlag( FieldType& rField, unsigned int flag )
{
	rField ^= flag;
}

template< typename FieldType >
inline bool ModifyFlag( FieldType& rField, unsigned int clearFlags, unsigned int setFlags )
{
	FieldType oldField = rField;
	rField &= ~clearFlags;
	rField |= setFlags;
	return rField != oldField;
}

template< typename FieldType >
inline bool CopyFlags( FieldType& rField, unsigned int mask, unsigned int flags )
{
	return ModifyFlag( rField, mask, flags & mask );
}


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


namespace fs
{
	bool IsValidFile( const TCHAR* pFilePath );
	bool IsValidDirectory( const TCHAR* pDirPath );
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
