#ifndef DocumentBase_h
#define DocumentBase_h
#pragma once

#include "Application_fwd.h"


class CDocumentBase : public CDocument
{
	DECLARE_DYNAMIC( CDocumentBase )
protected:
	CDocumentBase( void );
	virtual ~CDocumentBase();
public:
	template< typename ViewType >
	void UpdateAllViewsOfType( ViewType* pSenderView, int hint = 0, CObject* pHintObject = NULL )
	{
		// must have views if sent by one of them
		ASSERT( NULL == pSenderView || !m_viewList.IsEmpty() );
		for ( POSITION pos = GetFirstViewPosition(); pos != NULL; )
		{
			if ( ViewType* pView = dynamic_cast< ViewType* >( GetNextView( pos ) ) )
				if ( pView != pSenderView )
					pView->OnUpdate( pSenderView, hint, pHintObject );
		}
	}
public:
protected:
	// generated stuff
	DECLARE_MESSAGE_MAP()
};


#endif // DocumentBase_h
