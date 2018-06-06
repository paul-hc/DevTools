
#include "stdafx.h"
#include "PathItemBase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CPathItemBase::CPathItemBase( const fs::CPath& keyPath )
	: m_keyPath( keyPath )
	, m_displayPath( m_keyPath.GetNameExt() )
{
}

CPathItemBase::~CPathItemBase()
{
}

void CPathItemBase::StripDisplayCode( const fs::CPath& commonParentPath )
{
	m_displayPath = m_keyPath.Get();
	path::StripPrefix( m_displayPath, commonParentPath.GetPtr() );
}

const std::tstring& CPathItemBase::GetCode( void ) const
{
	return m_keyPath.Get();
}

std::tstring CPathItemBase::GetDisplayCode( void ) const
{
	return m_displayPath;
}
