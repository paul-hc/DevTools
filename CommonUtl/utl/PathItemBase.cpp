
#include "stdafx.h"
#include "PathItemBase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CPathItemBase::~CPathItemBase()
{
}

void CPathItemBase::ResetFilePath( const fs::CPath& filePath )
{
	m_filePath = filePath;
	m_displayPath = m_filePath.GetNameExt();
}

void CPathItemBase::StripDisplayCode( const fs::CPath& commonParentPath )
{
	m_displayPath = m_filePath.Get();
	path::StripPrefix( m_displayPath, commonParentPath.GetPtr() );
}

const std::tstring& CPathItemBase::GetCode( void ) const
{
	return m_filePath.Get();
}

std::tstring CPathItemBase::GetDisplayCode( void ) const
{
	return m_displayPath;
}
