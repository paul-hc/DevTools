
#include "pch.h"
#include "CommandService.h"
#include "CommandModel.h"
#include "Algorithms.h"
#include "EnumTags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace svc
{
	const CEnumTags& GetTags_StackType( void )
	{
		static const CEnumTags s_tags( _T("Undo|Redo"), _T("undo|redo") );
		return s_tags;
	}
}


CCommandService::CCommandService( void )
	: m_pCommandModel( new CCommandModel() )
	, m_dirty( false )
{
}

CCommandService::~CCommandService()
{
}

size_t CCommandService::FindCmdTopPos( svc::StackType stackType, utl::ICommand* pCmd ) const implement
{
	const std::deque<utl::ICommand*>& rStack = svc::Undo == stackType ? m_pCommandModel->GetUndoStack() : m_pCommandModel->GetRedoStack();
	size_t topPos = utl::FindPos( rStack, pCmd );
	if ( topPos != utl::npos )
		topPos = rStack.size() - topPos - 1;		// position from the top

	return topPos;
}

utl::ICommand* CCommandService::PeekCmd( svc::StackType stackType ) const implement
{
	return svc::Undo == stackType ? m_pCommandModel->PeekUndo() : m_pCommandModel->PeekRedo();
}

bool CCommandService::CanUndoRedo( svc::StackType stackType, int cmdTypeId /*= 0*/ ) const implement
{
	if ( utl::ICommand* pTopCmd = PeekCmdAs<utl::ICommand>( stackType ) )
		if ( 0 == cmdTypeId || cmdTypeId == pTopCmd->GetTypeID() )
			return svc::Undo == stackType
				? m_pCommandModel->CanUndo()
				: m_pCommandModel->CanRedo();

	return false;
}

bool CCommandService::UndoRedo( svc::StackType stackType ) implement
{
	m_dirty = true;
	return svc::Undo == stackType ? m_pCommandModel->Undo() : m_pCommandModel->Redo();
}

bool CCommandService::Execute( utl::ICommand* pCmd ) implement
{
	if ( !m_pCommandModel->Execute( pCmd ) )
		return false;

	m_dirty = true;
	return true;
}
