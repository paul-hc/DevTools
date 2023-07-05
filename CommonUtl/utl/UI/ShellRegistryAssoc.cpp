
#include "pch.h"
#include "ShellRegistryAssoc.h"
#include "Registry.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	reg::TKeyPath MakeShellHandlerVerbPath( const TCHAR handlerName[], const TCHAR verb[] )
	{
		// returns "Directory\\shell\\SlideView" for handlerName="Directory", verb="SlideView"
		ASSERT( !str::IsEmpty( verb ) );
		return reg::TKeyPath( handlerName ) / _T("shell") / verb;
	}

	bool QueryHandlerName( std::tstring& rHandlerName, const TCHAR ext[] )
	{
		reg::CKey key;
		if ( key.Open( HKEY_CLASSES_ROOT, ext, KEY_READ ) )			// extension already registered?
			if ( key.QueryStringValue( nullptr, rHandlerName ) )		// query for default key value (shell handler name)
				return !rHandlerName.empty();

		return false;
	}

	const TCHAR* GetDdeOpenCmd( void )
	{
		return _T("[open(\"%1\")]");
	}

	std::tstring MakeParam( unsigned int paramNo /*= 1*/ )
	{
		ASSERT( paramNo >= 1 );
		return arg::Enquote( std::tstring( _T("%") ) + num::FormatNumber( paramNo ) );
	}


	// example:
	//	verbPath="Directory\\shell\\SlideView":
	//	pVerbTag="Slide &View" (optional display text in shell context menu)
	//	pDdeCmd="[open(\"%1\")]" (optional DDE command)
	//
	bool RegisterShellVerb( const reg::TKeyPath& verbPath, const reg::TKeyPath& modulePath,
							const TCHAR* pVerbTag /*= nullptr*/, const TCHAR* pDdeCmd /*= nullptr*/,
							const std::tstring extraParams /*= str::GetEmpty()*/ )
	{
		REQUIRE( !verbPath.IsEmpty() );

		if ( !str::IsEmpty( pVerbTag ) )
		{
			reg::CKey key;
			if ( !key.Create( HKEY_CLASSES_ROOT, verbPath ) ||
				 !key.WriteStringValue( nullptr, pVerbTag ) )				// default value of the key
			{
				TRACE( _T(" * RegisterShellVerb( %s ): ERROR: %s\n"), verbPath.GetPtr(), key.GetLastError().FormatError().c_str() );
				return false;
			}
		}

		bool useDDE = !str::IsEmpty( pDdeCmd );
		reg::TKeyPath subKeyPath;
		{
			std::tstring cmdLine;

			if ( useDDE )
			{
				cmdLine = fs::GetShortFilePath( modulePath ).Get();			// use short module path for DDE protocol
				stream::Tag( cmdLine, _T("/dde"), _T(" ") );
			}
			else
			{
				cmdLine = arg::Enquote( modulePath );
				stream::Tag( cmdLine, MakeParam( 1 ), _T(" ") );
			}

			reg::CKey key;

			subKeyPath = verbPath / _T("command");
			if ( !key.Create( HKEY_CLASSES_ROOT, subKeyPath ) ||
				 !key.WriteStringValue( nullptr, cmdLine ) )				// default value of the key
			{
				TRACE( _T(" * RegisterShellVerb( %s ): ERROR: %s\n"), subKeyPath.GetPtr(), key.GetLastError().FormatError().c_str() );
				return false;
			}
		}

		if ( useDDE )
		{
			reg::CKey key;

			subKeyPath = verbPath / _T("ddeexec");
			if ( !key.Create( HKEY_CLASSES_ROOT, verbPath / _T("ddeexec") ) ||
				 !key.WriteStringValue( nullptr, pDdeCmd ) )				// default value of the key
			{
				TRACE( _T(" * RegisterShellVerb( %s ): ERROR: %s\n"), subKeyPath.GetPtr(), key.GetLastError().FormatError().c_str() );
				return false;
			}
		}

		return true;
	}

	bool UnregisterShellVerb( const reg::TKeyPath& verbPath )
	{
		return reg::DeleteKey( HKEY_CLASSES_ROOT, verbPath, Deep );			// deep-delete verb key
	}


	bool RegisterAdditionalDocumentExt( const reg::TKeyPath& docExt, const std::tstring& docTypeId )
	{
		ASSERT( !docExt.IsEmpty() && _T('.') == docExt.Get()[ 0 ] );

		// check if there is a valid association for the docExt
		std::tstring handlerName;
		shell::QueryHandlerName( handlerName, docExt.GetPtr() );

		if ( handlerName != docTypeId )										// bad handler?
			if ( !reg::WriteStringValue( HKEY_CLASSES_ROOT, docExt, nullptr, docTypeId ) )			// default value of the key
				return false;

		return reg::WriteStringValue( HKEY_CLASSES_ROOT, docExt / _T("ShellNew"), _T("NullFile"), std::tstring() );
	}
}
