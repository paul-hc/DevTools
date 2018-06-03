
#include "stdafx.h"
#include "BasePathItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CBasePathItem::CBasePathItem( const fs::CPath& keyPath )
	: m_keyPath( keyPath )
	, m_displayPath( m_keyPath.GetNameExt() )
{
}

CBasePathItem::~CBasePathItem()
{
}

void CBasePathItem::StripDisplayCode( const fs::CPath& commonParentPath )
{
	m_displayPath = m_keyPath.Get();
	path::StripPrefix( m_displayPath, commonParentPath.GetPtr() );
}

const std::tstring& CBasePathItem::GetCode( void ) const
{
	return m_keyPath.Get();
}

std::tstring CBasePathItem::GetDisplayCode( void ) const
{
	return m_displayPath;
}
