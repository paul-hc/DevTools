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
	bool m_hasOldDests;				// m_destPaths holds the OldDestPaths; toggled each time on Execute()
};


class CRenameItem;


// editing rename items: change CRenameItem::m_destPath (doesn't do any file renaming)
//
class CChangeDestPathsCmd : public CBaseChangeDestCmd
{
public:
	CChangeDestPathsCmd( CFileModel* pFileModel, const std::vector<CRenameItem*>* pSelItems, const std::vector<fs::CPath>& newDestPaths, const std::tstring& cmdTag = std::tstring() );
	virtual ~CChangeDestPathsCmd();

	std::vector<CRenameItem*> MakeSelItems( void ) const;

	// cmd::IFileDetailsCmd
	virtual bool HasSelItems( void ) const override;		// true: acts on a a subset of rename items - false: acts on all m_pFileModel->GetRenameItems()
	virtual size_t GetFileCount( void ) const override { return m_srcPaths.size(); }
	virtual void QueryDetailLines( std::vector<std::tstring>& rLines ) const override;
private:
	// base overrides
	virtual ChangeType EvalChange( void ) const override;
	virtual bool ToggleExecute( void ) override;

	CRenameItem* GetRenameItemAt( size_t posSrcPath ) const;
private:
	std::vector<fs::CPath> m_srcPaths;
	std::vector<fs::CPath> m_destPaths;
};


namespace cmd
{
	CChangeDestPathsCmd* MakeResetSelDestPathsCmd( CFileModel* pFileModel, const std::vector<CRenameItem*>& selItems );
}


#include "utl/FileState.h"


class CTouchItem;


// editing file states: change CTouchItem::m_destState (doesn't do any file operations)
//
class CChangeDestFileStatesCmd : public CBaseChangeDestCmd
{
public:
	CChangeDestFileStatesCmd( CFileModel* pFileModel, const std::vector<CTouchItem*>* pSelItems, const std::vector<fs::CFileState>& newDestStates, const std::tstring& cmdTag = std::tstring() );

	std::vector<CTouchItem*> MakeSelItems( void ) const;

	// cmd::IFileDetailsCmd
	virtual bool HasSelItems( void ) const override;		// true: acts on a a subset of rename items - false: acts on all m_pFileModel->GetRenameItems()
	virtual size_t GetFileCount( void ) const override { return m_srcStates.size(); }
	virtual void QueryDetailLines( std::vector<std::tstring>& rLines ) const override;
private:
	// base overrides
	virtual ChangeType EvalChange( void ) const override;
	virtual bool ToggleExecute( void ) override;

	CTouchItem* GetTouchItemAt( size_t posSrcState ) const;
private:
	std::vector<fs::CFileState> m_srcStates;
	std::vector<fs::CFileState> m_destStates;
};


class CResetDestinationsCmd : public CMacroCommand		// macro of 2 commands to reset RenameItems and TouchItems
{
public:
	CResetDestinationsCmd( CFileModel* pFileModel );

	// utl::IMessage overrides
	virtual std::tstring Format( utl::Verbosity verbosity ) const override;			// override for special formatting
};


// misc. editing commands:

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


template< typename SubjectT >
abstract class CBaseRenamePageObjectCommand : public CObjectCommand<SubjectT>
{
protected:
	CBaseRenamePageObjectCommand( cmd::CommandType typeId, SubjectT* pObject, CBaseRenamePage* pPage )
		: CObjectCommand<SubjectT>( typeId, pObject, &cmd::GetTags_CommandType() )
		, m_pPage( pPage )
	{
		ASSERT_PTR( m_pObject );
	}

	// base overrides
	virtual bool Unexecute( void ) override { ASSERT( false ); return false; }
public:
	virtual bool Execute( void ) override;		// scoped page internal change during execution

	CBaseRenamePage* GetPage( void ) const { return m_pPage; }
private:
	CBaseRenamePage* m_pPage;					// page executing this command
};


#include "Application_fwd.h"


class CReportListControl;


class CSortRenameListCmd : public CBaseRenamePageObjectCommand<CFileModel>
{
public:
	CSortRenameListCmd( CFileModel* pFileModel, CReportListControl* pFileListCtrl, const ren::TSortingPair& sorting );
protected:
	// base overrides
	virtual bool DoExecute( void ) override;
private:
	CReportListControl* m_pFileListCtrl;		// the listCtrl that was just sorted by user (clicked on colum header)
public:
	const ren::TSortingPair m_sorting;
};


#include "utl/UI/LayoutBasePropertySheet.h"
#include "utl/UI/SelectionData.h"


class CRenameItem;


class COnRenameListSelChangedCmd : public CBaseRenamePageObjectCommand<CLayoutBasePropertySheet>
{
public:
	COnRenameListSelChangedCmd( CBaseRenamePage* pPage, const ui::CSelectionData<CRenameItem>& selData );
	COnRenameListSelChangedCmd( CLayoutBasePropertySheet* pChildSheet, const ui::CSelectionData<CRenameItem>& selData );

	const ui::CSelectionData<CRenameItem>& GetSelData( void ) const { return m_selData; }

	// debugging
	void dbgTraceSelData( const CBaseRenamePage* pPage ) const;
protected:
	// base overrides
	virtual bool DoExecute( void ) override;
	virtual bool Unexecute( void ) override { ASSERT( false ); return false; }
private:
	std::tstring GetUiTypeName( const CBaseRenamePage* pPage ) const;
private:
	ui::CSelectionData<CRenameItem> m_selData;
};


#endif // EditingCommands_h
