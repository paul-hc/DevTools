#ifndef TextListEditor_h
#define TextListEditor_h
#pragma once

#include "ObjectCtrlBase.h"
#include "TextEditor.h"
#include "utl/ISubject.h"
#include <iterator>			// for std::back_inserter
#include <unordered_map>


// a multi-line edit with ISubject-based item interface
//
class CTextListEditor : public CTextEditor
	, public CObjectCtrlBase
{
	// hidden base methods:
	using CTextEditor::SetText;
	using CTextEditor::DDX_Text;
	using CTextEditor::DDX_UiEscapeSeqs;
	using CTextEditor::SetWindowText;
public:
	CTextListEditor( bool useFixedFont = true );
	virtual ~CTextListEditor();

	// lock item count: prevent adding or deleting item lines
	bool UseLockedItemCount( void ) const { return m_useLockedItemCount; }
	void SetUseLockedItemCount( bool useLockedItemCount = true ) { m_useLockedItemCount = useLockedItemCount; }

	size_t GetItemCount( void ) const override { return m_objects.size(); }

	utl::ISubject* GetAt( size_t index ) const { return index < GetItemCount() ? m_objects[index] : nullptr; }
	template< typename ObjectT > ObjectT* GetItemAt( size_t index ) const { return checked_static_cast<ObjectT*>( GetAt( index ) ); }

	size_t FindItemPos( const utl::ISubject* pObject ) const;
	template< typename ObjectT > std::vector<size_t> QueryItemIndexes( const std::vector<ObjectT*>& objects ) const;

	// caret item
	template< typename ObjectT > ObjectT* GetCaretItem( void ) const { return GetItemAt<ObjectT>( GetCaretLineIndex() ); }
	bool SetCaretItem( const utl::ISubject* pCaretObject, bool selectLine = true );

	// selection
	template< typename ObjectT > void QuerySelItems( OUT std::vector<ObjectT*>& rSelObjects ) const;
	template< typename ObjectT > bool SetSelItems( const std::vector<ObjectT*>& selObjects );

	// top visible item
	template< typename ObjectT > ObjectT* GetTopItem( void ) const { return GetItemAt<ObjectT>( GetTopLineIndex() ); }
	bool SetTopItem( const utl::ISubject* pTopObject );

	template< typename StringT, typename ObjectT >
	void QueryItemLines( OUT std::vector<StringT>& rItemLines, const std::vector<ObjectT*>& objects ) const;
public:
	void Clear( void ) { m_objects.clear(); UpdateItemsText(); }

	template< typename ContainerT >
	bool StoreItems( const ContainerT& objects )
	{
		m_objects.assign( objects.begin(), objects.end() );
		StoreItemPositions();
		return UpdateItemsText();
	}

	template< typename ContainerT >
	bool UpdateItems( const ContainerT& objects ) { return UpdateItemIndexesText( QueryItemIndexes( objects ) ); }
protected:
	bool UpdateItemsText( void );		// returns true if text changed
	bool UpdateItemIndexesText( const std::vector<size_t>& objectIndexes );		// targeted update, only for a item subset; returns true if text changed

	TLineIndex FindItemLineIndex( const utl::ISubject* pObject ) const { return static_cast<TLineIndex>( FindItemPos( pObject ) ); }
private:
	void StoreItemPositions( void );
private:
	bool m_useLockedItemCount;
	std::vector<utl::ISubject*> m_objects;
	std::unordered_map<utl::ISubject*, size_t> m_objectToPos;

protected:
	// base overrides
	virtual bool ValidateText( ui::CTextValidator& rValidator ) override;
};


namespace utl
{
	template< typename ContainerT >
	bool IsOrdered( const ContainerT& values );		// FWD
}


// CTextListEditor template code

template< typename ObjectT >
std::vector<size_t> CTextListEditor::QueryItemIndexes( const std::vector<ObjectT*>& objects ) const
{
	std::vector<size_t> indexes;
#ifdef IS_CPP_11
	std::transform( objects.begin(), objects.end(), std::back_inserter( indexes ), [this]( const ObjectT* pObject ) { return FindItemPos( pObject ); } );
#else
	for ( typename std::vector<ObjectT*>::const_iterator itObject = objects.begin(); itObject != objects.end(); ++itObject )
		indexes.push_back( FindItemPos( *itObject ) );
#endif
	return indexes;
}

template< typename ObjectT >
void CTextListEditor::QuerySelItems( OUT std::vector<ObjectT*>& rSelObjects ) const
{
	rSelObjects.clear();

	if ( !m_objects.empty() )
	{
		Range<TLineIndex> selLineRange = GetSelLineRange();

		std::transform( m_objects.begin() + selLineRange.m_start, m_objects.begin() + selLineRange.m_end + 1, std::back_inserter( rSelObjects ), func::As<ObjectT>() );
	}
}

template< typename ObjectT >
bool CTextListEditor::SetSelItems( const std::vector<ObjectT*>& selObjects )
{
	ASSERT( utl::IsOrdered( QueryItemIndexes( selObjects ) ) );

	if ( selObjects.empty() )
		return ClearSelection();		// collapse the selection

	Range<TLineIndex> selLineRange( FindItemLineIndex( selObjects.front() ) );

	if ( selObjects.size() > 1 )
		selLineRange.m_end = FindItemLineIndex( selObjects.back() );

	ENSURE( selLineRange.IsNormalized() );			// since selObjects is ordered
	return SelectLineRange( selLineRange ).second;	// true if changed
}

template< typename StringT, typename ObjectT >
void CTextListEditor::QueryItemLines( OUT std::vector<StringT>& rItemLines, const std::vector<ObjectT*>& objects ) const
{
	rItemLines.clear();
	rItemLines.reserve( objects.size() );

	for ( typename std::vector<ObjectT*>::const_iterator itObject = objects.begin(); itObject != objects.end(); ++itObject )
	{
		TLineIndex lineIndex = FindItemLineIndex( *itObject );

		if ( lineIndex != -1 )
			rItemLines.push_back( GetLineText( lineIndex ) );
	}
}


#endif // TextListEditor_h
