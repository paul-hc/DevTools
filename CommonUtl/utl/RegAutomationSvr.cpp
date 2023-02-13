
#include "pch.h"
#include "RegAutomationSvr.h"
#include "Path.h"
#include "Registry.h"
#include <afxdisp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// FWD from <afximpl.h>
#define CRIT_OBJECTFACTORYLIST  0
#define CRIT_DYNLINKLIST        0
void AFXAPI AfxLockGlobals( int lockType );
void AFXAPI AfxUnlockGlobals( int lockType );


namespace hlp
{
	class CFriendlyObjectFactory : public COleObjectFactory
	{
		CFriendlyObjectFactory( void ) : COleObjectFactory(  __uuidof( IUnknown ), nullptr, FALSE, nullptr ) {}
	public:
		using COleObjectFactory::m_clsid;			// coclass ID
		using COleObjectFactory::m_lpszProgID;		// human readable class ID
	};
}


namespace ole
{
	std::wstring FormatGUID( const GUID& guid )
	{
		wchar_t buffer[ 64 ];

		::StringFromGUID2( guid, buffer, COUNT_OF( buffer ) );
		return buffer;
	}

	std::tstring GetProgID( const COleObjectFactory* pFactory )
	{
		ASSERT_PTR( pFactory );
		return ( (const hlp::CFriendlyObjectFactory*)pFactory )->m_lpszProgID;
	}


	// CSafeForScripting class

	const TCHAR* CSafeForScripting::s_categoryIds[] =
	{
		_T("{40FC6ED5-2438-11CF-A3DB-080036F12502}"),		// i.e. "Automation Objects"
		_T("{7DD95801-9882-11CF-9FA9-00AA006C42C4}"),
		_T("{7DD95802-9882-11CF-9FA9-00AA006C42C4}")
	};

	size_t CSafeForScripting::UpdateRegistryAll( RegAction action /*= ole::Register*/ )
	{
		AFX_MODULE_STATE* pModuleState = AfxGetModuleState();
		::AfxLockGlobals( CRIT_OBJECTFACTORYLIST );

		size_t coClassCount = 0;

		for ( COleObjectFactory* pFactory = pModuleState->m_factoryList; pFactory != NULL; pFactory = pFactory->m_pNextFactory )
			if ( UpdateRegistry( pFactory->GetClassID(), action ) )
				++coClassCount;

		::AfxUnlockGlobals( CRIT_OBJECTFACTORYLIST );

	#ifdef _AFXDLL
		::AfxLockGlobals( CRIT_DYNLINKLIST );
		// register extension DLL factories
		for ( CDynLinkLibrary* pDLL = pModuleState->m_libraryList; pDLL != NULL; pDLL = pDLL->m_pNextDLL )
			for ( COleObjectFactory* pDLLFactory = pDLL->m_factoryList; pDLLFactory != NULL; pDLLFactory = pDLLFactory->m_pNextFactory )
				if ( UpdateRegistry( pDLLFactory->GetClassID(), action ) )
					++coClassCount;

		::AfxUnlockGlobals( CRIT_DYNLINKLIST );
	#endif

		return coClassCount;
	}

	bool CSafeForScripting::UpdateRegistry( const CLSID& coClassId, RegAction action )
	{
		std::wstring coClassIdTag = ole::FormatGUID( coClassId );

		return ole::Register == action
			? RegisterCoClass( coClassIdTag.c_str() )
			: UnregisterCoClass( coClassIdTag.c_str() );
	}

	bool CSafeForScripting::RegisterCoClass( const TCHAR* pCoClassIdTag )
	{
		fs::CPath implCategKeyPath = fs::CPath( _T("CLSID") ) / pCoClassIdTag / _T("Implemented Categories");
		bool succeeded = true;

		for ( unsigned int i = 0; i != COUNT_OF( s_categoryIds ); ++i )
		{
			fs::CPath categoryKeyPath = implCategKeyPath / s_categoryIds[ i ];
			reg::CKey key;

			if ( !key.Create( HKEY_CLASSES_ROOT, categoryKeyPath ) )
			{
				succeeded = false;
				TRACE( _T(" * CSafeForScripting::RegisterCoClass(): Error registering coclass category key: %s\n"), categoryKeyPath.GetPtr() );
			}
		}

		return succeeded;
	}

	bool CSafeForScripting::UnregisterCoClass( const TCHAR* pCoClassIdTag )
	{
		static const fs::CPath s_classesKeyFullPath( _T("CLSID") );
		bool succeeded = true;
		reg::CKey key;

		if ( key.Open( HKEY_CLASSES_ROOT, s_classesKeyFullPath / pCoClassIdTag ) )
		{
			key.DeleteAll();
			key.Close();

			if ( key.Open( HKEY_CLASSES_ROOT, s_classesKeyFullPath ) )
				if ( !key.DeleteSubKey( pCoClassIdTag ) )
				{
					succeeded = false;
					TRACE( _T(" * CSafeForScripting::UnregisterCoClass(): Error unregistering coclass category key: %s\n"), pCoClassIdTag );
				}
		}

		return succeeded;
	}
}
