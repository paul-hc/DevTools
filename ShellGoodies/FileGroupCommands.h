#ifndef FileGroupCommands_h
#define FileGroupCommands_h
#pragma once

#include "AppCommands.h"
#include "utl/AppTools.h"
#include "utl/Path.h"


typedef WORD FILEOP_FLAGS;		// defined in <shlwapi.h>


namespace cmd
{
	// abstract base for commands that operate on multiple files
	//
	abstract class CBaseFileGroupCmd
		: public CBaseSerialCmd
		, public IPersistentCmd
	{
	protected:
		CBaseFileGroupCmd( CommandType cmdType = CommandType(), const std::vector< fs::CPath >& filePaths = std::vector< fs::CPath >(), const CTime& timestamp = CTime::GetCurrentTime() );
	public:
		const std::vector< fs::CPath >& GetFilePaths( void ) const { return m_filePaths; }
		void SetOriginCmd( CBaseFileGroupCmd* pOriginCmd );

		CWnd* GetParentOwner( void ) const { return m_pParentOwner; }
		void SetParentOwner( CWnd* pParentOwner ) { m_pParentOwner = pParentOwner; }

		// base overrides
		virtual std::tstring Format( utl::Verbosity verbosity ) const override;
		virtual bool IsUndoable( void ) const override;

		// cmd::IPersistentCmd
		virtual bool IsValid( void ) const override;
		virtual const CTime& GetTimestamp( void ) const override;

		// cmd::IFileDetailsCmd
		virtual size_t GetFileCount( void ) const override;
		virtual void QueryDetailLines( std::vector< std::tstring >& rLines ) const override;

		virtual void Serialize( CArchive& archive ) override;
	protected:
		virtual std::tstring GetDestHeaderInfo( void ) const;

		bool AddActualCmdDetail( std::vector< std::tstring >& rLines ) const;

		struct CWorkingSet
		{
			CWorkingSet( const CBaseFileGroupCmd* pCmd, fs::AccessMode accessMode = fs::Read );
			CWorkingSet( const std::vector< fs::CPath >& srcFilePaths, const std::vector< fs::CPath >& destFilePaths, fs::AccessMode accessMode = fs::Read );

			bool IsValid( void ) const { return m_existStatus != app::Error; }
			bool IsBadFilePath( const fs::CPath& filePath ) const;

			// post-execution
			void QueryGroupDetails( std::tstring& rDetails, const CBaseFileGroupCmd* pCmd ) const;
			app::MsgType GetOutcomeMsgType( void ) const { return m_succeeded ? m_existStatus : app::Error; }
		public:
			std::vector< fs::CPath > m_currFilePaths;
			std::vector< fs::CPath > m_badFilePaths;
			std::vector< fs::CPath > m_currDestFilePaths;		// for commands that manage explicit destinations
			app::MsgType m_existStatus;
			bool m_succeeded;

			static const TCHAR s_lineEnd[];
		};

		struct HasValidPathPred
		{
			HasValidPathPred( const CWorkingSet* pWorkingSet ) : m_pWorkingSet( pWorkingSet ) { ASSERT_PTR( m_pWorkingSet ); }

			bool operator()( const std::tstring& detailLine ) const;		// detail line does not contain any bad path?
		private:
			const CWorkingSet* m_pWorkingSet;
		};

		bool HandleExecuteResult( const CWorkingSet& workingSet );
	private:
		persist CTime m_timestamp;
		persist std::vector< fs::CPath > m_filePaths;

		// transient data-members
		CWnd* m_pParentOwner;
	protected:
		fs::AccessMode m_fileAccessMode;
	public:
		FILEOP_FLAGS m_opFlags;
	};


	// abstract base for commands that operate deep transfers of multiple files
	//
	abstract class CBaseDeepTransferFilesCmd : public CBaseFileGroupCmd
	{
	protected:
		CBaseDeepTransferFilesCmd( void ) {}
		CBaseDeepTransferFilesCmd( CommandType cmdType, const std::vector< fs::CPath >& srcFilePaths, const fs::TDirPath& destDirPath );

