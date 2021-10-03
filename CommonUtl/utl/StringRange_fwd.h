#ifndef StringRange_fwd_h
#define StringRange_fwd_h
#pragma once


namespace str
{
	namespace range
	{
		template< typename CharT >
		class CStringRange;
	}

	typedef str::range::CStringRange<TCHAR> TStringRange;
}


#endif // StringRange_fwd_h
