#ifndef EditingCommands_h
#define EditingCommands_h
#pragma once

#include "AppCommands_fwd.h"


class CFileModel;


abstract class CBaseChangeDestCmd : public CCommand
	, public cmd::IFileDetailsCmd
{
protected:
	CBaseChangeDestCmd( cmd::CommandType cmdType, CFileModel* pFileModel, const std::tstring& cmdTag );

	enum ChangeType { Changed, Unchanged, Expired };

	virtual ChangeType EvalChange( void ) const = 0;
	virtual bool ToggleExecute( void ) = 0;
public:
	// utl::IMessage overrides
	virtual std::tstring Format( utl::Verbosity verbosity ) const override;			// override for special formatting

	// ICommand overrides
	virtual bool Execute( void ) override;
	virtual bool Unexecute( void ) override;
	virtual bool IsUndoable( void ) const override;
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
	virtual ChangeType EvalChange( void ) const override;
	virtual bool ToggleExecute( void ) override;
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
	virtual ChangeType EvalChange( void ) const override;
	virtual bool ToggleExecute( void ) override;
private:
	std::vector< fs::CFileState > m_srcStates;
	std::vector< fs::CFileState > m_destStates;
};


class CResetDestinationsCmd : public CMacroCommand		// macro of 2 commands to reset RenameItems and TouchItems
{
public:
	CResetDestinationsCmd( CFileModel* pFileModel );

	// utl::IMessage overrides
	virtual std::tstring Format( utl::Verbosity verbosity ) const override;			// override for special formatting
};


template< typename OptionsT >
class CEditOptionsCmd : public CObjectPropertyCommand<OptionsT, OptionsT>
{
public:
	CEditOptionsCmd( OptionsT* pDestOptions, const OptionsT& newOptions )
		: CObjectPropertyCommand<OptionsT, OptionsT>( cmd::EditOptions, pDestOptions, newOptions, &cmd::GetTags_CommandType() )
	{
	}
protected:
	// base overrides
	virtual bool DoExecute( void ) override
	{
		m_oldValue = *m_pObject;
		*m_pObject = m_value;

		m_pObject->PostApply();
		return true;
	}

	virtual bool Unexecute( void ) override
	{
		return CEditOptionsCmd( m_pObject, m_oldValue ).Execute();
	}
};


class CBaseRenamePage;


template< typename SubjectType >
abstract class CBaseRenamePageObjectCommand : public CObjectCommand<SubjectType>
{
protected:
	CBaseRenamePageObjectCommand( cmd::CommandType typeId, SubjectType* pObject, CBaseRenamePage* pPage )
		: CObjectCommand<SubjectType>( typeId, pObject, &cmd::GetTags_CommandType() )
		, m_pPage( pPage )
	{
		ASSERT_PTR( m_pObject );
	}
protected:
	// base overrides
	virtual bool Unexecute( void ) override { ASSERT( false ); return false; }
public:
	virtual bool Execute( void ) override;		// scoped page internal change during execution
public:
	CBaseRenamePage* m_pPage;				// page executing this command
};


#include "Application_fwd.h"


class CReportListControl;


class CSortRenameItemsCmd : public CBaseRenamePageObjectCommand<CFileModel>
{
public:
	CSortRenameItemsCmd( CFileModel* pFileModel, CReportListControl* pFileListCtrl, const ren::TSortingPair& sorting );
protected:
	// base overrides
	virtual bool DoExecute( void ) override;
private:
	CReportListControl* m_pFileListCtrl;		// the listCtrl that was just sorted by user (clicked on colum header)
public:
	const ren::TSortingPair m_sorting;
};


#include "utl/UI/LayoutBasePropertySheet.h"


class CRenameItem;


class COnRenameListSelChangedCmd : public CBaseRenamePageObjectCommand<CLayoutBasePropertySheet>
{
public:
	COnRenameListSelChangedCmd( CBaseRenamePage* pPage, CRenameItem* pSelItem );
protected:
	// base overrides
	virtual bool DoExecute( void ) override { return true; }
	virtual bool Unexecute( void ) override { ASSERT( false ); return false; }
public:
	static CRenameItem* s_pSelItem;		// static so that it stores the shared selected item, for new page initialization on creation; pointer since is invariant to sorting
};


#endif // EditingCommands_h
