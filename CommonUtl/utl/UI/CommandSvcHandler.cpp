
#include "pch.h"
#include "CommandSvcHandler.h"
#include "RegistrySerialization.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	CCommandSvcHandler::CCommandSvcHandler( size_t maxSize /*= 20*/ )
		: CCmdTarget()
		, m_maxSize( maxSize )
		, m_dirty( false )
	{
	}

	CCommandSvcHandler::~CCommandSvcHandler()
	{
	}

	bool CCommandSvcHandler::Execute( utl::ICommand* pCmd ) implement
	{
		if ( !m_cmdModel.Execute( pCmd ) )
			return false;

		m_cmdModel.RemoveExpiredCommands( m_maxSize );
		m_dirty = true;
		return true;
	}

	bool CCommandSvcHandler::LoadState( const TCHAR* pRegSection, const TCHAR* pEntry )
	{
		return reg::LoadProfileBinaryState( pRegSection, pEntry, &m_cmdModel );

		// note: on loading, caller must call this->ReHostCommands() to rebind the host to the loaded commands
	}

	bool CCommandSvcHandler::SaveState( const TCHAR* pRegSection, const TCHAR* pEntry ) const
	{
		return m_dirty && reg::SaveProfileBinaryState( pRegSection, pEntry, &m_cmdModel );
	}

	bool CCommandSvcHandler::DeleteStateFromRegistry( const TCHAR* pRegSection, const TCHAR* pEntry )
	{
		return reg::DeleteProfileBinaryState( pRegSection, pEntry );
	}

	void CCommandSvcHandler::Serialize( CArchive& archive ) overrides(CObject)
	{
		m_cmdModel.Serialize( archive );
	}


	// command handlers

	BEGIN_MESSAGE_MAP( CCommandSvcHandler, CCmdTarget )
		ON_COMMAND( ID_EDIT_UNDO, On_EditUndo )
		ON_UPDATE_COMMAND_UI( ID_EDIT_UNDO, OnUpdate_EditUndo )
		ON_COMMAND( ID_EDIT_REDO, On_EditRedo )
		ON_UPDATE_COMMAND_UI( ID_EDIT_REDO, OnUpdate_EditRedo )
	END_MESSAGE_MAP()

	void CCommandSvcHandler::On_EditUndo( void )
	{
		m_cmdModel.Undo();
		m_dirty = true;
	}

	void CCommandSvcHandler::OnUpdate_EditUndo( CCmdUI* pCmdUI )
	{
		pCmdUI->Enable( m_cmdModel.CanUndo() );
	}

	void CCommandSvcHandler::On_EditRedo( void )
	{
		m_cmdModel.Redo();
		m_dirty = true;
	}

	void CCommandSvcHandler::OnUpdate_EditRedo( CCmdUI* pCmdUI )
	{
		pCmdUI->Enable( m_cmdModel.CanRedo() );
	}
}
