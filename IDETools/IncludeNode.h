#ifndef IncludeNode_h
#define IncludeNode_h
#pragma once

#include "utl/Path.h"
#include "FileType.h"
#include "IncludeTag.h"


namespace loc { enum IncludeLocation; }


struct CIncludeNode
{
	CIncludeNode( const CIncludeTag& includeTag, const fs::CPath& path, loc::IncludeLocation locationType, unsigned int lineNo )
		: m_path( path )
		, m_fileType( ft::FindFileType( m_path.GetPtr() ) )
		, m_locationType( locationType )
		, m_includeTag( includeTag )
		, m_lineNo( lineNo )
		, m_orderIndex( -1 )
		, m_parsedChildCount( -1 )
	{
	}

	bool IsValidFile( void ) const { return !m_path.IsEmpty() && m_path.FileExist( fs::Read ); }

	bool IsParsed( void ) const { return m_parsedChildCount != -1; }		// content already parsed for included files
	int GetParsedChildrenCount( void ) const { return m_parsedChildCount; }
	void SetParsedChildrenCount( unsigned int parsedChildCount ) { m_parsedChildCount = parsedChildCount; }

	int GetOrderIndex( void ) const { return m_orderIndex; }
	void SetOrderIndex( int orderIndex ) { m_orderIndex = orderIndex; }
public:
	const fs::CPath m_path;
	const ft::FileType m_fileType;
	const loc::IncludeLocation m_locationType;
	const CIncludeTag m_includeTag;
	const unsigned int m_lineNo;			// parent source file: line where the #include statement is
private:
	int m_parsedChildCount;					// count of child included files (after parsing)
	int m_orderIndex;
};


namespace pred
{
	struct EqualTreeItemPath
	{
		EqualTreeItemPath( const CIncludeNode* pTreeItem ) : m_pTreeItem( pTreeItem ) {}

		bool operator()( const CIncludeNode* pTreeItem ) const { return m_pTreeItem->m_path.Equivalent( pTreeItem->m_path ); }
	private:
		const CIncludeNode* m_pTreeItem;
	};
}


#endif // IncludeNode_h
