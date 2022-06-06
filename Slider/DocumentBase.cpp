
#include "stdafx.h"
#include "DocumentBase.h"
#include "FileOperation.h"
#include "resource.h"
#include "utl/UI/ShellDialogs.h"
#include "utl/UI/ShellUtilities.h"
#include "utl/UI/WndUtils.h"
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

CWicImage* CDocumentBase::AcquireImage( const fs::TImagePathKey& imageKey )
{
	if ( !imageKey.first.IsEmpty() )
		return CWicImageCache::Instance().Acquire( imageKey ).first;

	return NULL;
}


std::vector< fs::CPath > CDocumentBase::s_destFilePaths;		// used in image operations; concrete documents may refer to it

bool CDocumentBase::HandleDeleteImages( const std::vector< fs::CFlexPath >& selFilePaths )
{
	std::vector< fs::CPath > physicalPaths;
	if ( path::QueryPhysicalPaths( physicalPaths, selFilePaths ) )
	{
		if ( !ui::IsKeyPressed( VK_SHIFT ) )
		{
			std::tstring message = str::Format( _T("Delete %d image files?"), physicalPaths.size() );

			if ( physicalPaths.size() != selFilePaths.size() )
			{
				message += _T("\n\n");
				message += str::Join( physicalPaths, _T("\n") );
			}

			if ( ui::MessageBox( message, MB_ICONQUESTION | MB_OKCANCEL ) != IDOK )
				return false;
		}

		shell::DeleteFiles( physicalPaths );
	}
	return true;
}

bool CDocumentBase::HandleMoveImages( const std::vector< fs::CFlexPath >& srcFilePaths )
{
	REQUIRE( !srcFilePaths.empty() );

	fs::TDirPath destFolderPath;
	if ( !shell::PickFolder( destFolderPath, NULL, 0, _T("Select Destination Folder") ) )
		return false;

	s_destFilePaths.clear();
	svc::MakeDestFilePaths( s_destFilePaths, srcFilePaths, destFolderPath, Shallow );

	if ( !svc::CheckOverrideExistingFiles( s_destFilePaths ) )
		return false;

	// move a mix of physical (MOVE) and complex (COPY) paths
	size_t count = svc::RelocateFiles( srcFilePaths, s_destFilePaths, Shallow );

	if ( 0 == count && !srcFilePaths.empty() )
		return false;				// user decided to prevent overriding duplicates

	if ( count != srcFilePaths.size() )
		ui::ReportError( str::Format( _T("%d out of %d files encountered a move issue!"), srcFilePaths.size() - count, srcFilePaths.size() ), MB_ICONWARNING );

	return true;
}


// commands

BEGIN_MESSAGE_MAP( CDocumentBase, CDocument )
	ON_COMMAND( ID_IMAGE_EXPLORE, On_ImageExplore )
	ON_UPDATE_COMMAND_UI( ID_IMAGE_EXPLORE, OnUpdate_ReadImageSingleFile )
END_MESSAGE_MAP()

void CDocumentBase::On_ImageExplore( void )
{
	if ( CWicImage* pImage = GetCurrentImage() )
		shell::ExploreAndSelectFile( pImage->GetImagePath().GetPhysicalPath().GetPtr() );
}

void CDocumentBase::OnUpdate_ReadImageSingleFile( CCmdUI* pCmdUI )
{
	std::vector< fs::CFlexPath > selImagePaths;
	pCmdUI->Enable( QuerySelectedImagePaths( selImagePaths ) && 1 == selImagePaths.size() && selImagePaths.front().FileExist() );
}
