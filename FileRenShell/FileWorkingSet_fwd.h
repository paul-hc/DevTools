#ifndef FileWorkingSet_fwd_h
#define FileWorkingSet_fwd_h
#pragma once

#include "utl/ISubject.h"
#include "utl/Path.h"
#include "utl/FileState.h"


class CEnumTags;
class CFileWorkingSet;

typedef std::pair< const fs::CPath, fs::CPath > TPathPair;
typedef std::pair< const fs::CFileState, fs::CFileState > TFileStatePair;


namespace fmt
{
	enum PathFormat;
}


namespace app
{
	enum DateTimeField { ModifiedDate, CreatedDate, AccessedDate, _DateTimeFieldCount };

	const CEnumTags& GetTags_DateTimeField( void );

	const CTime& GetTimeField( const fs::CFileState& fileState, DateTimeField field );
	inline CTime& RefTimeField( fs::CFileState& rFileState, DateTimeField field ) { return const_cast< CTime& >( GetTimeField( rFileState, field ) ); }


	enum CustomColors
	{
		ColorDeletedText = color::Red,
		ColorModifiedText = color::Blue,
		ColorErrorBk = color::PastelPink
	};
}


#endif // FileWorkingSet_fwd_h
