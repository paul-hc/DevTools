#ifndef Clipboard_h
#define Clipboard_h
#pragma once

#include "utl/TextClipboard.h"


class CClipboard : public CTextClipboard
{
	CClipboard( CWnd* pWnd );
public:
	static CClipboard* Open( CWnd* pWnd );		// AfxGetMainWnd()
public:
	// CF_HDROP: cut or copied files
	static bool HasDropFiles( void );
	static DROPEFFECT QueryDropFilePaths( std::vector< fs::CPath >& rSrcPaths );
	static bool AlsoCopyDropFilesAsPaths( CWnd* pParentOwner );		// if files Copied or Pasted on clipboard, also store their paths as text
public:
	static const CLIPFORMAT s_cfPreferredDropEffect;
};


#endif // Clipboard_h
