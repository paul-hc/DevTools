
#include "stdafx.h"
#include "TouchItem.h"
#include "utl/FmtUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CTouchItem::CTouchItem( TFileStatePair* pStatePair, fmt::PathFormat pathFormat )
	: m_pStatePair( safe_ptr( pStatePair ) )
	, m_displayPath( fmt::FormatPath( pStatePair->first.m_fullPath, pathFormat ) )
{
}

CTouchItem::~CTouchItem()
{
}

std::tstring CTouchItem::GetCode( void ) const
{
	return m_pStatePair->first.m_fullPath.Get();
}

std::tstring CTouchItem::GetDisplayCode( void ) const
{
	return m_displayPath;
}

bool CTouchItem::IsModified( void ) const
{
	return m_pStatePair->second.IsValid() && m_pStatePair->second != m_pStatePair->first;
}
