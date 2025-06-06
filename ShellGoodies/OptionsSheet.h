#ifndef OptionsSheet_h
#define OptionsSheet_h
#pragma once

#include "utl/UI/LayoutPropertySheet.h"


class CFileModel;
interface IFileEditor;


class COptionsSheet : public CLayoutPropertySheet
{
public:
	COptionsSheet( CFileModel* pFileModel, CWnd* pParent, UINT initialPageIndex = UINT_MAX );

	enum PageIndex { GeneralPage, CapitalizePage };

	CFileModel* GetFileModel( void ) const { return m_pFileModel; }
protected:
	// base overrides
	virtual void OnChangesApplied( void );
private:
	CFileModel* m_pFileModel;
	IFileEditor* m_pFileEditor;
};


#endif // OptionsSheet_h
