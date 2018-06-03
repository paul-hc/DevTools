
#include "stdafx.h"
#include "RenameItem.h"
#include "utl/FmtUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CRenameItem::CRenameItem( const fs::CPath& srcPath )
	: CBasePathItem( srcPath )
{
}

CRenameItem::~CRenameItem()
{
}
