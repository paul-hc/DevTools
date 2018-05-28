
#include "stdafx.h"
#include "RenameItem.h"
#include "utl/FmtUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CRenameItem::CRenameItem( const TPathPair* pPathPair )
	: CBasePathItem( pPathPair->first, fmt::FilenameExt )
	, m_pPathPair( safe_ptr( pPathPair ) )
{
}

CRenameItem::~CRenameItem()
{
}