		template< typename CmdT >
		static CmdT* SetCommandType( CmdT* pNewCmd, CommandType cmdType )
		{
			ASSERT_PTR( pNewCmd );
			pNewCmd->SetTypeID( cmdType );
			return pNewCmd;
		}
	public:
		const std::vector< fs::CPath >& GetSrcFilePaths( void ) const { return GetFilePaths(); }
		const fs::TDirPath& GetSrcCommonDirPath( void ) const { return m_srcCommonDirPath; }
		const fs::TDirPath& GetDestDirPath( void ) const { return m_destDirPath; }
		const fs::TDirPath& GetTopDestDirPath( void ) const { return m_topDestDirPath; }		// root for post-move cleanup

		void SetDeepRelDirPath( const fs::TDirPath& deepRelSubfolderPath );		// for deeper transfers
		bool QueryDestFilePaths( std::vector< fs::CPath >& rDestFilePaths ) const;

		// base overrides
		virtual void Serialize( CArchive& archive ) override;
		virtual void QueryDetailLines( std::vector< std::tstring >& rLines ) const override;

		// ICommand interface
	protected:
		virtual std::tstring GetDestHeaderInfo( void ) const override;

		void MakeDestFilePaths( std::vector< fs::CPath >& rDestFilePaths, const std::vector< fs::CPath >& srcFilePaths ) const;
		static void AppendFilePairLines( std::vector< std::tstring >& rLines, const std::vector< fs::CPath >& srcFilePaths, const std::vector< fs::CPath >& destFilePaths );
	private:
		fs::CPath MakeDeepDestFilePath( const fs::CPath& srcFilePath ) const;
	private:
		persist fs::TDirPath m_destDirPath;
		persist fs::TDirPath m_srcCommonDirPath;
		persist fs::TDirPath m_topDestDirPath;		// parent folder of m_destDirPath (if using extra sub-folder depth), otherwise same as m_destDirPath
	};


	// Abstract base for commands that operate shallow transfers of multiple files - such as Backup-Copy/Move.
	// Keeps track of destination paths explicitly (unlike implicitly for Deep commands).
	//
	abstract class CBaseShallowTransferFilesCmd : public CBaseFileGroupCmd
	{
	protected:
		CBaseShallowTransferFilesCmd( void ) {}
		CBaseShallowTransferFilesCmd( CommandType cmdType, const std::vector< fs::CPath >& srcFilePaths, const fs::TDirPath& destDirPath, const std::vector< fs::CPath >* pDestFilePaths = nullptr );
	public:
		const std::vector< fs::CPath >& GetSrcFilePaths( void ) const { return GetFilePaths(); }
		const std::vector< fs::CPath >& GetDestFilePaths( void ) const { return m_destFilePaths; }
		const fs::TDirPath& GetDestDirPath( void ) const { return m_destDirPath; }

		bool QueryDestFilePaths( std::vector< fs::CPath >& rDestFilePaths ) const;

		// base overrides
		virtual void Serialize( CArchive& archive ) override;
		virtual void QueryDetailLines( std::vector< std::tstring >& rLines ) const override;

		// ICommand interface
	protected:
		virtual std::tstring GetDestHeaderInfo( void ) const override;

		void MakeDestFilePaths( std::vector< fs::CPath >& rDestFilePaths, const std::vector< fs::CPath >& srcFilePaths ) const;

		// overridables
		virtual fs::CPath DoMakeDestFilePath( const fs::CPath& srcFilePath ) const = 0;

		fs::CPath MakeShallowDestFilePath( const fs::CPath& srcFilePath ) const;		// straight shallow SRC/DEST
		fs::CPath MakeBackupDestFilePath( const fs::CPath& srcFilePath ) const;			// backup SRC/DEST
		static void AppendFilePairLines( std::vector< std::tstring >& rLines, const std::vector< fs::CPath >& srcFilePaths, const std::vector< fs::CPath >& destFilePaths );
	protected:
		persist fs::TDirPath m_destDirPath;
		persist std::vector< fs::CPath > m_destFilePaths;
	};
}


// Used for removing duplicate files. Can be undone by undeleting the file from Recycle Bin.
//
class CDeleteFilesCmd : public cmd::CBaseFileGroupCmd
{
	DECLARE_SERIAL( CDeleteFilesCmd )

	CDeleteFilesCmd( void ) {}
public:
	CDeleteFilesCmd( const std::vector< fs::CPath >& filePaths );
	virtual ~CDeleteFilesCmd();

