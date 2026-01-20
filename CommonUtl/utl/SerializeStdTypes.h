#ifndef utl_SerializeStdTypes_h
#define utl_SerializeStdTypes_h
#pragma once

#include "Serialization.h"
#include "Range.h"
#include "Path.h"


// generic archive streamer

template< typename Type >
inline CArchive& operator&( CArchive& archive, Type& rValue )
{
	if ( archive.IsStoring() )
		return archive << rValue;
	else
		return archive >> rValue;
}


// custom types archive insertors/extractors

inline CArchive& operator<<( CArchive& archive, const fs::CPath& path )
{
	return archive << path.Get();
}

inline CArchive& operator>>( CArchive& archive, OUT fs::CPath& rPath )
{
	std::tstring filePath;
	archive >> filePath;
	rPath.Set( filePath );
	return archive;
}


template< typename Type >
inline CArchive& operator<<( CArchive& archive, const Range<Type>& range )
{
	return archive << range.m_start << range.m_end;
}

template< typename Type >
inline CArchive& operator>>( CArchive& archive, OUT Range<Type>& rRange )
{
	return archive >> rRange.m_start >> rRange.m_end;
}


// custom types archive streamers

inline CArchive& operator&( CArchive& archive, IN OUT bool& rBoolean )
{
	// backwards compatibility: serialize bool as BOOL
	if ( archive.IsStoring() )
		archive << static_cast<BOOL>( rBoolean );
	else
	{
		BOOL boolean;
		archive >> boolean;
		rBoolean = boolean != FALSE;
	}
	return archive;
}

inline CArchive& operator&( CArchive& archive, IN OUT FILETIME& rFileTime )
{
	// avoid conflict with operator<<( CArchive&, COleDateTime )
	if ( archive.IsStoring() )
		return archive << rFileTime.dwLowDateTime << rFileTime.dwHighDateTime;
	else
		return archive >> rFileTime.dwLowDateTime >> rFileTime.dwHighDateTime;
}

inline CArchive& operator&( CArchive& archive, IN OUT std::wstring* pWideStr )
{
	if ( archive.IsStoring() )
		archive << pWideStr;
	else
		archive >> pWideStr;

	return archive;
}


#endif // utl_SerializeStdTypes_h
