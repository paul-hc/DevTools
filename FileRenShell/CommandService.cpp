
#include "stdafx.h"
#include "CommandService.h"
#include "CommandModelService.h"
#include "GeneralOptions.h"
#include "utl/CommandModel.h"
#include "utl/EnumTags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace svc
{
	const CEnumTags& GetTags_StackType( void )
	{
		static const CEnumTags tags( _T("Undo|Redo") );
		return tags;
	}
}


CCommandService::CCommandService( void )
	: m_pCommandModel( new CCommandModel )
	, m_dirty( false )
{
}

CCommandService::~CCommandService()
{
}

bool CCommandService::SaveCommandModel( void )
{
	if ( !CGeneralOptions::Instance().m_undoLogPersist )
		return false;

	// cleanup the command stacks before saving
	m_pCommandModel->RemoveExpiredCommands( MaxCommands );
	RemoveCommandsThat( pred::IsZombieCmd() );		// zombie command: it has no effect on files (in most cases empty macros due to non-existing files)

	if ( !CCommandModelService::SaveUndoLog( *m_pCommandModel, CGeneralOptions::Instance().m_undoLogFormat ) )
		return false;

	m_dirty = false;
	return true;
}

bool CCommandService::LoadCommandModel( void )
{
	ASSERT( m_pCommandModel->IsUndoEmpty() && m_pCommandModel->IsRedoEmpty() );		// load once

	if ( CGeneralOptions::Instance().m_undoLogPersist )
		if ( CCommandModelService::LoadUndoLog( m_pCommandModel.get() ) )			// load the most recently modified log file (regardless of CGeneralOptions::m_undoLogFormat)
		{
			m_dirty = false;
			return true;
		}

	return false;
}

utl::ICommand* CCommandService::PeekCmd( svc::StackType stackType ) const
{
	return svc::Undo == stackType ? m_pCommandModel->PeekUndo() : m_pCommandModel->PeekRedo();
}

bool CCommandService::CanUndoRedo( svc::StackType stackType, int typeId /*= 0*/ ) const
{
	if ( utl::ICommand* pTopCmd = PeekCmdAs< utl::ICommand >( stackType ) )
		if ( 0 == typeId || typeId == pTopCmd->GetTypeID() )
			return svc::Undo == stackType
				? m_pCommandModel->CanUndo()
				: m_pCommandModel->CanRedo();

	return false;
}

bool CCommandService::UndoRedo( svc::StackType stackType )
{
	m_dirty = true;
	return svc::Undo == stackType ? m_pCommandModel->Undo() : m_pCommandModel->Redo();
}

bool CCommandService::Execute( utl::ICommand* pCmd )
{
	if ( !m_pCommandModel->Execute( pCmd ) )
		return false;

	m_dirty = true;
	return true;
}
