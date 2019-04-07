#ifndef AppCommands_h
#define AppCommands_h
#pragma once

#include "utl/Command.h"


class CEnumTags;
namespace fs { class CPath; }


namespace cmd
{
	enum CommandType
	{
		RenameFile = 100, TouchFile, FindDuplicates,
		DeleteFiles, MoveFiles,
		ChangeDestPaths, ChangeDestFileStates, ResetDestinations,
		EditOptions,

		Priv_UndeleteFiles
	};

	const CEnumTags& GetTags_CommandType( void );


	interface IFileDetailsCmd
	{
		virtual size_t GetFileCount( void ) const = 0;
		virtual void QueryDetailLines( std::vector< std::tstring >& rLines ) const = 0;
	};

	interface IPersistentCmd : public IFileDetailsCmd
	{
		virtual bool IsValid( void ) const = 0;
		virtual const CTime& GetTimestamp( void ) const = 0;
	};

	interface IErrorObserver
	{
		virtual void ClearFileErrors( void ) = 0;
		virtual void OnFileError( const fs::CPath& srcPath, const std::tstring& errMsg ) = 0;
	};


	enum FileFormat { TextFormat, BinaryFormat };


	bool IsPersistentCmd( const utl::ICommand* pCmd );		// some persistent commands are also editor-specific file action commands
	bool IsZombieCmd( const utl::ICommand* pCmd );			// empty macro file action command with no effect?
	bool StripTimestamp( std::tstring& rText, const utl::ICommand* pCmd );
}


namespace pred
{
	struct IsPersistentCmd
	{
		typedef utl::ICommand* argument_type;		// required by std::not1()

		bool operator()( const utl::ICommand* pCmd ) const
		{
			return cmd::IsPersistentCmd( pCmd );
		}
	};

	struct IsZombieCmd
	{
		bool operator()( const utl::ICommand* pCmd ) const
		{
			return cmd::IsZombieCmd( pCmd );
		}
	};
}


class CLogger;


namespace cmd
{
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


	class CScopedLogger
	{
	public:
		CScopedLogger( CLogger* pLogger ) : m_pOldLogger( CBaseSerialCmd::s_pLogger ) { CBaseSerialCmd::s_pLogger = pLogger; }
		~CScopedLogger() { CBaseSerialCmd::s_pLogger = m_pOldLogger; }
	private:
		CLogger* m_pOldLogger;
	};

	class CScopedErrorObserver
	{
	public:
		CScopedErrorObserver( IErrorObserver* pErrorObserver ) : m_pOldErrorObserver( CBaseSerialCmd::s_pErrorObserver ) { CBaseSerialCmd::s_pErrorObserver = pErrorObserver; }
		~CScopedErrorObserver() { CBaseSerialCmd::s_pErrorObserver = m_pOldErrorObserver; }
	private:
		IErrorObserver* m_pOldErrorObserver;
	};
}


#endif // AppCommands_h
