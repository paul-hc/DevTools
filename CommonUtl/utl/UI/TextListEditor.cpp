
#include "pch.h"
#include "TextListEditor.h"
#include "utl/Algorithms.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CTextListEditor::CTextListEditor( bool useFixedFont /*= true*/ )
	: CTextEditor( useFixedFont )
	, CObjectCtrlBase( this )
	, m_useLockedItemCount( false )
{
	SetKeepSelOnFocus();
	SetUsePasteTransact();		// get just one EN_CHANGE notification after paste
	SetShowFocus();
	SetUseLockedItemCount();
}

CTextListEditor::~CTextListEditor()
{
}

void CTextListEditor::StoreItemPositions( void )
{
	size_t pos = 0;

	m_objectToPos.clear();
#ifdef IS_CPP_11
	std::for_each( m_objects.begin(), m_objects.end(), [this, &pos]( utl::ISubject* pObject ) { m_objectToPos.insert( std::make_pair( pObject, pos++ ) ); } );
#else
	for ( std::vector<utl::ISubject*>::const_iterator itObject = m_objects.begin(); itObject != m_objects.end(); ++itObject )
		m_objectToPos.insert( std::make_pair( *itObject, pos++ ) );
#endif

	ENSURE( m_objectToPos.size() == m_objects.size() );
}

size_t CTextListEditor::FindItemPos( const utl::ISubject* pObject ) const
{
	REQUIRE( m_objects.size() == m_objectToPos.size() );

	std::unordered_map<utl::ISubject*, size_t>::const_iterator itFound = m_objectToPos.find( const_cast<utl::ISubject*>( pObject ) );
	return itFound != m_objectToPos.end() ? itFound->second : utl::npos;
}

bool CTextListEditor::SetCaretItem( const utl::ISubject* pCaretObject, bool selectLine /*= true*/ )
{
	TLineIndex caretLineIndex = FindItemLineIndex( pCaretObject );

	if ( -1 == caretLineIndex )
	{
		ASSERT( false );		// caret object not found!
		return false;
	}

	Range<CTextEdit::TCharPos> selRange( LineToCharPos( caretLineIndex ) );

	if ( selectLine )
		selRange = GetLineRange( caretLineIndex );

	if ( selRange == GetSelRange() )
		return false;	// no selection change

	SetSelRange( selRange );
	return true;		// selection changed
}

bool CTextListEditor::SetTopItem( const utl::ISubject* pTopObject )
{
	if ( nullptr == pTopObject )
		return false;

	TLineIndex topLineIndex = FindItemLineIndex( pTopObject );

	if ( -1 == topLineIndex )
	{
		ASSERT( false );		// top object not found!
		return false;
	}

	return SetTopLineIndex( topLineIndex );
}

bool CTextListEditor::UpdateItemsText( void )
{
	std::vector<std::tstring> itemTextLines;
	itemTextLines.reserve( m_objects.size() );

	for ( std::vector<utl::ISubject*>::const_iterator itObject = m_objects.begin(); itObject != m_objects.end(); ++itObject )
		itemTextLines.push_back( FormatCode( *itObject ) );

	return SetText( str::Join( itemTextLines, CTextEdit::s_lineEnd ) );		// true if text changed
}

bool CTextListEditor::UpdateItemIndexesText( const std::vector<size_t>& objectIndexes )
{
	if ( objectIndexes.empty() )
		return false;		// nothing to update

	if ( objectIndexes.size() == m_objects.size() )		// update all objects?
		return UpdateItemsText();

	ASSERT( objectIndexes.size() < m_objects.size() );

	std::vector<std::tstring> itemTextLines;
	str::SplitLines( itemTextLines, GetText().c_str(), CTextEdit::s_lineEnd );		// get existing text

	if ( itemTextLines.empty() || objectIndexes.size() == m_objects.size() )
		return UpdateItemsText();

	ASSERT( itemTextLines.size() == m_objects.size() );

	for ( std::vector<size_t>::const_iterator itItemPos = objectIndexes.begin(); itItemPos != objectIndexes.end(); ++itItemPos )
		if ( *itItemPos != utl::npos )
			itemTextLines[*itItemPos] = FormatCode( GetAt( *itItemPos ) );
		else
			ASSERT( false );

	bool changed = ReplaceText( str::Join( itemTextLines, CTextEdit::s_lineEnd ) );

	SetModify( FALSE );		// reset Modify flag set by ReplaceText()
	return changed;			// text changed
}

bool CTextListEditor::ValidateText( ui::CTextValidator& rValidator ) override
{
	if ( m_useLockedItemCount )
		rValidator.m_itemCount = GetItemCount();	// store the locked item count (desired)

	return __super::ValidateText( rValidator );
}
