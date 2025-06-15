#ifndef FileMacroCommands_h
#define FileMacroCommands_h
#pragma once

#include "AppCommands.h"
#include "utl/AppTools.h"
#include "utl/Path.h"
#include "utl/RuntimeException.h"


namespace cmd
{
	class CBaseFileCmd;
	class CUserFeedbackException;
	typedef std::pair<std::tstring, app::MsgType> TMessagePair;

	enum UserFeedback { Abort, Retry, Ignore };


	class CFileMacroCmd : public CObject
		, public CMacroCommand
		, public IPersistentCmd
	{
		DECLARE_SERIAL( CFileMacroCmd )

		CFileMacroCmd( void ) {}
	public:
		CFileMacroCmd( CommandType subCmdType, const CTime& timestamp = CTime::GetCurrentTime() );

		// base overrides
		virtual std::tstring Format( utl::Verbosity verbosity ) const override;
		virtual bool Execute( void ) override;
		virtual bool Unexecute( void ) override;

		// cmd::IPersistentCmd
		virtual bool IsValid( void ) const override;
		virtual const CTime& GetTimestamp( void ) const override;

		// cmd::IFileDetailsCmd
		virtual size_t GetFileCount( void ) const override;
		virtual void QueryDetailLines( std::vector<std::tstring>& rLines ) const override;

		virtual void Serialize( CArchive& archive ) override;
	private:
		enum CmdMode { ExecuteMode, UnexecuteMode };

		void ExecuteMacro( CmdMode cmdMode );

		struct CExecMessage		// executes the leaf command (ExecuteMode) or reverse command (UnexecuteMode), and stores the aggregate balloon message
		{
			CExecMessage( void ) : m_msgType( app::Info ), m_pLeafCmd( nullptr ), m_cmdPos( 0 ) { m_message.reserve( 512 ); }

			CBaseFileCmd* StoreLeafCmd( CBaseFileCmd* pLeafCmd, CmdMode cmdMode );		// returns the target command
			void AppendLeafMessage( void );
		public:
			// aggregate balloon message and type
			std::tstring m_message;
			app::MsgType m_msgType;
		private:
			CBaseFileCmd* m_pLeafCmd;
			std::auto_ptr<CBaseFileCmd> m_pReverseLeafCmd;
			size_t m_cmdPos;
		};
	private:
		persist CTime m_timestamp;
	};


	// abstract base for file LEAF commands embedded in a CFileMacroCmd, that operates on a single file
	//
	abstract class CBaseFileCmd : public CBaseSerialCmd
	{
	protected:
		CBaseFileCmd( CommandType cmdType = CommandType(), const fs::CPath& srcPath = fs::CPath() );
	public:
		void ExecuteHandle( void ) throws_( CUserFeedbackException );

		const TMessagePair& GetExecMessage( void ) const { return m_execMessage; }
		void SetExecMessage( const TMessagePair& execMessage ) { m_execMessage = execMessage; }

		// base overrides
		virtual bool Unexecute( void ) override;
		virtual std::auto_ptr<CBaseFileCmd> MakeUnexecuteCmd( void ) = 0;
	protected:
		virtual void RecordMessage( const std::tstring& coreMessage, app::MsgType msgType ) override;
	private:
		UserFeedback HandleFileError( CException* pExc, const fs::CPath& srcPath );
		static std::tstring ExtractMessage( CException* pExc, const fs::CPath& srcPath );
	public:
		abstract persist fs::CPath m_srcPath;	// persistent for some commands
	private:
		// transient
		TMessagePair m_execMessage;				// stored for aggregate message in the macro
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
	virtual std::tstring Format( utl::Verbosity verbosity ) const override;
	virtual bool Execute( void ) override;
	virtual bool IsUndoable( void ) const override;
	virtual std::auto_ptr<CBaseFileCmd> MakeUnexecuteCmd( void ) override;

	// base overrides
	virtual void Serialize( CArchive& archive ) override;
public:
	persist fs::CPath m_destPath;
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
	virtual std::tstring Format( utl::Verbosity verbosity ) const override;
	virtual bool Execute( void ) override;
	virtual bool IsUndoable( void ) const override;
	virtual std::auto_ptr<CBaseFileCmd> MakeUnexecuteCmd( void ) override;

	// base overrides
	virtual void Serialize( CArchive& archive ) override;
public:
	persist fs::CFileState m_srcState;
	persist fs::CFileState m_destState;
};


#endif // FileMacroCommands_h
