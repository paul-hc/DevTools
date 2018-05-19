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


	std::tstring FormatFileState( const fs::CFileState& fileState );
	bool ParseFileState( fs::CFileState& rState, str::TStringRange& rTextRange );

	std::tstring FormatRenameEntry( const fs::CPath& srcPath, const fs::CPath& destPath );
	bool ParseRenameEntry( fs::CPath& rSrcPath, fs::CPath& rDestPath, const str::TStringRange& textRange );

	std::tstring FormatTouchEntry( const fs::CFileState& srcState, const fs::CFileState& destState );
	bool ParseTouchEntry( fs::CFileState& rSrcState, fs::CFileState& rDestState, const str::TStringRange& textRange );


	namespace impl
	{
		std::tstring FormatFileState( const fs::CFileState& state, bool withFullPath = true );
		bool ParseFileState( fs::CFileState& rState, const std::tstring& text, bool withFullPath = true );
	}
}


#endif // FmtUtils_h
