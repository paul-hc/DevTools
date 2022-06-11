#ifndef EditingCommands_hxx
#define EditingCommands_hxx

#include "RenamePages.h"	// CBaseRenamePage class


template< typename SubjectType >
bool CBaseRenamePageObjectCommand<SubjectType>::Execute( void ) override
{
	CScopedInternalChange pageChanging( m_pPage );

	return __super::Execute();
}


#endif // EditingCommands_hxx
