
#include "stdafx.h"
#include "DocumentBase.h"
#include "resource.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/WicImageCache.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC( CDocumentBase, CDocument )

CDocumentBase::CDocumentBase( void )
	: CDocument()
{
}

CDocumentBase::~CDocumentBase()
{
}

CWicImage* CDocumentBase::AcquireImage( const fs::ImagePathKey& imageKey )
{
	if ( !imageKey.first.IsEmpty() )
		return CWicImageCache::Instance().Acquire( imageKey ).first;

	return NULL;
}


// commands

BEGIN_MESSAGE_MAP( CDocumentBase, CDocument )
	ON_COMMAND( ID_IMAGE_EXPLORE, On_ImageExplore )
	ON_UPDATE_COMMAND_UI( ID_IMAGE_EXPLORE, OnUpdate_ImageSingleFileReadOp )
END_MESSAGE_MAP()

void CDocumentBase::On_ImageExplore( void )
{
	if ( CWicImage* pImage = GetCurrentImage() )
		shell::ExploreAndSelectFile( pImage->GetImagePath().GetPhysicalPath().GetPtr() );
}

void CDocumentBase::OnUpdate_ImageSingleFileReadOp( CCmdUI* pCmdUI )
{
	std::vector< fs::CFlexPath > selImagePaths;
	pCmdUI->Enable( QuerySelectedImagePaths( selImagePaths ) && 1 == selImagePaths.size() && selImagePaths.front().FileExist() );
}
