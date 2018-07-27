#ifndef Utilities_h
#define Utilities_h
#pragma once


#define ASSERT_PTR( p ) ASSERT( (p) != NULL )


#ifdef _DEBUG
	#define HR_AUDIT( expr ) utl::Audit( (expr), (#expr) )
	#define HR_OK( expr ) utl::Check( (expr), (#expr) )
#else
	#define HR_AUDIT( expr ) utl::Audit( (expr), NULL )
	#define HR_OK( expr ) utl::Check( (expr), NULL )
#endif


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
	template< typename CharType > bool IsEmpty( const CharType* pText ) { return NULL == pText || 0 == *pText; }

	inline size_t GetLength( const char* pText ) { return pText != NULL ? strlen( pText ) : 0; }
	inline size_t GetLength( const wchar_t* pText ) { return pText != NULL ? wcslen( pText ) : 0; }

	inline const char* begin( const char* pText ) { return pText; }
	inline const wchar_t* begin( const wchar_t* pText ) { return pText; }

	inline const char* end( const char* pText ) { return pText + GetLength( pText ); }
	inline const wchar_t* end( const wchar_t* pText ) { return pText + GetLength( pText ); }


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


	template< typename CharType, typename StringType >
	void SplitAdd( std::vector< StringType >& rItems, const CharType* pSource, const CharType* pSep )
	{
		ASSERT( !str::IsEmpty( pSep ) );

		if ( !str::IsEmpty( pSource ) )
		{
			const size_t sepLen = str::GetLength( pSep );
			typedef const CharType* const_iterator;

			for ( const_iterator itItemStart = str::begin( pSource ), itEnd = str::end( pSource ); ; )
			{
				const_iterator itItemEnd = std::search( itItemStart, itEnd, pSep, pSep + sepLen );
				if ( itItemEnd != itEnd )
				{
					rItems.push_back( std::basic_string< CharType >( itItemStart, std::distance( itItemStart, itItemEnd ) ) );
					itItemStart = itItemEnd + sepLen;
				}
				else
				{
					rItems.push_back( std::basic_string< CharType >( itItemStart ) );			// last item
					break;
				}
			}
		}
	}

	template< typename CharType, typename StringType >
	inline void Split( std::vector< StringType >& rItems, const CharType* pSource, const CharType* pSep )
	{
		rItems.clear();
		SplitAdd( rItems, pSource, pSep );
	}
}


namespace pred
{
	enum CompareResult { Less = -1, Equal, Greater };


	template< typename DiffType >
	inline CompareResult ToCompareResult( DiffType difference )
	{
		if ( difference < 0 )
			return Less;
		else if ( difference > 0 )
			return Greater;
		else
			return Equal;
	}
}


namespace utl
{
	HRESULT Audit( HRESULT hResult, const char* pFuncName );
	inline bool Check( HRESULT hResult, const char* pFuncName ) { return SUCCEEDED( Audit( hResult, pFuncName ) ); }
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
