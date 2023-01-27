#ifndef Algorithms_fwd_h
#define Algorithms_fwd_h
#pragma once


// forward declarations - required for C++ 14+ compilation, which is stricter!

namespace pred
{
	template< typename ToKeyFunc >
	struct LessKey;

	template< typename ToKeyFunc >
	LessKey<ToKeyFunc> MakeLessKey( ToKeyFunc toKeyFunc );
}

#endif // Algorithms_fwd_h
