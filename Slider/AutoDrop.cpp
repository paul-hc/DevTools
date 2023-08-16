
#include "pch.h"
#include "AutoDrop.h"
#include "SearchPattern.h"
#include "FileAttr.h"
#include "ImageFileEnumerator.h"
#include "Application_fwd.h"
#include "resource.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/ProgressService.h"
#include <map>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace auto_drop
{
	// COpEntry implementation

	COpEntry::COpEntry( const std::tstring& srcFullPath, const std::tstring& destFileExt, bool imageWasMoved )
		: m_srcFullPath( srcFullPath )
		, m_destFileExt( destFileExt )
		, m_imageWasMoved( imageWasMoved )
	{
	}

	COpEntry::COpEntry( const std::tstring& srcFullPath, int fileNumber, bool imageWasMoved )
		: m_srcFullPath( srcFullPath )
		, m_imageWasMoved( imageWasMoved )
	{
		m_destFileExt = CSearchPattern::FormatNumericFilePath( path::FindFilename( m_srcFullPath.c_str() ), fileNumber ).c_str();
	}

	void COpEntry::Stream( CArchive& archive )
	{
		archive
			& m_srcFullPath & m_destFileExt & m_imageWasMoved;
	}

	bool COpEntry::CheckCopyOpConsistency( const std::tstring& destDirPath )
	{
		bool imageWasMovedOrg = m_imageWasMoved;

		if ( !m_imageWasMoved )
			if ( path::Equivalent( path::GetParentPath( m_srcFullPath.c_str() ).c_str(), destDirPath.c_str() ) )
				m_imageWasMoved = true;			// COPY operation specified -> change to MOVE since source and dest dirs are the same

		ASSERT( !path::Equivalent( m_srcFullPath.c_str(), GetDestFullPath( destDirPath ).c_str() ) );
		return m_imageWasMoved == imageWasMovedOrg;		// consistent if operation didn't change
	}

	std::tstring COpEntry::GetMidDestPath( const std::tstring& destDirPath ) const
	{
		ASSERT( !m_destFileExt.empty() && !destDirPath.empty() );

		static const std::tstring fnamePrefix = _T("_");
		return path::Combine( destDirPath.c_str(), ( fnamePrefix + m_destFileExt ).c_str() ).c_str();
	}


	// COpGroup implementation

	void COpGroup::Stream( CArchive& archive )
	{
		if ( archive.IsStoring() )
		{
			archive << m_type;
			archive << m_groupID;
		}
		else
		{
			archive >> (int&)m_type;
			archive >> m_groupID;
		}
		serial::StreamItems( archive, *this );
	}

} // namespace auto_drop


namespace auto_drop
{
	// CContext implementation

	CMenu CContext::s_dropContextMenu;

	CContext::CContext( void )
		: m_dropOperation( PromptUser )
		, m_dropScreenPos( -1, -1 )
	{
		if ( nullptr == s_dropContextMenu.GetSafeHmenu() )
			ui::LoadPopupMenu( &s_dropContextMenu, IDR_CONTEXT_MENU, app::DropPopup );
	}

	CContext::~CContext()
	{
	}

	void CContext::Clear( void )
	{
		m_pDestSearchPattern.reset();
		m_droppedSrcFiles.clear();
		m_droppedDestFiles.clear();
		m_dropOperation = PromptUser;
		m_insertBefore.Clear();
		m_dropScreenPos = CPoint( -1, -1 );
	}

	bool CContext::IsValidDropRecipient( bool checkValidPath /*= true*/ ) const
	{
		return m_pDestSearchPattern.get() != nullptr && m_pDestSearchPattern->IsAutoDropDirPath( checkValidPath );
	}

	const fs::CPath& CContext::GetDestSearchPath( void ) const
	{
		ASSERT_PTR( m_pDestSearchPattern.get() );
		return m_pDestSearchPattern->GetFilePath();
	}

	bool CContext::InitAutoDropRecipient( const CSearchPattern& destSearchPattern )
	{
		if ( !destSearchPattern.IsEmpty() )
			m_pDestSearchPattern.reset( new CSearchPattern( destSearchPattern ) );
		else
			m_pDestSearchPattern.reset();

		if ( !IsValidDropRecipient() )
			return false;

		TRACE( _T("*** Initializing AutoDropRecipient for directory: %s\n"), GetDestSearchPath().GetPtr() );
		return true;
	}

