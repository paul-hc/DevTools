#ifndef EditingCommands_h
#define EditingCommands_h
#pragma once

#include "AppCommands.h"


class CFileModel;


abstract class CBaseChangeDestCmd
	: public CCommand
	, public cmd::IFileDetailsCmd
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

	// cmd::IFileDetailsCmd
	virtual size_t GetFileCount( void ) const;
	virtual void QueryDetailLines( std::vector< std::tstring >& rLines ) const;
private:
	// base overrides
	virtual ChangeType EvalChange( void ) const;
	virtual bool ToggleExecute( void );
private:
	std::vector< fs::CPath > m_srcPaths;
	std::vector< fs::CPath > m_destPaths;
};


#include "utl/FileState.h"


class CChangeDestFileStatesCmd : public CBaseChangeDestCmd
{
public:
	CChangeDestFileStatesCmd( CFileModel* pFileModel, std::vector< fs::CFileState >& rNewDestStates, const std::tstring& cmdTag = std::tstring() );

	// cmd::IFileDetailsCmd
	virtual size_t GetFileCount( void ) const;
	virtual void QueryDetailLines( std::vector< std::tstring >& rLines ) const;
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


#endif // EditingCommands_h
