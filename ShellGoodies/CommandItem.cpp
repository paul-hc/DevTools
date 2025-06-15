
#include "pch.h"
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

	if ( m_pCmd != nullptr )
	{
		std::vector<std::tstring> fields;
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
	return GetCmdTypeStrip().GetImageList();
}

CToolStrip& CCommandItem::GetCmdTypeStrip( void )
{
	static CToolStrip s_strip;
	if ( !s_strip.IsValid() )
	{
		s_strip.AddButton( UINT_MAX, IDI_UNKNOWN );			// unknown command image

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
		s_strip.AddButton( cmd::CopyPasteFilesAsBackup, ID_PASTE_AS_BACKUP );
		s_strip.AddButton( cmd::CutPasteFilesAsBackup, ID_PASTE_AS_BACKUP );
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
	size_t imagePos = strip.FindButtonPos( ExtractCmdID( pCmd ) );

	if ( utl::npos == imagePos )
		imagePos = strip.FindButtonPos( UINT_MAX );

	ENSURE( imagePos < strip.GetButtonIds().size() );
	return static_cast<int>( imagePos );
}

UINT CCommandItem::ExtractCmdID( utl::ICommand* pCmd )
{
	if ( nullptr == pCmd )
		return UINT_MAX;

	if ( CMacroCommand::MacroCmdId == pCmd->GetTypeID() )		// macro ID with undefined image?
	{
		const CMacroCommand* pMacroCmd = checked_static_cast<const CMacroCommand*>( pCmd );

		if ( pMacroCmd->GetMainCmd() != nullptr )
			pCmd = pMacroCmd->GetMainCmd();						// use the main command's image
		else if ( !pMacroCmd->IsEmpty() )
			pCmd = pMacroCmd->GetSubCommands().front();			// use the first command's image
	}

	return pCmd->GetTypeID();
}
