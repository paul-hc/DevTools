
#include "pch.h"
#include "PathItemBase.h"
#include "SerializeStdTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CPathItemBase implementation

CPathItemBase::~CPathItemBase()
{
}

void CPathItemBase::ResetFilePath( const fs::CPath& filePath )
{
	m_filePath = filePath;
	m_displayPath = m_filePath.GetFilenamePtr();
}

void CPathItemBase::SetFilePath( const fs::CPath& filePath )
{
	ResetFilePath( filePath );
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

void CPathItemBase::Stream( CArchive& archive )
{
	if ( archive.IsStoring() )
		archive << m_filePath;
	else
	{
		fs::CPath filePath;
		archive >> filePath;
		ResetFilePath( filePath );
	}
}
