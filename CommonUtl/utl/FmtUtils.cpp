
#include "stdafx.h"
#include "FmtUtils.h"
#include "FileState.h"
#include "FlagTags.h"
#include "StringRange.h"
#include "StringUtilities.h"
#include "TimeUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fmt
{
	static const TCHAR s_pairSep[] = _T(" -> ");
	static const TCHAR s_touchSep[] = _T(" :: ");
	static const TCHAR s_stateBraces[] = { _T("{}") };

	std::tstring FormatBraces( const TCHAR core[], const TCHAR braces[] )
	{
		ASSERT( str::GetLength( braces ) >= 2 );
		return str::Format( _T("%c%s%c"), braces[ 0 ], core, braces[ 1 ] );
	}

	bool ParseBraces( str::TStringRange& rTextRange, const TCHAR braces[] )
	{
		ASSERT( str::GetLength( braces ) >= 2 );
		rTextRange.Trim();
		return rTextRange.Strip( braces[ 0 ], braces[ 1 ] );
	}


	std::tstring FormatFromTo( const std::tstring& src, const std::tstring& dest )
	{
		return src + s_pairSep + dest;
	}

	bool ParseFromTo( std::tstring& rSrc, std::tstring& rDest, const str::TStringRange& textRange )
	{
		Range< size_t > sepPos;
		if ( textRange.Find( sepPos, s_pairSep ) )
		{
			str::TStringRange srcRange = textRange.MakeLead( sepPos.m_start );
			str::TStringRange destRange = textRange.MakeTrail( sepPos.m_end );

			srcRange.Trim();
			destRange.Trim();

			rSrc = srcRange.Extract();
			rDest = destRange.Extract();
			return !rSrc.empty() && !rDest.empty();
		}
		return false;
	}


	const TCHAR* FormatPath( const fs::CPath& fullPath, PathFormat format )
	{
		switch ( format )
		{
			default: ASSERT( false );
			case NoPath:		return NULL;
			case FullPath:		return fullPath.GetPtr();
			case FilenameExt:	return fullPath.GetNameExt();
		}
	}


	std::tstring FormatFileAttributes( DWORD fileAttr, bool uiFormat /*= false*/ )
	{
		return uiFormat
			? fs::GetTags_FileAttributes().FormatUi( fileAttr, _T(", ") )
			: fs::GetTags_FileAttributes().FormatKey( fileAttr, _T("") );
	}

	DWORD ParseFileAttributes( const std::tstring& text, bool uiFormat /*= false*/ )
	{
		DWORD fileAttr = 0;
		bool parsedHex = str::EqualsN( text.c_str(), _T("0x"), 2, false ) && num::ParseHexNumber( fileAttr, text );

		if ( !parsedHex )
			if ( uiFormat )
				fs::GetTags_FileAttributes().ParseUi( reinterpret_cast< int* >( &fileAttr ), text, _T(", ") );
			else
				fs::GetTags_FileAttributes().ParseKey( reinterpret_cast< int* >( &fileAttr ), text, _T("") );

		return static_cast< DWORD >( fileAttr );
	}


	std::tstring FormatFileState( const fs::CFileState& fileState )
	{
		return FormatBraces( impl::FormatFileState( fileState, NoPath ).c_str(), s_stateBraces );
	}

	bool ParseFileState( fs::CFileState& rState, str::TStringRange& rTextRange )
	{
		return
			ParseBraces( rTextRange, s_stateBraces ) &&
			impl::ParseFileState( rState, rTextRange.Extract(), NoPath );
	}


	std::tstring FormatRenameEntry( const fs::CPath& srcPath, const fs::CPath& destPath )
	{
		return srcPath.Get() + s_pairSep + destPath.GetNameExt();
	}

	bool ParseRenameEntry( fs::CPath& rSrcPath, fs::CPath& rDestPath, const str::TStringRange& textRange )
	{
		Range< size_t > sepPos;
		if ( textRange.Find( sepPos, s_pairSep ) )
		{
			str::TStringRange srcRange = textRange.MakeLead( sepPos.m_start );
			str::TStringRange destRange = textRange.MakeTrail( sepPos.m_end );
			srcRange.Trim();
			destRange.Trim();

			rSrcPath.Set( srcRange.Extract() );
			rDestPath.Set( destRange.Extract() );

			if ( path::IsNameExt( rDestPath.GetPtr() ) )
				rDestPath = rSrcPath.GetParentPath() / rDestPath;		// convert to full path

			return !rSrcPath.IsEmpty() && !rDestPath.IsEmpty();
		}
		return false;
	}


	std::tstring FormatTouchEntry( const fs::CFileState& srcState, const fs::CFileState& destState )
	{
		ASSERT( srcState.m_fullPath == destState.m_fullPath );
		return srcState.m_fullPath.Get() + s_touchSep + FormatFileState( srcState ) + s_pairSep + FormatFileState( destState );
	}

	bool ParseTouchEntry( fs::CFileState& rSrcState, fs::CFileState& rDestState, const str::TStringRange& textRange )
	{
		Range< size_t > sepPos;
		if ( textRange.Find( sepPos, s_touchSep ) )
		{
			rSrcState.m_fullPath = rDestState.m_fullPath = textRange.ExtractLead( sepPos.m_start );
			str::TStringRange nextRange = textRange.MakeTrail( sepPos.m_end );

			if ( nextRange.Find( sepPos, s_pairSep ) )
			{
				str::TStringRange srcRange = nextRange.MakeLead( sepPos.m_start );
				str::TStringRange destRange = nextRange.MakeTrail( sepPos.m_end );

				if ( ParseFileState( rSrcState, srcRange ) )
					if ( ParseFileState( rDestState, destRange ) )
						return true;
			}
		}
		return false;
	}


	namespace impl
	{
		static const TCHAR s_fieldSep[] = _T("|");

		std::tstring FormatFileState( const fs::CFileState& state, PathFormat pathFormat )
		{
			std::vector< std::tstring > parts;
			if ( !state.IsEmpty() )
			{
				if ( pathFormat != NoPath )
					parts.push_back( FormatPath( state.m_fullPath, pathFormat ) );

				parts.push_back( FormatFileAttributes( state.m_attributes, false ) );
				parts.push_back( time_utl::FormatTimestamp( state.m_creationTime ) );
				parts.push_back( time_utl::FormatTimestamp( state.m_modifTime ) );
				parts.push_back( time_utl::FormatTimestamp( state.m_accessTime ) );
			}
			return str::Join( parts, s_fieldSep );
		}

		bool ParseFileState( fs::CFileState& rState, const std::tstring& text, PathFormat pathFormat )
		{
			std::vector< std::tstring > parts;
			str::Split( parts, text.c_str(), s_fieldSep );

			const size_t fieldCount = NoPath == pathFormat ? 4 : 5;
			if ( parts.size() != fieldCount )
			{
				rState.Clear();
				return false;
			}

			size_t pos = 0;

			if ( pathFormat != NoPath )
				rState.m_fullPath.Set( parts[ pos++ ] );

			rState.m_attributes = static_cast< BYTE >( ParseFileAttributes( parts[ pos++ ], false ) );
			rState.m_creationTime = time_utl::ParseTimestamp( parts[ pos++ ] );
			rState.m_modifTime = time_utl::ParseTimestamp( parts[ pos++ ] );
			rState.m_accessTime = time_utl::ParseTimestamp( parts[ pos++ ] );
			return true;
		}
	}
}
