
#include "stdafx.h"
#include "CommandService.h"
#include "CommandModel.h"
#include "EnumTags.h"

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

utl::ICommand* CCommandService::PeekCmd( svc::StackType stackType ) const
{
	return svc::Undo == stackType ? m_pCommandModel->PeekUndo() : m_pCommandModel->PeekRedo();
}

bool CCommandService::CanUndoRedo( svc::StackType stackType, int cmdTypeId /*= 0*/ ) const
{
	if ( utl::ICommand* pTopCmd = PeekCmdAs< utl::ICommand >( stackType ) )
		if ( 0 == cmdTypeId || cmdTypeId == pTopCmd->GetTypeID() )
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