	size_t CContext::SetupDroppedFiles( HDROP hDropInfo, const fs::CFlexPath& insertBefore )
	{
		ASSERT_PTR( hDropInfo );

		Clear();
		if ( !IsValidDropRecipient( true ) )
			return 0;			// invalid recipient directory

		m_insertBefore = insertBefore;
		ASSERT( m_insertBefore.FileExist() );
		GetCursorPos( &m_dropScreenPos );

		if ( ui::IsKeyPressed( VK_CONTROL ) )
			m_dropOperation = FileCopy;
		else if ( ui::IsKeyPressed( VK_SHIFT ) )
			m_dropOperation = FileMove;

		UINT fileCount = ::DragQueryFile( hDropInfo, ( UINT )-1, nullptr, 0 );
		bool abortDrop = false;

		for ( UINT i = 0; i != fileCount && !abortDrop; ++i )
		{
			TCHAR buffer[ MAX_PATH ];
			::DragQueryFile( hDropInfo, i, buffer, _MAX_PATH );

			fs::CPath imageFilePath( buffer );
			if ( imageFilePath.FileExist() )
				m_droppedSrcFiles.push_back( imageFilePath.GetPtr() );

			if ( !m_insertBefore.IsEmpty() )
				if ( m_insertBefore == imageFilePath )
					abortDrop = true;		// one of the dropped files are the same as insertion point!
		}
		::DragFinish( hDropInfo );

		if ( abortDrop )
		{	// avoid droopping on the same file
			Clear();
			return 0;
		}

		if ( m_dropOperation != FileMove && !m_droppedSrcFiles.empty() )
		{	// check to see if is not actually a MOVE operation for all the dropped files
			bool allSameSrcDestDir = true;

			for ( std::vector<std::tstring>::const_iterator itDrop = m_droppedSrcFiles.begin(); allSameSrcDestDir && itDrop != m_droppedSrcFiles.end(); ++itDrop )
				allSameSrcDestDir = GetDestSearchPath() == path::GetParentPath( itDrop->c_str() );

			if ( allSameSrcDestDir )
				m_dropOperation = FileMove;		// automatically set the MOVE operation
		}

		return m_droppedSrcFiles.size();
	}

	bool CContext::SearchForImages( CImagesModel* pFoundImagesModel ) const
	{
		ASSERT_PTR( pFoundImagesModel );

		CProgressService progress( AfxGetMainWnd(), app::GetSearchingOpLabel() );

		try
		{
			CImageFileEnumerator imageEnum( progress.GetProgressEnumerator() );

			imageEnum.Search( *m_pDestSearchPattern );
			imageEnum.SwapFoundImages( *pFoundImagesModel );
		}
		catch ( CException* pExc )
		{
			app::HandleReportException( pExc );
		}

		return !pFoundImagesModel->IsEmpty();
	}

