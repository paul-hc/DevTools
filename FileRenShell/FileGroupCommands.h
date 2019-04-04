#ifndef FileGroupCommands_h
#define FileGroupCommands_h
#pragma once

#include "AppCommands.h"
#include "utl/Path.h"


namespace cmd
{
	// abstract base for commands that operate on multiple files
	//
	abstract class CBaseFileGroupCmd : public CBaseSerialCmd
	{
	protected:
		CBaseFileGroupCmd( CommandType cmdType = CommandType(), const std::vector< fs::CPath >& filePaths = std::vector< fs::CPath >(), const CTime& timestamp = CTime::GetCurrentTime() );
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
			CWorkingSet( const CBaseFileGroupCmd& cmd, fs::AccessMode accessMode = fs::Read );
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
		CUndeleteFilesCmd( const std::vector< fs::CPath >& delFilePaths ) : cmd::CBaseFileGroupCmd( cmd::Priv_UndeleteFiles, delFilePaths ) {}

		// ICommand interface
		virtual bool Execute( void );
		virtual bool IsUndoable( void ) const { return false; }
	};
};


#endif // FileGroupCommands_h
