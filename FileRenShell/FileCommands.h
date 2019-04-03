#ifndef FileCommands_h
#define FileCommands_h
#pragma once

#include "utl/PathItemBase.h"
#include "utl/RuntimeException.h"
#include "FileCommands_fwd.h"


class CLogger;
namespace app { enum MsgType; }


namespace cmd
{
	enum UserFeedback { Abort, Retry, Ignore };


	abstract class CBaseSerialCmd : public CObject
								  , public CCommand
	{
		DECLARE_DYNAMIC( CBaseSerialCmd )
	protected:
		CBaseSerialCmd( CommandType cmdType = CommandType() );
	public:
		// base overrides
		virtual void Serialize( CArchive& archive );
	public:
		static CLogger* s_pLogger;
		static IErrorObserver* s_pErrorObserver;
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


	// abstract base for commands that operate on multiple files
	//
	abstract class CBaseMultiFilesCmd : public CBaseSerialCmd
	{
	protected:
		CBaseMultiFilesCmd( CommandType cmdType = CommandType(), const std::vector< fs::CPath >& filePaths = std::vector< fs::CPath >(), const CTime& timestamp = CTime::GetCurrentTime() );
	public:
		const std::vector< fs::CPath >& GetFilePaths( void ) const { return m_filePaths; }
		const CTime& GetTimestamp( void ) const { return m_timestamp; }

		// base overrides
		virtual std::tstring Format( utl::Verbosity verbosity ) const;

		virtual void Serialize( CArchive& archive );
	protected:
		enum MultiFileStatus { AllExist, SomeExist, NoneExist };

		struct CWorkingSet
		{
			CWorkingSet( const CBaseMultiFilesCmd& cmd, fs::AccessMode accessMode = fs::Read );
		public:
			std::vector< fs::CPath > m_currFilePaths;
			std::vector< fs::CPath > m_badFilePaths;
			MultiFileStatus m_existStatus;
		};

		void HandleExecuteResult( bool succeeded, const CWorkingSet& workingSet, const std::tstring& details = str::GetEmpty() ) const;
	private:
		CTime m_timestamp;
		std::vector< fs::CPath > m_filePaths;
	};


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


	class CScopedLogger
	{
	public:
		CScopedLogger( CLogger* pLogger ) : m_pOldLogger( CBaseFileCmd::s_pLogger ) { CBaseFileCmd::s_pLogger = pLogger; }
		~CScopedLogger() { CBaseFileCmd::s_pLogger = m_pOldLogger; }
	private:
		CLogger* m_pOldLogger;
	};

	class CScopedErrorObserver
	{
	public:
		CScopedErrorObserver( IErrorObserver* pErrorObserver ) : m_pOldErrorObserver( CBaseFileCmd::s_pErrorObserver ) { CBaseFileCmd::s_pErrorObserver = pErrorObserver; }
		~CScopedErrorObserver() { CBaseFileCmd::s_pErrorObserver = m_pOldErrorObserver; }
	private:
		IErrorObserver* m_pOldErrorObserver;
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


// Used for removing duplicate files. Can be undone by undeleting the file from Recycle Bin.
//
class CDeleteFilesCmd : public cmd::CBaseMultiFilesCmd
{
	DECLARE_SERIAL( CDeleteFilesCmd )

	CDeleteFilesCmd( void ) {}
public:
	CDeleteFilesCmd( const std::vector< fs::CPath >& filePaths );
	virtual ~CDeleteFilesCmd();

	// ICommand interface
	virtual bool Execute( void );
	virtual bool Unexecute( void );
	virtual bool IsUndoable( void ) const;
private:
	struct CUndeleteFilesCmd : public cmd::CBaseMultiFilesCmd
	{
		CUndeleteFilesCmd( const std::vector< fs::CPath >& delFilePaths ) : cmd::CBaseMultiFilesCmd( cmd::Priv_UndeleteFiles, delFilePaths ) {}

		// ICommand interface
		virtual bool Execute( void );
		virtual bool IsUndoable( void ) const { return false; }
	};
};


class CFileModel;


abstract class CBaseChangeDestCmd : public CCommand
{
protected:
	CBaseChangeDestCmd( cmd::CommandType cmdType, CFileModel* pFileModel, const std::tstring& cmdTag );

	enum ChangeType { Changed, Unchanged, Expired };

	virtual ChangeType EvalChange( void ) const = 0;
	virtual bool ToggleExecute( void ) = 0;
public:
	// utl::IMessage overrides
	virtual std::tstring Format( utl::Verbosity verbosity ) const;			// override for special formatting

	// ICommand overrides
	virtual bool Execute( void );
	virtual bool Unexecute( void );
	virtual bool IsUndoable( void ) const;
private:
	std::tstring m_cmdTag;
protected:
	CFileModel* m_pFileModel;
	bool m_hasOldDests;						// m_destPaths holds the OldDestPaths; toggled each time on Execute()
};


class CChangeDestPathsCmd : public CBaseChangeDestCmd
{
public:
	CChangeDestPathsCmd( CFileModel* pFileModel, std::vector< fs::CPath >& rNewDestPaths, const std::tstring& cmdTag = std::tstring() );
private:
	// base overrides
	virtual ChangeType EvalChange( void ) const;
	virtual bool ToggleExecute( void );
private:
	std::vector< fs::CPath > m_srcPaths;
	std::vector< fs::CPath > m_destPaths;
};


class CChangeDestFileStatesCmd : public CBaseChangeDestCmd
{
public:
	CChangeDestFileStatesCmd( CFileModel* pFileModel, std::vector< fs::CFileState >& rNewDestStates, const std::tstring& cmdTag = std::tstring() );
private:
	// base overrides
	virtual ChangeType EvalChange( void ) const;
	virtual bool ToggleExecute( void );
private:
	std::vector< fs::CFileState > m_srcStates;
	std::vector< fs::CFileState > m_destStates;
};


class CResetDestinationsCmd : public CMacroCommand		// macro of 2 commands to reset RenameItems and TouchItems
{
public:
	CResetDestinationsCmd( CFileModel* pFileModel );

	// utl::IMessage overrides
	virtual std::tstring Format( utl::Verbosity verbosity ) const;			// override for special formatting
};


template< typename OptionsT >
class CEditOptionsCmd : public CObjectPropertyCommand< OptionsT, OptionsT >
{
public:
	CEditOptionsCmd( OptionsT* pDestOptions, const OptionsT& newOptions )
		: CObjectPropertyCommand< OptionsT, OptionsT >( cmd::EditOptions, pDestOptions, newOptions, &cmd::GetTags_CommandType() )
	{
	}
protected:
	// base overrides
	virtual bool DoExecute( void )
	{
		m_oldValue = *m_pObject;
		*m_pObject = m_value;

		m_pObject->PostApply();
		return true;
	}

	virtual bool Unexecute( void )
	{
		return CEditOptionsCmd( m_pObject, m_oldValue ).Execute();
	}
};


#endif // FileCommands_h