	bool CContext::MakeAutoDrop( COpStack& dropUndoStack )
	{
		if ( !IsValidDropRecipient( true ) || 0 == GetFileCount() )
			return false;

		CImagesModel foundImagesModel;
		SearchForImages( &foundImagesModel );		// search for all the existing image files

		const std::vector<CFileAttr*>& existingFiles = foundImagesModel.GetFileAttrs();

		// NOTE: existingFiles contains only files that have numeric file-name
		std::map<fs::CFlexPath, int> shiftMap;
		std::map<fs::CFlexPath, int>::iterator itMap;

		for ( std::vector<CFileAttr*>::const_iterator itFileAttr = existingFiles.begin(); itFileAttr != existingFiles.end(); ++itFileAttr )
			shiftMap[ ( *itFileAttr )->GetPath() ] = CSearchPattern::ParseNumFileNameNumber( ( *itFileAttr )->GetPath().GetPtr() );

		// exclude files dropped from the auto-drop directory
		for ( std::vector<std::tstring>::const_iterator itDrop = m_droppedSrcFiles.begin(); itDrop != m_droppedSrcFiles.end(); ++itDrop )
		{
			itMap = shiftMap.find( fs::CFlexPath( *itDrop ) );
			if ( itMap != shiftMap.end() )
				shiftMap.erase( itMap );			// file dropped from/to auto-drop directory
		}

		// shift the new numbers to make room for dropped files
		std::map<fs::CFlexPath, int>::iterator itInsDrop = m_insertBefore.IsEmpty() ? shiftMap.end() : shiftMap.find( m_insertBefore );
		int fileNumber = 1, fileNumberDrop = 0;

		// reorder the entire map taking care to make room at insertion point (if any) for the dropped files
		for ( itMap = shiftMap.begin(); itMap != shiftMap.end(); ++itMap, ++fileNumber )
		{
			if ( itMap == itInsDrop )
			{
				fileNumberDrop = fileNumber;
				fileNumber += (int)m_droppedSrcFiles.size();
			}

			itMap->second = fileNumber;
		}
		if ( fileNumberDrop == 0 )
			fileNumberDrop = fileNumber;		// no insertion point -> set to the next number (append)

		int groupID = static_cast<int>( CTime::GetCurrentTime().GetTime() );	// use current timestamp as group ID

		// push to the undo stack the group of dropped files
		dropUndoStack.push_front( COpGroup( COpGroup::Dropped, groupID ) );

		COpGroup& rDroppedGroup = dropUndoStack.front();

		// push the dropped files renames to the undo vector
		for ( std::vector<std::tstring>::const_iterator itDrop = m_droppedSrcFiles.begin(); itDrop != m_droppedSrcFiles.end(); ++itDrop, ++fileNumberDrop )
			if ( CSearchPattern::ParseNumFileNameNumber( itDrop->c_str() ) != fileNumberDrop )
			{	// different new number, so push it to the dropped group
				COpEntry droppedEntry( *itDrop, fileNumberDrop, m_dropOperation == FileMove );

				droppedEntry.CheckCopyOpConsistency( GetDestSearchPath().GetPtr() );
				rDroppedGroup.push_front( droppedEntry );
			}

		if ( rDroppedGroup.empty() )
		{	// no dropped rename (since src==dest), so remove this group from undo stack and abort operation
			dropUndoStack.pop_front();
			return false;
		}
		// also build the dropped files destination vector (for selection)
		m_droppedDestFiles.resize( rDroppedGroup.size() );

		std::vector<std::tstring>::iterator itDest = m_droppedDestFiles.begin();

		for ( COpGroup::const_iterator itSrc = rDroppedGroup.begin(); itSrc != rDroppedGroup.end(); ++itSrc, ++itDest )
			*itDest = itSrc->GetDestFullPath( GetDestSearchPath().GetPtr() );

		// push to the undo stack the group of entries for the shifted existing files (src & dest)
		if ( shiftMap.size() > 0 )
		{	// push a new group for shifted existing files
			dropUndoStack.push_front( COpGroup( COpGroup::Existing, groupID ) );

			COpGroup& rShiftedGroup = dropUndoStack.front();

			// append rename entries for shifted existing files
			for ( itMap = shiftMap.begin(); itMap != shiftMap.end(); ++itMap )
				if ( CSearchPattern::ParseNumFileNameNumber( itMap->first.GetPtr() ) != itMap->second )
				{	// different new number, so push it to the shifted existing group
					COpEntry existingEntry( itMap->first.GetPtr(), itMap->second, true );

					existingEntry.CheckCopyOpConsistency( GetDestSearchPath().GetPtr() );
					rShiftedGroup.push_front( existingEntry );
				}
			// remove the newly added group if is empty
			if ( rShiftedGroup.empty() )
				dropUndoStack.pop_front();
		}

		return DoAutoDropOperation( dropUndoStack, false );
	}

	/**
		Renames the necessary files in the auto-drop directory so they'll be in contiguous numeric sequence.
		Returns true if any modification is made, otherwise false.
	*/
	bool CContext::DefragmentFiles( COpStack& dropUndoStack )
	{
		if ( !IsValidDropRecipient( true ) )
			return false;			// invalid target search pattern

		CImagesModel foundImagesModel;
		if ( !SearchForImages( &foundImagesModel ) )		// search for all the existing image files
			return false;			// no files found -> skip reorder

		// push to the undo stack the group of entries for the shifted existing files
		dropUndoStack.push_front( COpGroup( COpGroup::Existing, static_cast<int>( CTime::GetCurrentTime().GetTime() ) ) );

		COpGroup& rShiftedGroup = dropUndoStack.front();

		// NOTE: existingFiles contains only files that have numeric file-name
		const std::vector<CFileAttr*>& existingFiles = foundImagesModel.GetFileAttrs();

		// append rename entries for shifted existing files
		for ( size_t i = 0; i != existingFiles.size(); ++i )
		{
			const std::tstring& filePath = existingFiles[ i ]->GetPath().Get();
			const int fileNumber = 1 + static_cast<int>( i );

			if ( CSearchPattern::ParseNumFileNameNumber( filePath.c_str() ) != fileNumber )
			{	// different new number, so push it to the shifted existing group
				COpEntry existingEntry( filePath, fileNumber, true );

				existingEntry.CheckCopyOpConsistency( GetDestSearchPath().GetPtr() );
				rShiftedGroup.push_front( existingEntry );
			}
		}

		// remove the newly added group if is empty
		if ( rShiftedGroup.empty() )
		{
			dropUndoStack.pop_front();
			return false;			// no defragmentation required
		}
		return DoAutoDropOperation( dropUndoStack, false );
	}

