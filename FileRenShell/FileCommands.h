#ifndef FileCommands_h
#define FileCommands_h
#pragma once

#include "utl/RuntimeException.h"
#include "FileCommands_fwd.h"
#include "PathItemBase.h"


class CLogger;


namespace cmd
{
	enum UserFeedback { Abort, Retry, Ignore };


	class CUserFeedbackException : public CRuntimeException
	{
	public:
		CUserFeedbackException( UserFeedback feedback );
	private:
		static const CEnumTags& GetTags_Feedback( void );
	public:
		const UserFeedback m_feedback;
	};


	abstract class CFileCmd : public CCommand
	{
	protected:
		CFileCmd( CommandType cmdType, const fs::CPath& srcPath );
	public:
		void ExecuteHandle( void ) throws_( CUserFeedbackException );

		// base overrides
		virtual bool Unexecute( void );
		virtual std::auto_ptr< CFileCmd > MakeUnexecuteCmd( void ) const = 0;
	private:
		UserFeedback HandleFileError( CException* pExc ) const;
		std::tstring ExtractMessage( CException* pExc ) const;
	public:
		const fs::CPath m_srcPath;
	private:
		static const TCHAR s_fmtError[];
	public:
		static CLogger* s_pLogger;
		static IErrorObserver* s_pErrorObserver;
	};


	class CScopedLogger
	{
	public:
		CScopedLogger( CLogger* pLogger ) : m_pOldLogger( CFileCmd::s_pLogger ) { CFileCmd::s_pLogger = pLogger; }
		~CScopedLogger() { CFileCmd::s_pLogger = m_pOldLogger; }
	private:
		CLogger* m_pOldLogger;
	};

	class CScopedErrorObserver
	{
	public:
		CScopedErrorObserver( IErrorObserver* pErrorObserver ) : m_pOldErrorObserver( CFileCmd::s_pErrorObserver ) { CFileCmd::s_pErrorObserver = pErrorObserver; }
		~CScopedErrorObserver() { CFileCmd::s_pErrorObserver = m_pOldErrorObserver; }
	private:
		IErrorObserver* m_pOldErrorObserver;
	};


	class CFileMacroCmd : public CMacroCommand
	{
	public:
		CFileMacroCmd( CommandType subCmdType, const CTime& timestamp = CTime::GetCurrentTime() );

		const CTime& GetTimestamp( void ) const { return m_timestamp; }

		// base overrides
		virtual std::tstring Format( bool detailed ) const;
		virtual bool Execute( void );
		virtual bool Unexecute( void );
	private:
		enum Mode { ExecuteMode, UnexecuteMode };

		void ExecuteMacro( Mode mode );
	private:
		CTime m_timestamp;
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
	virtual bool IsUndoable( void ) const;
	virtual std::auto_ptr< CFileCmd > MakeUnexecuteCmd( void ) const;
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
	virtual bool IsUndoable( void ) const;
	virtual std::auto_ptr< CFileCmd > MakeUnexecuteCmd( void ) const;
public:
	const fs::CFileState m_srcState;
	const fs::CFileState m_destState;
};


// notification command; uses inline execution, not to be executed by the command model with undo/redo
//
class COnDestPathsChangeCmd : public CCommand
{
	COnDestPathsChangeCmd( void ) : CCommand( cmd::DestPathChanged, NULL, &cmd::GetTags_CommandType() ) {}
public:
	static COnDestPathsChangeCmd& Instance( void );

	// base overrides
	virtual bool Execute( void );
};


#endif // FileCommands_h
