#ifndef CommonWinDefs_h
#define CommonWinDefs_h
#pragma once


// include this in stdafx.h, just before first Windows header: #include <afxwin.h>


#ifdef NOMINMAX

// required by some Windows headers: return by value to make it compatible with uses such as: max< int, unsigned long >( a, b )

template< typename A, typename B > A max( const A& a, const B& b ) { return ( a < static_cast< A >( b ) ) ? static_cast< A >( b ) : a; }
template< typename A, typename B > A min( const A& a, const B& b ) { return ( static_cast< A >( b ) < a ) ? static_cast< A >( b ) : a; }

#endif //NOMINMAX


// new theme parts & states (missing in VC9 tmschema.h)
namespace vt
{
	#include "utl/vsstyle.h"


	// missing part & state definitions

	enum GLOBALS_PARTS { GP_BORDER = 1, GP_LINEHORZ, GP_LINEVERT };

	enum BORDER_STATES { BSS_FLAT = 1, BSS_RAISED, BSS_SUNKEN };
	enum LINEHORZ_STATES { LHS_FLAT = 1, LHS_RAISED, LHS_SUNKEN };
	enum LINEVERT_STATES { LVS_FLAT = 1, LVS_RAISED, LVS_SUNKEN };

	enum SEARCHEDITBOX_PARTS { SEBP_SEARCHEDITBOXTEXT = 1 };
	enum SEARCHEDITBOXTEXT_STATES { SEBTS_FORMATTED = 1 };
}


#endif // CommonWinDefs_h
