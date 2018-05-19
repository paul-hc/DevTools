
#include "stdafx.h"
#include "TouchItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CTouchItem::CTouchItem( TFileStatePair* pStatePair, bool showFullPath )
	: m_pStatePair( safe_ptr( pStatePair ) )
	, m_displayPath( showFullPath ? pStatePair->first.m_fullPath.GetPtr() : pStatePair->first.m_fullPath.GetNameExt() )
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
	if ( m_pStatePair->second.IsValid() )
		return m_pStatePair->second != m_pStatePair->first;

	return false;
}
