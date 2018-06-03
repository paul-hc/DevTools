#ifndef FileCommands_h
#define FileCommands_h
#pragma once

#include "FileCommands_fwd.h"
#include "BasePathItem.h"


class CUndoLogSerializerTests;


namespace cmd
{
	abstract class CFileCmd : public CCommand
	{
	protected:
		CFileCmd( Command command, const fs::CPath& srcPath );
	public:
		const fs::CPath m_srcPath;
	};


	class CFileMacroCmd : public CMacroCommand
	{
		friend class CUndoLogSerializerTests;
	public:
		CFileMacroCmd( Command subCmdType, IErrorObserver* pErrorObserver, const CTime& timestamp = CTime::GetCurrentTime() );

		IErrorObserver* SetErrorObserver( IErrorObserver* pErrorObserver ) { m_pErrorObserver = pErrorObserver; }

		// base overrides
		virtual std::tstring Format( bool detailed ) const;
		virtual bool Execute( void );
		virtual bool Unexecute( void );
	private:
		enum Mode { ExecuteMode, UnexecuteMode };

		size_t ExecuteMacro( Mode mode );
		void RollbackMacro( size_t doneCmdCount, Mode mode );
		void RemoveCmdAt( size_t pos );

		enum UserFeedback { Abort, Retry, Ignore };

		UserFeedback HandleFileError( const CFileCmd* pCmd, CException* pExc ) const;
		static std::tstring ExtractMessage( const fs::CPath& srcPath, CException* pExc );
	private:
		CTime m_timestamp;
		IErrorObserver* m_pErrorObserver;

		static const TCHAR s_fmtError[];
	};
}


class CRenameFileCmd : public cmd::CFileCmd
{
public:
	CRenameFileCmd( const fs::CPath& srcPath, const fs::CPath& destPath );
	virtual ~CRenameFileCmd();

	// ICommand interface
	virtual std::tstring Format( bool detailed ) const;
	virtual bool Execute( void );
	virtual bool Unexecute( void );
	virtual bool IsUndoable( void ) const;
public:
	const fs::CPath m_destPath;
};


#include "utl/FileState.h"


class CTouchFileCmd : public cmd::CFileCmd
{
public:
	CTouchFileCmd( const fs::CFileState& srcState, const fs::CFileState& destState );
	virtual ~CTouchFileCmd();

	// ICommand interface
	virtual std::tstring Format( bool detailed ) const;
	virtual bool Execute( void );
	virtual bool Unexecute( void );
	virtual bool IsUndoable( void ) const;
public:
	const fs::CFileState m_srcState;
	const fs::CFileState m_destState;
};


#endif // FileCommands_h
