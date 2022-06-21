#ifndef FmtUtils_h
#define FmtUtils_h
#pragma once

#include "StringRange_fwd.h"


namespace fs
{
	class CPath;
	struct CFileState;
}


namespace fmt
{
	// formatting utilities: for instance used in batch file operations logging

	std::tstring FormatBraces( const TCHAR core[], const TCHAR braces[] );
	bool ParseBraces( str::TStringRange& rTextRange, const TCHAR braces[] );

	std::tstring FormatFromTo( const std::tstring& src, const std::tstring& dest );
	bool ParseFromTo( std::tstring& rSrc, std::tstring& rDest, const str::TStringRange& textRange );


	enum PathFormat { NoPath, FullPath, FilenameExt };

	const TCHAR* FormatPath( const fs::CPath& fullPath, PathFormat format );


	std::tstring FormatFileAttributes( DWORD fileAttr, bool uiFormat = false );
	DWORD ParseFileAttributes( const std::tstring& text, bool uiFormat = false );


	std::tstring FormatFileStateCore( const fs::CFileState& fileState, bool tagged = true );		// core: excluding fileState.m_fullPath
	bool ParseFileStateCore( fs::CFileState& rFileState, str::TStringRange& rTextRange );

	std::tstring FormatClipFileState( const fs::CFileState& fileState, PathFormat pathFormat = FullPath, bool tagged = true );
	fs::CFileState& ParseClipFileState( fs::CFileState& rFileState, const std::tstring& text, const fs::CPath* pKeyPath = NULL ) throws_( CRuntimeException );

	std::tstring FormatRenameEntry( const fs::CPath& srcPath, const fs::CPath& destPath );
	std::tstring FormatRenameEntryRelativeDest( const fs::CPath& srcPath, const fs::CPath& destPath );
	bool ParseRenameEntry( fs::CPath& rSrcPath, fs::CPath& rDestPath, const str::TStringRange& textRange );

	std::tstring FormatTouchEntry( fs::CFileState srcState, fs::CFileState destState, bool tagged = true );
	bool ParseTouchEntry( fs::CFileState& rSrcState, fs::CFileState& rDestState, const str::TStringRange& textRange );


	namespace impl
	{
		std::tstring FormatFileState( const fs::CFileState& fileState, PathFormat pathFormat, bool tagged );
		bool ParseFileState_Tagged( fs::CFileState& rFileState, const std::tstring& text, PathFormat pathFormat );		// time fields are optional; if missing will preserve existing ones
	}
}


#endif // FmtUtils_h
