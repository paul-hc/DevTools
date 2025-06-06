#ifndef Table_h
#define Table_h
#pragma once

#include "utl/Encoding.h"


// composite text cell structured as a hierarchy of folders and leafs, to describe cell nodes in a table

class CTextCell
{
public:
	CTextCell( const CTextCell* pParent, const std::tstring& name );
	~CTextCell();

	bool IsRoot( void ) const { return nullptr == m_pParent; }
	bool IsFolder( void ) const { return !IsLeaf(); }
	bool IsLeaf( void ) const { return m_children.empty(); }

	const CTextCell* GetParent( void ) const { return m_pParent; }
	CTextCell* AddChild( const std::tstring& name );

	const std::tstring& GetName( void ) const { return m_name; }
	CTextCell* FindCell( const std::tstring& name ) const;

	std::tstring MakePath( const CTextCell* pRoot = nullptr ) const;
	CTextCell* DeepFindCell( const TCHAR* pCellPath ) const;		// path names are separated by s_columnSep ('\t')

	const std::vector< CTextCell* >& GetChildren( void ) const { return m_children; }
	void QuerySubFolders( std::vector< CTextCell* >& rSubFolders ) const;
	void QueryLeafs( std::vector< CTextCell* >& rLeafs ) const;
private:
	std::tstring m_name;
	const CTextCell* m_pParent;
	std::vector< CTextCell* > m_children;
public:
	static TCHAR s_columnSep[];
};


// uses an input stream of rows of tab-separated values to build a hierarchy of folders and leafs cells

class CTable
{
public:
	CTable( void );
	~CTable();

	const CTextCell* GetRoot( void ) const { return &m_root; }

	fs::Encoding ParseTextFile( const fs::CPath& textFilePath, bool sortRows ) throws_( CRuntimeException );

	void ParseRows( std::vector< std::tstring >& rRows, bool sortRows );
	void ParseColumns( const std::tstring& row );
private:
	CTextCell m_root;		// table root
};


#endif // Table_h
