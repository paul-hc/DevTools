
#include "pch.h"
#include "FmtUtils.h"
#include "FileState.h"
#include "EnumTags.h"
#include "FlagTags.h"
#include "RuntimeException.h"
#include "StringRange.h"
#include "StringUtilities.h"
#include "TimeUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fmt
{
	static const TCHAR s_clipSep[] = _T("\t");
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
		Range<size_t> sepPos;
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
			case FilenameExt:	return fullPath.GetFilenamePtr();
		}
	}


	std::tstring FormatFileAttributes( DWORD fileAttr, bool uiFormat /*= false*/ )
	{
		return uiFormat
			? fs::CFileState::GetTags_FileAttributes().FormatUi( fileAttr, _T(", ") )
			: fs::CFileState::GetTags_FileAttributes().FormatKey( fileAttr, _T("") );
	}

	DWORD ParseFileAttributes( const std::tstring& text, bool uiFormat /*= false*/ )
	{
		DWORD fileAttr = 0;
		bool parsedHex = str::EqualsIN( text.c_str(), _T("0x"), 2 ) && num::ParseHexNumber( fileAttr, text );

		if ( !parsedHex )
			if ( uiFormat )
				fs::CFileState::GetTags_FileAttributes().ParseUi( reinterpret_cast<int*>( &fileAttr ), text, _T(", ") );
			else
				fs::CFileState::GetTags_FileAttributes().ParseKey( reinterpret_cast<int*>( &fileAttr ), text, _T("") );

		return static_cast<DWORD>( fileAttr );
	}


	std::tstring FormatFileStateCore( const fs::CFileState& fileState, bool tagged /*= true*/ )
	{
		return FormatBraces( impl::FormatFileState( fileState, NoPath, tagged ).c_str(), s_stateBraces );
	}

	bool ParseFileStateCore( fs::CFileState& rFileState, str::TStringRange& rTextRange )
	{
		return
			ParseBraces( rTextRange, s_stateBraces ) &&
			impl::ParseFileState_Tagged( rFileState, rTextRange.Extract(), NoPath );
	}


	std::tstring FormatClipFileState( const fs::CFileState& fileState, PathFormat pathFormat /*= FullPath*/, bool tagged /*= true*/ )
	{
		return std::tstring( FormatPath( fileState.m_fullPath, pathFormat ) ) + s_clipSep + FormatFileStateCore( fileState, tagged );
	}

	fs::CFileState& ParseClipFileState( fs::CFileState& rFileState, const std::tstring& text, const fs::CPath* pKeyPath /*= NULL*/ ) throws_( CRuntimeException )
	{
		str::TStringRange textRange( text );

		Range<size_t> sepPos;
		if ( textRange.Find( sepPos, s_clipSep ) )
		{
			rFileState.m_fullPath = textRange.ExtractLead( sepPos.m_start );

			if ( pKeyPath != NULL )
			{
				if ( !rFileState.m_fullPath.HasParentPath() )
					rFileState.m_fullPath.SetDirPath( pKeyPath->GetParentPath().Get() );		// qualify with SRC dir path

				if ( rFileState.m_fullPath != *pKeyPath )
					throw CRuntimeException( str::Format( _T("Pasted destination file path is inconsistent with source path.\n\nSource path: %s\nDestination path: %s"),
						pKeyPath->GetPtr(), rFileState.m_fullPath.GetPtr() ) );
			}

			str::TStringRange stateRange = textRange.MakeTrail( sepPos.m_end );
			if ( ParseFileStateCore( rFileState, stateRange ) )
				return rFileState;
		}

		throw CRuntimeException( str::Format( _T("Pasted destination file status format is not valid: %s"), text.c_str() ) );
	}


	std::tstring FormatRenameEntry( const fs::CPath& srcPath, const fs::CPath& destPath )
	{
		return srcPath.Get() + s_pairSep + destPath.GetFilenamePtr();
	}

	std::tstring FormatRenameEntryRelativeDest( const fs::CPath& srcPath, const fs::CPath& destPath )
	{
		std::tstring relativeDestPath = path::StripCommonPrefix( destPath.GetPtr(), srcPath.GetParentPath().GetPtr() );
		return srcPath.Get() + s_pairSep + relativeDestPath;
	}

	bool ParseRenameEntry( fs::CPath& rSrcPath, fs::CPath& rDestPath, const str::TStringRange& textRange )
	{
		Range<size_t> sepPos;
		if ( textRange.Find( sepPos, s_pairSep ) )
		{
			str::TStringRange srcRange = textRange.MakeLead( sepPos.m_start );
			str::TStringRange destRange = textRange.MakeTrail( sepPos.m_end );
			srcRange.Trim();
			destRange.Trim();

			rSrcPath.Set( srcRange.Extract() );
			rDestPath.Set( destRange.Extract() );

			if ( path::IsFilename( rDestPath.GetPtr() ) )
				rDestPath = rSrcPath.GetParentPath() / rDestPath;		// convert to full path

			return !rSrcPath.IsEmpty() && !rDestPath.IsEmpty();
		}
		return false;
	}


	std::tstring FormatTouchEntry( fs::CFileState srcState, fs::CFileState destState, bool tagged /*= true*/ )
	{
		ASSERT( srcState.m_fullPath == destState.m_fullPath );

		// net changes output: reset equal time-fields to NULL to minimize output
		static const CTime s_nullTime( 0 );

		if ( srcState.m_creationTime == destState.m_creationTime )
			srcState.m_creationTime = destState.m_creationTime = s_nullTime;

		if ( srcState.m_modifTime == destState.m_modifTime )
			srcState.m_modifTime = destState.m_modifTime = s_nullTime;

		if ( srcState.m_accessTime == destState.m_accessTime )
			srcState.m_accessTime = destState.m_accessTime = s_nullTime;

		return srcState.m_fullPath.Get() + s_touchSep + FormatFileStateCore( srcState, tagged ) + s_pairSep + FormatFileStateCore( destState, tagged );
	}

	bool ParseTouchEntry( fs::CFileState& rSrcState, fs::CFileState& rDestState, const str::TStringRange& textRange )
	{
		Range<size_t> sepPos;
		if ( textRange.Find( sepPos, s_touchSep ) )
		{
			rSrcState.m_fullPath = rDestState.m_fullPath = textRange.ExtractLead( sepPos.m_start );
			str::TStringRange nextRange = textRange.MakeTrail( sepPos.m_end );

			if ( nextRange.Find( sepPos, s_pairSep ) )
			{
				str::TStringRange srcRange = nextRange.MakeLead( sepPos.m_start );
				str::TStringRange destRange = nextRange.MakeTrail( sepPos.m_end );

				if ( ParseFileStateCore( rSrcState, srcRange ) )
					if ( ParseFileStateCore( rDestState, destRange ) )
						return true;
			}
		}
		return false;
	}


	namespace impl
	{
		static const TCHAR s_fieldSep[] = _T("|");
		static const TCHAR s_tagPrefixSep[] = _T("=");


		std::tstring FormatFileState_Tagged( const fs::CFileState& fileState, PathFormat pathFormat );


		std::tstring FormatFileState( const fs::CFileState& fileState, PathFormat pathFormat, bool tagged )
		{
			if ( tagged )
				return FormatFileState_Tagged( fileState, pathFormat );

			std::vector<std::tstring> parts;
			if ( !fileState.IsEmpty() )
			{
				if ( pathFormat != NoPath )
					parts.push_back( FormatPath( fileState.m_fullPath, pathFormat ) );

				parts.push_back( FormatFileAttributes( fileState.m_attributes, false ) );
				parts.push_back( time_utl::FormatTimestamp( fileState.m_creationTime ) );
				parts.push_back( time_utl::FormatTimestamp( fileState.m_modifTime ) );
				parts.push_back( time_utl::FormatTimestamp( fileState.m_accessTime ) );
			}
			return str::Join( parts, s_fieldSep );
		}

		bool ParseFileState( fs::CFileState& rFileState, const std::tstring& text, PathFormat pathFormat )
		{
			std::vector<std::tstring> parts;
			str::Split( parts, text.c_str(), s_fieldSep );

			const size_t fieldCount = NoPath == pathFormat ? 4 : 5;
			if ( parts.size() != fieldCount )
			{
				rFileState.Clear();
				return false;
			}

			size_t pos = 0;

			if ( pathFormat != NoPath )
				rFileState.m_fullPath.Set( parts[ pos++ ] );

			rFileState.m_attributes = static_cast<BYTE>( ParseFileAttributes( parts[ pos++ ], false ) );
			rFileState.m_creationTime = time_utl::ParseTimestamp( parts[ pos++ ] );
			rFileState.m_modifTime = time_utl::ParseTimestamp( parts[ pos++ ] );
			rFileState.m_accessTime = time_utl::ParseTimestamp( parts[ pos++ ] );
			return true;
		}


		std::tstring _FormatTaggedTimeField( const CTime& time, fs::TimeField field )
		{
			return fs::GetTags_TimeField().FormatKey( field ) + s_tagPrefixSep + time_utl::FormatTimestamp( time );
		}

		bool _ParseTaggedTimeField( CTime& rTime, fs::TimeField& rField, const std::tstring& text )
		{
			str::TStringRange textRange( text );
			Range<size_t> sepPos;
			if ( textRange.Find( sepPos, s_tagPrefixSep ) )
			{
				str::TStringRange tagRange = textRange.MakeLead( sepPos.m_start );
				if ( fs::GetTags_TimeField().ParseAs( rField, tagRange.Extract(), CEnumTags::KeyTag ) )
				{
					rTime = time_utl::ParseTimestamp( textRange.MakeTrail( sepPos.m_end ).Extract() );
					return time_utl::IsValid( rTime );
				}
			}

			return false;
		}

		// tagged: time-fields prefixed with key tags: "M="
		//
		std::tstring FormatFileState_Tagged( const fs::CFileState& fileState, PathFormat pathFormat )
		{
			std::vector<std::tstring> parts;
			if ( !fileState.IsEmpty() )
			{
				if ( pathFormat != NoPath )
					parts.push_back( FormatPath( fileState.m_fullPath, pathFormat ) );

				parts.push_back( FormatFileAttributes( fileState.m_attributes, false ) );

				for ( fs::TimeField field = fs::TimeField( 0 ); field != fs::_TimeFieldCount; ++(int&)field )
				{
					const CTime& time = fileState.GetTimeField( field );
					if ( time_utl::IsValid( time ) )
						parts.push_back( _FormatTaggedTimeField( time, field ) );		// only add defined time-fields
				}
			}
			return str::Join( parts, s_fieldSep );
		}

		bool ParseFileState_Tagged( fs::CFileState& rFileState, const std::tstring& text, PathFormat pathFormat )
		{
			std::vector<std::tstring> parts;
			str::Split( parts, text.c_str(), s_fieldSep );

			std::vector<std::tstring>::iterator itPart = parts.begin();

			if ( itPart == parts.end() )
				return false;

			if ( pathFormat != NoPath )
				rFileState.m_fullPath.Set( *itPart++ );

			if ( itPart == parts.end() )
				return false;

			rFileState.m_attributes = static_cast<BYTE>( ParseFileAttributes( *itPart++, false ) );

			if ( 3 == std::distance( itPart, parts.end() ) && std::tstring::npos == itPart->find( s_tagPrefixSep ) )	// 3 time fields not tagged?
			{	// backwards compatibility: parse untagged time fields (mandatory)
				rFileState.m_creationTime = time_utl::ParseTimestamp( *itPart++ );
				rFileState.m_modifTime = time_utl::ParseTimestamp( *itPart++ );
				rFileState.m_accessTime = time_utl::ParseTimestamp( *itPart++ );
			}
			else
			{
				// optional tagged time fields: missing ones will preserve existing data-members
				for ( ; itPart != parts.end(); ++itPart )
				{
					fs::TimeField field;
					CTime time;

					if ( _ParseTaggedTimeField( time, field, *itPart ) )
						rFileState.SetTimeField( time, field );
				}
			}

			return true;
		}

		void ParseFileState_TaggedThrow( fs::CFileState& rFileState, const std::tstring& text, PathFormat pathFormat ) throws_( CRuntimeException )
		{
			if ( !ParseFileState_Tagged( rFileState, text, pathFormat ) )
				throw CRuntimeException( str::Format( _T("Invalid format for file status: %s"), text.c_str() ) );
		}
	}
}
