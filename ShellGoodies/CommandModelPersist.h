#ifndef CommandModelPersist_h
#define CommandModelPersist_h
#pragma once

#include <deque>
#include "utl/ICommand.h"
#include "AppCommands_fwd.h"


class CCommandModel;


class CCommandModelPersist
{
public:
	static bool SaveUndoLog( CCommandModel* pCommandModel );
	static bool LoadUndoLog( CCommandModel* pOutCommandModel );

	static fs::CPath GetUndoLogPath( void );
};


#include "utl/Serialization.h"


namespace cmd
{
	abstract class CLogSerializer
	{
	protected:
		CLogSerializer( CCommandModel* pCommandModel ) : m_pCommandModel( pCommandModel ) { ASSERT_PTR( m_pCommandModel ); }
	public:
		virtual ~CLogSerializer() {}

		virtual bool Save( const fs::CPath& undoLogPath ) = 0;
		virtual bool Load( const fs::CPath& undoLogPath ) = 0;
	protected:
		static const CEnumTags& GetTags_Section( void );
	protected:
		CCommandModel* m_pCommandModel;
	};


	class CBinaryLogSerializer : public CLogSerializer
							   , private serial::IStreamable
	{
	public:
		CBinaryLogSerializer( CCommandModel* pCommandModel ) : CLogSerializer( pCommandModel ) {}

		// base overrides
		virtual bool Save( const fs::CPath& undoLogPath ) override;
		virtual bool Load( const fs::CPath& undoLogPath ) override;
	private:
		// serial::IStreamable interface
		virtual void Save( CArchive& archive ) override throws_( CException* );
		virtual void Load( CArchive& archive ) override throws_( CException* );

		static void SaveStack( CArchive& archive, svc::StackType section, const std::deque< utl::ICommand* >& cmdStack );
		static void LoadStack( CArchive& archive, std::deque< utl::ICommand* >& rCmdStack );
	};
}


#endif // CommandModelPersist_h
