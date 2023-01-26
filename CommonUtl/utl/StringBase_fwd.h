#ifndef StringBase_fwd_h
#define StringBase_fwd_h
#pragma once

#include <iosfwd>


// forward declarations for stream extractors of common UTL types - required for C++ 11 compilation, which is stricter!

#if _MSC_VER <= 1700		// vc11 (VS-2012)
	// introducing char32_t just for illustration - no support yet implemented in STL or Windows
	typedef unsigned long char32_t;				// UTF32, e.g. U'a'
#else
	// C++ 11: char32_t is a built-in type
#endif

// defined in utl/TextEncoding.h

namespace str
{
	typedef std::basic_string<char32_t> wstring4;
}

//std::ostream& operator<<( std::ostream& os, const char32_t* pText32 );
//std::wostream& operator<<( std::wostream& os, const char32_t* pText32 );

std::ostream& operator<<( std::ostream& os, const str::wstring4& text32 );
std::wostream& operator<<( std::wostream& os, const str::wstring4& text32 );


// defined in utl/TimeUtils.h
std::ostream& operator<<( std::ostream& oss, const CTime& dt );
std::wostream& operator<<( std::wostream& oss, const CTime& dt );


namespace fs
{
	class CPath;
	struct CFileState;
}


// defined in utl/Path.h
std::ostream& operator<<( std::ostream& os, const fs::CPath& path );
std::wostream& operator<<( std::wostream& os, const fs::CPath& path );

// defined in utl/FileState.h
std::ostream& operator<<( std::ostream& os, const fs::CFileState& fileState );
std::wostream& operator<<( std::wostream& os, const fs::CFileState& fileState );


// defined in utl/test/UnitTest.h
template< typename Type1, typename Type2 >
std::wostream& operator<<( std::wostream& os, const std::pair<Type1, Type2>& rPair );


#endif // StringBase_fwd_h