	// ICommand interface
	virtual bool Execute( void ) override;
	virtual bool Unexecute( void ) override;
private:
	struct CUndeleteFilesCmd : public cmd::CBaseFileGroupCmd
	{
		CUndeleteFilesCmd( const std::vector< fs::CPath >& delFilePaths )
			: cmd::CBaseFileGroupCmd( cmd::Priv_UndeleteFiles, delFilePaths )
		{
			m_fileAccessMode = fs::Exist;
		}

		// ICommand interface
		virtual bool Execute( void ) override;
		virtual bool IsUndoable( void ) const override { return false; }
	};
};


// Used for copying files, using a deep destination directory structure.
//
class CCopyFilesCmd : public cmd::CBaseDeepTransferFilesCmd
{
	DECLARE_SERIAL( CCopyFilesCmd )

	CCopyFilesCmd( void ) {}
public:
	CCopyFilesCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::TDirPath& destDirPath );
	virtual ~CCopyFilesCmd();

	static CCopyFilesCmd* MakePasteCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::TDirPath& destDirPath );

	// ICommand interface
	virtual bool Execute( void ) override;
	virtual bool Unexecute( void ) override;
};


// Used for moving files, using a deep destination directory structure.
//
class CMoveFilesCmd : public cmd::CBaseDeepTransferFilesCmd
{
	DECLARE_SERIAL( CMoveFilesCmd )

	CMoveFilesCmd( void ) {}
public:
	CMoveFilesCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::TDirPath& destDirPath );
	virtual ~CMoveFilesCmd();

	static CMoveFilesCmd* MakePasteCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::TDirPath& destDirPath );

	// ICommand interface
	virtual bool Execute( void ) override;
	virtual bool Unexecute( void ) override;
};


// Used for copying files, using a deep destination directory structure.
//
class CCreateFoldersCmd : public cmd::CBaseDeepTransferFilesCmd
{
	DECLARE_SERIAL( CCreateFoldersCmd )

	CCreateFoldersCmd( void ) {}
public:
	enum Structure { CreateNormal, PasteDirs, PasteDeepStruct };

	CCreateFoldersCmd( const std::vector< fs::CPath >& srcFolderPaths, const fs::TDirPath& destDirPath, Structure structure = CreateNormal );
	virtual ~CCreateFoldersCmd();

	const std::vector< fs::CPath >& GetSrcFolderPaths( void ) const { return GetSrcFilePaths(); }

	// ICommand interface
	virtual bool Execute( void ) override;
	virtual bool Unexecute( void ) override;
};


// Used for creating backup copies of files into a flat destination directory.
//
class CCopyPasteFilesAsBackupCmd : public cmd::CBaseShallowTransferFilesCmd
{
	DECLARE_SERIAL( CCopyPasteFilesAsBackupCmd )

	CCopyPasteFilesAsBackupCmd( void ) {}
public:
	CCopyPasteFilesAsBackupCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::TDirPath& destDirPath );
	virtual ~CCopyPasteFilesAsBackupCmd();

	// ICommand interface
	virtual bool Execute( void ) override;
	virtual bool Unexecute( void ) override;
protected:
	// base overrides
	virtual fs::CPath DoMakeDestFilePath( const fs::CPath& srcFilePath ) const override { return MakeBackupDestFilePath( srcFilePath ); }
};


// Used for moving backup files into a flat destination directory.
//
class CCutPasteFilesAsBackupCmd : public cmd::CBaseShallowTransferFilesCmd
{
	DECLARE_SERIAL( CCutPasteFilesAsBackupCmd )

	CCutPasteFilesAsBackupCmd( void ) {}
	CCutPasteFilesAsBackupCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::TDirPath& destDirPath, const std::vector< fs::CPath >& destFilePaths );	// only for unexecution
public:
	CCutPasteFilesAsBackupCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::TDirPath& destDirPath );
	virtual ~CCutPasteFilesAsBackupCmd();

	// ICommand interface
	virtual bool Execute( void ) override;
	virtual bool Unexecute( void ) override;
protected:
	// base overrides
	virtual fs::CPath DoMakeDestFilePath( const fs::CPath& srcFilePath ) const override { return MakeBackupDestFilePath( srcFilePath ); }
};


#endif // FileGroupCommands_h
