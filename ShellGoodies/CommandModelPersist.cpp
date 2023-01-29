
#include "pch.h"
#include "CommandModelPersist.h"
#include "utl/AppTools.h"
#include "utl/Command.h"
#include "utl/CommandModel.h"
#include "utl/EnumTags.h"
#include "utl/FileSystem.h"
#include "utl/Guards.h"
#include "utl/Serialization.h"
#include "utl/Timer.h"
#include "utl/TimeUtils.h"
#include "utl/UI/MfcUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/Serialization.hxx"


// CCommandModelPersist class

bool CCommandModelPersist::SaveUndoLog( CCommandModel* pCommandModel )
{
	//utl::CSlowSectionGuard slow( _T("CCommandModelPersist::SaveUndoLog"), 0.01 );

	cmd::CBinaryLogSerializer serializer( pCommandModel );
	return serializer.Save( GetUndoLogPath() );
}

bool CCommandModelPersist::LoadUndoLog( CCommandModel* pOutCommandModel )
{
	//utl::CSlowSectionGuard slow( _T("CCommandModelPersist::LoadUndoLog()"), 0.01 );
	cmd::CBinaryLogSerializer serializer( pOutCommandModel );
	return serializer.Load( GetUndoLogPath() );
}

fs::CPath CCommandModelPersist::GetUndoLogPath( void )
{
	static const CEnumTags s_logExtTags( _T(".dat") );
	fs::CPathParts parts( app::GetModulePath().Get() );

	parts.m_fname += _T("_undo");
	parts.m_ext = s_logExtTags.FormatUi( cmd::BinaryFormat );
	return fs::CPath( parts.MakePath() );
}


namespace cmd
{
	// CLogSerializer class

	const CEnumTags& CLogSerializer::GetTags_Section( void )
	{
		static const CEnumTags s_tags( _T("[UNDO SECTION]|[REDO SECTION]") );
		return s_tags;
	}


	// CBinaryLogSerializer implementation

	bool CBinaryLogSerializer::Save( const fs::CPath& undoLogPath ) override
	{
		ui::CAdapterDocument doc( this, undoLogPath.Get() );
		return doc.Save();
	}

	bool CBinaryLogSerializer::Load( const fs::CPath& undoLogPath ) override
	{
		ui::CAdapterDocument doc( this, undoLogPath.Get() );
		return doc.Load();
	}

	void CBinaryLogSerializer::Save( CArchive& archive ) override throws_( CException* )
	{
		SaveStack( archive, svc::Undo, m_pCommandModel->GetUndoStack() );
		SaveStack( archive, svc::Redo, m_pCommandModel->GetRedoStack() );
	}

	void CBinaryLogSerializer::Load( CArchive& archive ) override throws_( CException* )
	{
		std::deque< utl::ICommand* > undoStack, redoStack;
		LoadStack( archive, undoStack );
		LoadStack( archive, redoStack );

		m_pCommandModel->SwapUndoStack( undoStack );
		m_pCommandModel->SwapRedoStack( redoStack );
	}

	void CBinaryLogSerializer::SaveStack( CArchive& archive, svc::StackType section, const std::deque< utl::ICommand* >& cmdStack )
	{
		archive << &GetTags_Section().FormatUi( section );			// as Utf8; just for inspection

		serial::Save_CObjects_Mixed( archive, cmdStack );
	}

	void CBinaryLogSerializer::LoadStack( CArchive& archive, std::deque< utl::ICommand* >& rCmdStack )
	{
		std::tstring sectionTag; sectionTag;
		archive >> &sectionTag;			// as Utf8; just discard it

		serial::Load_CObjects_Mixed( archive, rCmdStack );
	}

} //namespace cmd
