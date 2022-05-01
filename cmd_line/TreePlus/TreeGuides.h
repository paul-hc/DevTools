#ifndef TreeGuides_h
#define TreeGuides_h
#pragma once

#include "CmdLineOptions_fwd.h"


struct CPagePos
{
	CPagePos( size_t pos, size_t count ) : m_pos( pos ), m_count( count ) {}

	template< typename VectorT >
	CPagePos( const VectorT& items ) : m_pos( 0 ), m_count( items.size() ) {}

	bool IsEmpty( void ) const { return 0 == m_count; }
	bool IsLast( void ) const { return !IsEmpty() && m_pos == m_count - 1; }

	bool AtEnd( void ) const { return IsEmpty() || m_pos == m_count; }

	CPagePos& operator++( void )
	{
		ASSERT( !AtEnd() );
		++m_pos;
		return *this;
	}
public:
	size_t m_pos;
	const size_t m_count;
};


struct CGuidesProfile
{
public:
	wchar_t m_vertBar;
	wchar_t m_subDirPlus;
	wchar_t m_subDirLastSlash;
	wchar_t m_subDirMinusFill;

	static const CGuidesProfile s_outProfiles[ _ProfileCount ];
};


class CTreeGuides
{
public:
	CTreeGuides( GuidesProfileType profileType, size_t indentSize );

	const std::wstring& GetFilePrefix( bool hasMoreSubDirs ) const { return hasMoreSubDirs ? m_fileIndent : m_fileIndentLast; }

	const std::wstring& GetSubDirPrefix( const CPagePos& subDirPos ) const { return subDirPos.IsLast() ? m_dirEntryLast : m_dirEntry; }
	const std::wstring& GetSubDirRecursePrefix( const CPagePos& subDirPos ) const { return subDirPos.IsLast() ? m_fileIndentLast : m_fileIndent; }

	// table-specific guides (using flat root guides)
	const std::wstring& GetTable_SubDirPrefix( size_t depth, const CPagePos& subDirPos ) const { return depth != 0 ? GetSubDirPrefix( subDirPos ) : s_tableRootLead; }
	const std::wstring& GetTable_SubDirRecursePrefix( size_t depth, const CPagePos& subDirPos ) const { return depth != 0 ? GetSubDirRecursePrefix( subDirPos ) : s_tableRootLead; }
private:
	std::wstring MakePart( size_t indentSize, wchar_t leadCh, wchar_t padCh ) const;
private:
	const CGuidesProfile& m_profile;

	std::wstring m_dirEntry;			// "+---"
	std::wstring m_dirEntryLast;		// "\---"
	std::wstring m_fileIndent;			// "|   "
	std::wstring m_fileIndentLast;		// "    "

	static const std::wstring s_tableRootLead;		// ""
};


#endif // TreeGuides_h
