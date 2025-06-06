
#include "pch.h"
#include "TreeGuides.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CGuidesProfile implementation

const CGuidesProfile CGuidesProfile::s_outProfiles[ _ProfileCount ] =
{
	{ _T('│'), _T('├'), _T('└'), _T('─') },			// GraphGuides
	{ _T('|'), _T('+'), _T('\\'), _T('-') },		// AsciiGuides
	{ _T(' '), _T(' '), _T(' '), _T(' ') },			// BlankGuides
	{ _T('\t'), _T('\t'), _T('\t'), _T('\0') }		// TabGuides
};


// CTreeGuides implementation

const std::wstring CTreeGuides::s_tableRootLead;

CTreeGuides::CTreeGuides( GuidesProfileType profileType, size_t indentSize )
	: m_profile( CGuidesProfile::s_outProfiles[ profileType ] )
	, m_dirEntry( MakePart( indentSize, m_profile.m_subDirPlus, m_profile.m_subDirMinusFill ) )				// "+---"
	, m_dirEntryLast( MakePart( indentSize, m_profile.m_subDirLastSlash, m_profile.m_subDirMinusFill ) )		// "\---"
	, m_fileIndent( MakePart( indentSize, m_profile.m_vertBar, ' ' ) )											// "|   "
	, m_fileIndentLast( MakePart( indentSize, ' ', ' ' ) )														// "    "
{
	if ( TabGuides == profileType )
		m_dirEntry = m_dirEntryLast = m_fileIndent = m_fileIndentLast = MakePart( 1, m_profile.m_vertBar, _T('\0') );		// use a fixed indent of 1 tab
}

std::wstring CTreeGuides::MakePart( size_t indentSize, wchar_t leadCh, wchar_t padCh ) const
{
	std::tstring part( indentSize, padCh );
	part[ 0 ] = leadCh;
	return part;
}
