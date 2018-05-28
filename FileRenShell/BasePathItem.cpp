
#include "stdafx.h"
#include "BasePathItem.h"
#include "utl/FmtUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CBasePathItem::CBasePathItem( const fs::CPath& keyPath, fmt::PathFormat fmtDisplayPath )
	: m_keyPath( keyPath )
	, m_displayPath( fmt::FormatPath( m_keyPath, fmtDisplayPath ) )
{
}

CBasePathItem::~CBasePathItem()
{
}

const std::tstring& CBasePathItem::GetCode( void ) const
{
	return m_keyPath.Get();
}

std::tstring CBasePathItem::GetDisplayCode( void ) const
{
	return m_displayPath;
}
