#ifndef utl_SerializeStdTypes_h
#define utl_SerializeStdTypes_h
#pragma once

#include "Path.h"
#include "Serialization.h"


// custom types archive insertors/extractors

inline CArchive& operator<<( CArchive& archive, const fs::CPath& path )
{
	// as std::tstring
	return archive << path.Get();
}

inline CArchive& operator>>( CArchive& archive, fs::CPath& rPath )
{
	// as std::tstring
	std::tstring filePath;
	archive >> filePath;
	rPath.Set( filePath );
	return archive;
}


template< typename Type >
inline CArchive& operator<<( CArchive& archive, const Range< Type >& range )
{
	return archive << range.m_start << range.m_end;
}

template< typename Type >
inline CArchive& operator>>( CArchive& archive, Range< Type >& rRange )
{
	return archive >> rRange.m_start >> rRange.m_end;
}


// custom types archive streamers

template< typename Type >
inline CArchive& operator&( CArchive& archive, Type& rValue )
{
	if ( archive.IsStoring() )
		return archive << rValue;
	else
		return archive >> rValue;
}

inline CArchive& operator&( CArchive& archive, bool& rBoolean )
{
	// backwards compatibility: serialize bool as BOOL
	if ( archive.IsStoring() )
		archive << static_cast< BOOL >( rBoolean );
	else
	{
		BOOL boolean;
		archive >> boolean;
		rBoolean = boolean != FALSE;
	}
	return archive;
}

inline CArchive& operator&( CArchive& archive, FILETIME& rFileTime )
{
	// avoid conflict with operator<<( CArchive&, COleDateTime )
	if ( archive.IsStoring() )
		return archive << rFileTime.dwLowDateTime << rFileTime.dwHighDateTime;
	else
		return archive >> rFileTime.dwLowDateTime >> rFileTime.dwHighDateTime;
}


#endif // utl_SerializeStdTypes_h
