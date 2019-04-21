
#include "stdafx.h"
#include "CommandItem.h"
#include "AppCommands.h"
#include "utl/EnumTags.h"
#include "utl/UI/ToolStrip.h"
#include "utl/UI/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


void CCommandItem::SetCmd( utl::ICommand* pCmd )
{
	m_pCmd = pCmd;
	m_imageIndex = LookupImageIndex( m_pCmd );
	m_code.clear();

	if ( m_pCmd != NULL )
	{
		std::vector< std::tstring > fields;
		cmd::QueryCmdFields( fields, m_pCmd );
		if ( !fields.empty() )
			m_code = fields.front();
	}
}

const std::tstring& CCommandItem::GetCode( void ) const
{
	return m_code;
}

CImageList* CCommandItem::GetImageList( void )
{
	return GetCmdTypeStrip().m_pImageList.get();
}

CToolStrip& CCommandItem::GetCmdTypeStrip( void )
{
	static CToolStrip s_strip;
	if ( !s_strip.IsValid() )
	{
		s_strip.AddButton( UINT_MAX, ID_EDIT_DETAILS );			// unknown command image

		s_strip.AddButton( cmd::RenameFile, ID_RENAME_ITEM );
		s_strip.AddButton( cmd::TouchFile, ID_TOUCH_FILES );
		s_strip.AddButton( cmd::FindDuplicates, ID_FIND_DUPLICATE_FILES );
		s_strip.AddButton( cmd::DeleteFiles, ID_CMD_DELETE_FILES );
		s_strip.AddButton( cmd::CopyFiles, ID_CMD_COPY_FILES );
		s_strip.AddButton( cmd::PasteCopyFiles, ID_PASTE_DEEP_POPUP );
		s_strip.AddButton( cmd::MoveFiles, ID_CMD_MOVE_FILES );
		s_strip.AddButton( cmd::PasteMoveFiles, ID_PASTE_DEEP_POPUP );
		s_strip.AddButton( cmd::PasteCreateFolders, ID_CREATE_FOLDERS );
		s_strip.AddButton( cmd::PasteCreateDeepFolders, ID_CREATE_DEEP_FOLDER_STRUCT );
		s_strip.AddButton( cmd::ChangeDestPaths, ID_CMD_CHANGE_DEST_PATHS );
		s_strip.AddButton( cmd::ChangeDestFileStates, ID_CMD_CHANGE_DEST_FILE_STATES );
		s_strip.AddButton( cmd::ResetDestinations, ID_CMD_RESET_DESTINATIONS );
		s_strip.AddButton( cmd::EditOptions, ID_OPTIONS );
	}
	return s_strip;
}

int CCommandItem::LookupImageIndex( utl::ICommand* pCmd )
{
	const CToolStrip& strip = GetCmdTypeStrip();
	size_t imagePos = strip.FindButtonPos( pCmd != NULL ? pCmd->GetTypeID() : UINT_MAX );

	if ( utl::npos == imagePos )
		imagePos = strip.FindButtonPos( UINT_MAX );

	ENSURE( imagePos < strip.m_buttonIds.size() );
	return static_cast< int >( imagePos );
}
