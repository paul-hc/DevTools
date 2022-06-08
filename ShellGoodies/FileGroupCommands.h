#ifndef FileGroupCommands_h
#define FileGroupCommands_h
#pragma once

#include "AppCommands.h"
#include "utl/ContainerUtilities.h"
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
		void CopyTimestampOf( const CBaseFileGroupCmd& srcCmd ) { m_timestamp = srcCmd.GetTimestamp(); }

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
		static void QueryFilePairLines( std::vector< std::tstring >& rLines, const std::vector< fs::CPath >& srcFilePaths, const std::vector< fs::CPath >& destFilePaths );

		enum MultiFileStatus { AllExist, SomeExist, NoneExist };

		struct CWorkingSet
		{
			CWorkingSet( const CBaseFileGroupCmd& cmd, fs::AccessMode accessMode = fs::Read );

			bool IsValid( void ) const { return m_existStatus != NoneExist; }
			bool IsBadFilePath( const fs::CPath& filePath ) const { return !utl::Contains( m_badFilePaths, filePath ); }
		public:
			std::vector< fs::CPath > m_currFilePaths;
			std::vector< fs::CPath > m_badFilePaths;
			MultiFileStatus m_existStatus;
			bool m_succeeded;
		};

		bool HandleExecuteResult( const CWorkingSet& workingSet, const std::tstring& groupDetails );
	private:
		persist CTime m_timestamp;
		persist std::vector< fs::CPath > m_filePaths;
		CWnd* m_pParentOwner;
	public:
		FILEOP_FLAGS m_opFlags;
		static const TCHAR s_lineEnd[];
	};


	// abstract base for commands that operate deep transfers of multiple files
	//
	abstract class CBaseDeepTransferFilesCmd : public CBaseFileGroupCmd
	{
	protected:
		CBaseDeepTransferFilesCmd( void ) {}
		CBaseDeepTransferFilesCmd( CommandType cmdType, const std::vector< fs::CPath >& srcFilePaths, const fs::CPath& destDirPath );
	public:
		const std::vector< fs::CPath >& GetSrcFilePaths( void ) const { return GetFilePaths(); }
		const fs::CPath& GetSrcCommonDirPath( void ) const { return m_srcCommonDirPath; }
		const fs::CPath& GetDestDirPath( void ) const { return m_destDirPath; }
		const fs::CPath& GetTopDestDirPath( void ) const { return m_topDestDirPath; }		// root for post-move cleanup

		void SetDeepRelDirPath( const fs::CPath& deepRelSubfolderPath );		// for deeper transfers

		// base overrides
		virtual void Serialize( CArchive& archive ) override;
		virtual void QueryDetailLines( std::vector< std::tstring >& rLines ) const override;

		// ICommand interface
	protected:
		virtual std::tstring GetDestHeaderInfo( void ) const override;

		void MakeDestFilePaths( std::vector< fs::CPath >& rDestFilePaths, const std::vector< fs::CPath >& srcFilePaths ) const;
		fs::CPath MakeDeepDestFilePath( const fs::CPath& srcFilePath ) const;
	private:
		persist fs::CPath m_srcCommonDirPath;
		persist fs::CPath m_destDirPath;
		persist fs::CPath m_topDestDirPath;		// parent folder of m_destDirPath (if using extra sub-folder depth), otherwise same as m_destDirPath
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
			: cmd::CBaseFileGroupCmd( cmd::Priv_UndeleteFiles, delFilePaths ) {}

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
	CCopyFilesCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::CPath& destDirPath, bool isPaste = false );
	virtual ~CCopyFilesCmd();

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
	CMoveFilesCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::CPath& destDirPath, bool isPaste = false );
	virtual ~CMoveFilesCmd();

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

	CCreateFoldersCmd( const std::vector< fs::CPath >& srcFolderPaths, const fs::CPath& destDirPath, Structure structure = CreateNormal );
	virtual ~CCreateFoldersCmd();

	const std::vector< fs::CPath >& GetSrcFolderPaths( void ) const { return GetSrcFilePaths(); }

	// ICommand interface
	virtual bool Execute( void ) override;
	virtual bool Unexecute( void ) override;
};


#endif // FileGroupCommands_h
