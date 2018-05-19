
#include "stdafx.h"
#include "FmtUtils.h"
#include "FileState.h"
#include "StringRange.h"
#include "StringUtilities.h"
#include "TimeUtl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fmt
{
	static const TCHAR s_fieldSep[] = _T("|");
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


	std::tstring FormatFileState( const fs::CFileState& fileState )
	{
		return FormatBraces( impl::FormatFileState( fileState, false ).c_str(), s_stateBraces );
	}

	bool ParseFileState( fs::CFileState& rState, str::TStringRange& rTextRange )
	{
		return
			ParseBraces( rTextRange, s_stateBraces ) &&
			impl::ParseFileState( rState, rTextRange.Extract(), false );
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
		std::tstring FormatFileState( const fs::CFileState& state, bool withFullPath /*= true*/ )
		{
			std::vector< std::tstring > parts;
			if ( !state.IsEmpty() )
			{
				if ( withFullPath )
					parts.push_back( state.m_fullPath.Get() );

				parts.push_back( num::FormatHexNumber( state.m_attributes ) );
				parts.push_back( time_utl::FormatTimestamp( state.m_creationTime ) );
				parts.push_back( time_utl::FormatTimestamp( state.m_modifTime ) );
				parts.push_back( time_utl::FormatTimestamp( state.m_accessTime ) );
			}
			return str::Join( parts, s_fieldSep );
		}

		bool ParseFileState( fs::CFileState& rState, const std::tstring& text, bool withFullPath /*= true*/ )
		{
			std::vector< std::tstring > parts;
			str::Split( parts, text.c_str(), s_fieldSep );

			const size_t fieldCount = withFullPath ? 5 : 4;
			if ( parts.size() != fieldCount )
			{
				rState.Clear();
				return false;
			}

			size_t pos = 0;

			if ( withFullPath )
				rState.m_fullPath.Set( parts[ pos++ ] );

			num::ParseHexNumber( rState.m_attributes, parts[ pos++ ] );
			rState.m_creationTime = time_utl::ParseTimestamp( parts[ pos++ ] );
			rState.m_modifTime = time_utl::ParseTimestamp( parts[ pos++ ] );
			rState.m_accessTime = time_utl::ParseTimestamp( parts[ pos++ ] );
			return true;
		}
	}
}
