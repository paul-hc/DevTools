#ifndef FileGroupCommands_h
#define FileGroupCommands_h
#pragma once

#include "AppCommands.h"
#include "utl/ContainerUtilities.h"
#include "utl/Path.h"


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

		// base overrides
		virtual std::tstring Format( utl::Verbosity verbosity ) const;

		// cmd::IPersistentCmd
		virtual bool IsValid( void ) const;
		virtual const CTime& GetTimestamp( void ) const;

		// cmd::IFileDetailsCmd
		virtual size_t GetFileCount( void ) const;
		virtual void QueryDetailLines( std::vector< std::tstring >& rLines ) const;

		virtual void Serialize( CArchive& archive );
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
	protected:
		static const TCHAR s_lineEnd[];
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
	virtual bool Execute( void );
	virtual bool Unexecute( void );
	virtual bool IsUndoable( void ) const;
private:
	struct CUndeleteFilesCmd : public cmd::CBaseFileGroupCmd
	{
		CUndeleteFilesCmd( const std::vector< fs::CPath >& delFilePaths )
			: cmd::CBaseFileGroupCmd( cmd::Priv_UndeleteFiles, delFilePaths ) {}

		// ICommand interface
		virtual bool Execute( void );
		virtual bool IsUndoable( void ) const { return false; }
	};
};


// Used for moving files (usually duplicates), using a deep destination directory structure.
//
class CMoveFilesCmd : public cmd::CBaseFileGroupCmd
{
	DECLARE_SERIAL( CMoveFilesCmd )

	CMoveFilesCmd( void ) {}
public:
	CMoveFilesCmd( const std::vector< fs::CPath >& srcFilePaths, const fs::CPath& destDirPath );
	virtual ~CMoveFilesCmd();

	const std::vector< fs::CPath >& GetSrcFilePaths( void ) const { return GetFilePaths(); }
	const fs::CPath& GetSrcCommonDirPath( void ) const { return m_srcCommonDirPath; }
	const fs::CPath& GetDestDirPath( void ) const { return m_destDirPath; }

	// base overrides
	virtual void Serialize( CArchive& archive );
	virtual void QueryDetailLines( std::vector< std::tstring >& rLines ) const;

	// ICommand interface
	virtual bool Execute( void );
	virtual bool Unexecute( void );
	virtual bool IsUndoable( void ) const;
protected:
	virtual std::tstring GetDestHeaderInfo( void ) const;

	void MakeDestFilePaths( std::vector< fs::CPath >& rDestFilePaths, const std::vector< fs::CPath >& srcFilePaths ) const;
	fs::CPath MakeDeepDestFilePath( const fs::CPath& srcFilePath ) const;
private:
	persist fs::CPath m_srcCommonDirPath;
	persist fs::CPath m_destDirPath;
};


#endif // FileGroupCommands_h
