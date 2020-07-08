#ifndef DocumentBase_h
#define DocumentBase_h
#pragma once

#include "utl/UI/ImagePathKey.h"
#include "Application_fwd.h"


class CWicImage;


class CDocumentBase : public CDocument
{
	DECLARE_DYNAMIC( CDocumentBase )
protected:
	CDocumentBase( void );
	virtual ~CDocumentBase();

	virtual CWicImage* GetCurrentImage( void ) const = 0;
	virtual bool QuerySelectedImagePaths( std::vector< fs::CFlexPath >& rSelImagePaths ) const = 0;
public:
	fs::CPath GetDocFilePath( void ) const { return fs::CPath( GetPathName().GetString() ); }

	static CWicImage* AcquireImage( const fs::ImagePathKey& imageKey );

	template< typename ViewT >
	void UpdateAllViewsOfType( ViewT* pSenderView, int hint = 0, CObject* pHintObject = NULL )
	{
		// must have views if sent by one of them
		ASSERT( NULL == pSenderView || !m_viewList.IsEmpty() );
		for ( POSITION pos = GetFirstViewPosition(); pos != NULL; )
		{
			if ( ViewT* pView = dynamic_cast< ViewT* >( GetNextView( pos ) ) )
				if ( pView != pSenderView )
					pView->OnUpdate( pSenderView, hint, pHintObject );
		}
	}

	// generated stuff
protected:
	afx_msg void On_ImageExplore( void );
	afx_msg void OnUpdate_ImageSingleFileReadOp( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // DocumentBase_h
