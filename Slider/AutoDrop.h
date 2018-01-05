#ifndef AutoDrop_h
#define AutoDrop_h
#pragma once

#include "utl/Serialization.h"
#include "utl/FlexPath.h"
#include "FileList.h"
#include <deque>
#include <vector>


namespace auto_drop
{
	// auto-drop: file reorder & rename


	class COpEntry
	{
	public:
		COpEntry( void ) : m_imageWasMoved( true ) {}
		COpEntry( const std::tstring& srcFullPath, const std::tstring& destFileExt, bool imageWasMoved );
		COpEntry( const std::tstring& srcFullPath, int fileNumber, bool imageWasMoved );

		void Stream( CArchive& archive );

		std::tstring GetDestFullPath( const std::tstring& destDirPath ) const { return path::Combine( destDirPath.c_str(), m_destFileExt.c_str() ).c_str(); }
		std::tstring GetMidDestPath( const std::tstring& destDirPath ) const;

		bool CheckCopyOpConsistency( const std::tstring& destDirPath );
	public:
		persist std::tstring m_srcFullPath;		// source full path in a drop copy/move operation
		persist std::tstring m_destFileExt;		// destination fileext, situated in the auto-drop directory
		persist bool m_imageWasMoved;		// if true the file was moved, otherwise was copied
	};


	// group of potential multiple move/rename operations

	class COpGroup : public std::deque< COpEntry >
	{
		typedef std::deque< COpEntry > BaseType;
	public:
		enum GroupType
		{
			Dropped,			// group of dropped files
			Existing			// group of existing files shifted-up
		};
	public:
		COpGroup( GroupType type = Dropped, int groupID = 0 ) : m_type( type ), m_groupID( groupID ) {}

		void Stream( CArchive& archive );
	public:
		persist GroupType m_type;
		persist int m_groupID;			// ID used to group multiple undoable rename operations
	};


	// stack of auto-drop file operation groups

	class COpStack : public std::deque< COpGroup >
	{
		typedef std::deque< COpGroup > BaseType;
	public:
		COpStack( void ) {}

		void Stream( CArchive& archive ) { serial::StreamItems( archive, *this ); }
	};


	enum FileSetOperation { MoveSrcToDest, CopySrcToDest, DeleteSrc };


	class CContext
	{
	public:
		CContext( void );
		~CContext();

		void Clear( void );

		bool IsValidDropRecipient( bool checkValidPath = true ) const { return m_destSearchSpec.IsAutoDropDirPath( checkValidPath ); }
		size_t GetFileCount( void ) const { return m_droppedSrcFiles.size(); }

		bool InitAutoDropRecipient( const CSearchSpec& destSearchSpec );
		size_t SetupDroppedFiles( HDROP hDropInfo, const fs::CFlexPath& insertBefore );

		bool MakeAutoDrop( COpStack& dropUndoStack );
		bool DefragmentFiles( COpStack& dropUndoStack );

		bool UndoRedoOperation( COpStack& rFromStack, COpStack& rToStack, bool isUndoOp );
	protected:
		bool DoAutoDropOperation( const COpStack& dropStack, bool isUndoOp ) const;

		static bool DoFileSetOperation( const std::vector< std::tstring >& srcPaths, const std::vector< std::tstring >& destPaths,
										FileSetOperation fileSetOp );
		static FileSetOperation GetFileSetOperation( bool imageWasMoved, bool isUndoOp, int phaseNo )
		{
			// NOTE: undoing a COPY actually means DELETE
			return ( imageWasMoved || ( phaseNo == ( isUndoOp ? 1 : 2 ) ) ) ? MoveSrcToDest : ( isUndoOp ? DeleteSrc : CopySrcToDest );
		}
	public:
		enum DropOperation { PromptUser, FileMove, FileCopy };

		std::vector< std::tstring > m_droppedSrcFiles;
		std::vector< std::tstring > m_droppedDestFiles;		// drop source and destionation files
		DropOperation m_dropOperation;
		CSearchSpec m_destSearchSpec;		// search specifier containing the destination folder for dropped files
		fs::CFlexPath m_insertBefore;		// image file path pointed on drop (if any, otherwise empty)
		CPoint m_dropScreenPos;				// screen position for drop

		static CMenu s_dropContextMenu;
	};

} //namespace auto_drop


#endif // AutoDrop_h