	bool CContext::UndoRedoOperation( COpStack& rFromStack, COpStack& rToStack, bool isUndoOp )
	{
		COpStack::const_iterator itGroup;
		int groupID = rFromStack.front().m_groupID;

		// transfer all the groups containing the same groupID from UNDO to REDO stack (thus reversing the stack order)
		for ( ; itGroup = rFromStack.begin(), itGroup != rFromStack.end() && itGroup->m_groupID == groupID; ++itGroup )
		{
			rToStack.push_front( *itGroup );		// push a copy by value
			rFromStack.pop_front();					// pop the transferred group (this will invalidate itGroup -> reassign in the loop)
		}

		// undo file operations for all the groups in redo stack
		return DoAutoDropOperation( rToStack, isUndoOp );
	}

	static LPCTSTR opName[ 3 ] = { _T("MOVE"), _T("COPY"), _T("DELETE_SRC") };

	bool CContext::DoAutoDropOperation( const COpStack& dropStack, bool isUndoOp ) const
	{
		ASSERT( dropStack.size() > 0 );

		CWaitCursor wait;
		COpStack::const_iterator itGroup;
		int groupID = dropStack.front().m_groupID;

		// make a two phase operation in order to avoid filename conflicts
		for ( int phaseNo = 1; phaseNo <= 2; ++phaseNo )
			// iterate for all the entries containing the same groupID
			for ( itGroup = dropStack.begin(); itGroup != dropStack.end() && itGroup->m_groupID == groupID; ++itGroup )
			{	// prepare data for two phase-rename
				const COpGroup& group = *itGroup;
				std::vector<fs::CPath> srcPaths, destPaths;

				ASSERT( group.size() > 0 );
				srcPaths.resize( group.size() );
				destPaths.resize( group.size() );

				std::vector<fs::CPath>::iterator itSrc = srcPaths.begin(), itDest = destPaths.begin();

				for ( COpGroup::const_iterator itOp = group.begin(); itOp != group.end(); ++itOp, ++itSrc, ++itDest )
					if ( 1 == phaseNo )
					{
						*itSrc = isUndoOp ? itOp->GetDestFullPath( GetDestSearchPath().Get() ) : itOp->m_srcFullPath;
						*itDest = itOp->GetMidDestPath( GetDestSearchPath().Get() );
					}
					else // 2 == phaseNo
					{
						*itSrc = itOp->GetMidDestPath( GetDestSearchPath().Get() );
						*itDest = isUndoOp ? itOp->m_srcFullPath : itOp->GetDestFullPath( GetDestSearchPath().Get() );
					}

				// NOTE: undoing a COPY actually means DELETE !
				FileSetOperation fileSetOp = GetFileSetOperation( group.front().m_imageWasMoved, isUndoOp, phaseNo );

				if ( !DoFileSetOperation( srcPaths, destPaths, fileSetOp ) )
					TRACE( _T("\t* FAILED %s Phase %d\n"), opName[ fileSetOp ], phaseNo );
			}

		return true;
	}

	bool CContext::DoFileSetOperation( const std::vector<fs::CPath>& srcPaths, const std::vector<fs::CPath>& destPaths,
									   FileSetOperation fileSetOp )
	{
		ASSERT( srcPaths.size() == destPaths.size() );

	#ifdef _DEBUG
		std::vector<fs::CPath>::const_iterator itSrc = srcPaths.begin(), itDest = destPaths.begin();

		TRACE( _T("* %s FileSetOperation\n"), opName[ fileSetOp ] );
		for ( ; itSrc != srcPaths.end(); ++itSrc, ++itDest )
			TRACE( _T("\t%s -> %s\n"), itSrc->GetPtr(), itDest->GetPtr() );
	#endif

		switch ( fileSetOp )
		{
			case MoveSrcToDest:	return shell::MoveFiles( srcPaths, destPaths, AfxGetMainWnd(), FOF_MULTIDESTFILES | FOF_ALLOWUNDO );
			case CopySrcToDest:	return shell::CopyFiles( srcPaths, destPaths, AfxGetMainWnd(), FOF_MULTIDESTFILES | FOF_ALLOWUNDO );
			case DeleteSrc:		return shell::DeleteFiles( srcPaths, AfxGetMainWnd(), FOF_NOCONFIRMATION );
		}
		ASSERT( false );
		return false;
	}

} // namespace auto_drop
