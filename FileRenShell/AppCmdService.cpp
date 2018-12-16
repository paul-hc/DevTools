
#include "stdafx.h"
#include "AppCmdService.h"
#include "CommandModelService.h"
#include "GeneralOptions.h"
#include "utl/CommandModel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CAppCmdService::CAppCmdService( void )
	: CCommandService()
{
}

CAppCmdService::~CAppCmdService()
{
}

bool CAppCmdService::SaveCommandModel( void )
{
	if ( !CGeneralOptions::Instance().m_undoLogPersist )
		return false;

	// cleanup the command stacks before saving
	m_pCommandModel->RemoveExpiredCommands( MaxCommands );
	RemoveCommandsThat( pred::IsZombieCmd() );		// zombie command: it has no effect on files (in most cases empty macros due to non-existing files)

	if ( !CCommandModelService::SaveUndoLog( *m_pCommandModel, CGeneralOptions::Instance().m_undoLogFormat ) )
		return false;

	SetDirty( false );
	return true;
}

bool CAppCmdService::LoadCommandModel( void )
{
	ASSERT( m_pCommandModel->IsUndoEmpty() && m_pCommandModel->IsRedoEmpty() );		// load once

	if ( CGeneralOptions::Instance().m_undoLogPersist )
		if ( CCommandModelService::LoadUndoLog( m_pCommandModel.get() ) )			// load the most recently modified log file (regardless of CGeneralOptions::m_undoLogFormat)
		{
			SetDirty( false );
			return true;
		}

	return false;
}
