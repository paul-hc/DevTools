
#include "stdafx.h"
#include "PathAlgorithms.h"
#include "utl/EnumTags.h"
#include "utl/StringUtilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const CEnumTags& GetTags_ChangeCase( void )
{
	static const CEnumTags& tags( str::Load( IDS_CHANGE_CASE_TAGS ) );
	return tags;
}
