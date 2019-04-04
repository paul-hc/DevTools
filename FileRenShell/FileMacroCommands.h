#ifndef FileMacroCommands_h
#define FileMacroCommands_h
#pragma once

#include "AppCommands.h"
#include "utl/Path.h"
#include "utl/RuntimeException.h"


namespace cmd
{
	class CUserFeedbackException;

	enum UserFeedback { Abort, Retry, Ignore };


	class CFileMacroCmd : public CObject
						, public CMacroCommand
	{
		DECLARE_SERIAL( CFileMacroCmd )

		CFileMacroCmd( void ) {}
	public:
		CFileMacroCmd( CommandType subCmdType, const CTime& timestamp = CTime::GetCurrentTime() );

		const CTime& GetTimestamp( void ) const { return m_timestamp; }

		// base overrides
		virtual std::tstring Format( utl::Verbosity verbosity ) const;
		virtual bool Execute( void );
		virtual bool Unexecute( void );

		virtual void Serialize( CArchive& archive );
	private:
		enum Mode { ExecuteMode, UnexecuteMode };

		void ExecuteMacro( Mode mode );
	private:
		CTime m_timestamp;
	};


	// abstract base for file leaf commands embedded in a CFileMacroCmd, that operates on a single file
	//
	abstract class CBaseFileCmd : public CBaseSerialCmd
	{
	protected:
		CBaseFileCmd( CommandType cmdType = CommandType(), const fs::CPath& srcPath = fs::CPath() );
	public:
		void ExecuteHandle( void ) throws_( CUserFeedbackException );

		// base overrides
		virtual bool Unexecute( void );
		virtual std::auto_ptr< CBaseFileCmd > MakeUnexecuteCmd( void ) const = 0;
	private:
		UserFeedback HandleFileError( CException* pExc, const fs::CPath& srcPath ) const;
		static std::tstring ExtractMessage( CException* pExc, const fs::CPath& srcPath );
	public:
		fs::CPath m_srcPath;
	private:
		static const TCHAR s_fmtError[];
	};
}


class CRenameFileCmd : public cmd::CBaseFileCmd
{
	DECLARE_SERIAL( CRenameFileCmd )

	CRenameFileCmd( void ) {}
public:
	CRenameFileCmd( const fs::CPath& srcPath, const fs::CPath& destPath );
	virtual ~CRenameFileCmd();

	// ICommand interface
	virtual std::tstring Format( utl::Verbosity verbosity ) const;
	virtual bool Execute( void );
	virtual bool IsUndoable( void ) const;
	virtual std::auto_ptr< CBaseFileCmd > MakeUnexecuteCmd( void ) const;

	// base overrides
	virtual void Serialize( CArchive& archive );
public:
	fs::CPath m_destPath;
};


#include "utl/FileState.h"


class CTouchFileCmd : public cmd::CBaseFileCmd
{
	DECLARE_SERIAL( CTouchFileCmd )

	CTouchFileCmd( void ) {}
public:
	CTouchFileCmd( const fs::CFileState& srcState, const fs::CFileState& destState );
	virtual ~CTouchFileCmd();

	// ICommand interface
	virtual std::tstring Format( utl::Verbosity verbosity ) const;
	virtual bool Execute( void );
	virtual bool IsUndoable( void ) const;
	virtual std::auto_ptr< CBaseFileCmd > MakeUnexecuteCmd( void ) const;

	// base overrides
	virtual void Serialize( CArchive& archive );
public:
	fs::CFileState m_srcState;
	fs::CFileState m_destState;
};


#endif // FileMacroCommands_h
