
#include "stdafx.h"
#include "Clipboard.h"
#include "FileSystem.h"
#include "utl/StringUtilities.h"

// for CF_HDROP utils
#include "OleUtils.h"
#include "ShellUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const CLIPFORMAT CClipboard::s_cfPreferredDropEffect = static_cast<CLIPFORMAT>( ::RegisterClipboardFormat( CFSTR_PREFERREDDROPEFFECT ) );

CClipboard::CClipboard( CWnd* pWnd )
	: CTextClipboard( pWnd->GetSafeHwnd() )
{
}

CClipboard* CClipboard::Open( CWnd* pWnd /*= AfxGetMainWnd()*/ )
{
	ASSERT_PTR( pWnd->GetSafeHwnd() );
	return pWnd->OpenClipboard() ? new CClipboard( pWnd ) : NULL;
}


bool CClipboard::HasDropFiles( void )
{
	return ::IsClipboardFormatAvailable( CF_HDROP ) != FALSE;
}

DROPEFFECT CClipboard::QueryDropFilePaths( std::vector< fs::CPath >& rSrcPaths )
{
	CComPtr<IDataObject> pDataObject;

	if ( ::IsClipboardFormatAvailable( CF_HDROP ) )
		if ( SUCCEEDED( ::OleGetClipboard( &pDataObject ) ) )
		{
			FORMATETC format = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
			STGMEDIUM stgMedium = { 0 };	// defend against buggy data object

			if ( SUCCEEDED( pDataObject->GetData( &format, &stgMedium ) ) )		// transfer the data
			{
				HDROP hDropInfo = (HDROP)stgMedium.hGlobal;
				shell::QueryDroppedFiles( rSrcPaths, hDropInfo );
			}

			return ::IsClipboardFormatAvailable( s_cfPreferredDropEffect )
				? ole_utl::GetValueDWord( pDataObject, CFSTR_PREFERREDDROPEFFECT )		// Copy or Paste?
				: DROPEFFECT_COPY;
		}

	return DROPEFFECT_NONE;
}

bool CClipboard::AlsoCopyDropFilesAsPaths( CWnd* pParentOwner )
{
	if ( CanPasteText() )
		return false;				// avoid overriding existing text

	std::vector< fs::CPath > srcPaths;
	QueryDropFilePaths( srcPaths );

	if ( srcPaths.empty() )
		return false;				// no CF_HDROP stored on clipboard

	CopyToLines( srcPaths, pParentOwner->GetSafeHwnd(), CClipboard::s_lineEnd, false );		// keep all the file transfer (Copy, Cut) clipboard data, and add the source paths as text
	return true;
}
