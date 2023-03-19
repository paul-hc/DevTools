
#include "pch.h"
#include "IncludeTag.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


void CIncludeTag::Clear( void )
{
	m_includeTag.clear();
	m_directiveFormat.clear();
	m_filePath.Clear();
	m_fileType = ft::Unknown;
	m_localInclude = true;
	m_evenTag = false;
}

void CIncludeTag::Setup( const TCHAR* pIncludeTag )
{
	Clear();
	m_includeTag = pIncludeTag;

	size_t startPos = m_includeTag.find_first_of( _T("<\""), 0 );
	if ( startPos != std::tstring::npos )
	{
		m_localInclude = _T('\"') == m_includeTag[ startPos ];
		size_t endPos = m_includeTag.find( m_localInclude ? _T('\"') : _T('>'), startPos + 1 );
		if ( endPos != std::tstring::npos )
		{
			m_evenTag = true;
			m_includeTag = m_includeTag.substr( startPos, endPos - startPos + 1 );
			m_filePath = m_includeTag.substr( 1, m_includeTag.length() - 2 );
		}
	}
}

void CIncludeTag::Setup( const std::tstring& filePath, bool localInclude )
{
	Clear();

	m_filePath.Set( filePath );
	m_localInclude = localInclude;
	if ( !m_filePath.IsEmpty() )
		m_includeTag = str::Format( m_localInclude ? _T("\"%s\"") : _T("<%s>"), m_filePath.GetPtr() );
}

bool CIncludeTag::IsEnclosed( void ) const
{
	switch ( *m_includeTag.c_str() )
	{
		case _T('<'):
		case _T('\"'):
			return true;
	}
	return false;
}

// normalize a include tag string to its minimal standard form
std::tstring CIncludeTag::GetStrippedTag( const TCHAR* pIncludeTag )
{
	CIncludeTag tag( pIncludeTag ), strippedTag;
	if ( !tag.IsEmpty() )
		return CIncludeTag( tag.m_filePath.Get(), tag.m_localInclude ).GetTag();

	return std::tstring();
}

void CIncludeTag::SetIncludeDirectiveFormat( const TCHAR* pDirective, bool isCppPreprocessor )
{
	m_directiveFormat.clear();

	if ( isCppPreprocessor )
		m_directiveFormat += _T('#');

	m_directiveFormat += pDirective;

	if ( str::Equals<str::Case>( _T("importlib"), pDirective ) )
		m_directiveFormat += _T("(%s)");
	else
		m_directiveFormat += _T(" %s");
}

std::tstring CIncludeTag::GetIncludeDirectiveFormat( void ) const
{
	std::tstring idFormat;

	if ( !m_directiveFormat.empty() )
		idFormat = m_directiveFormat;
	else
		switch ( m_fileType )
		{
			case ft::IDL:	idFormat = _T("import %s"); break;
			case ft::TLB:	idFormat = _T("importlib(%s)"); break;
			default:		idFormat = _T("#include %s"); break;
		}

	if ( -1 == idFormat.find( _T('%') ) )
		idFormat += _T(" %s");

	return idFormat;
}

std::tstring CIncludeTag::FormatIncludeStatement( void ) const
{
	return str::Format( GetIncludeDirectiveFormat().c_str(), m_includeTag.c_str() );		// statement + tag
}
