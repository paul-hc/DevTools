
#include "stdafx.h"
#include "SafeForScripting.h"
#include "utl/Registry.h"

#if _MSC_VER <= 1200
#include <../src/afximpl.h>		// VC++ 6
#else
#include <../src/mfc/afximpl.h> // VC++ 7.1 or higher
#endif


namespace scripting
{
	// COM registration helpers implementation

	static const TCHAR* categoryIds[] =
	{
		_T("{40FC6ED5-2438-11CF-A3DB-080036F12502}"),
		_T("{7DD95801-9882-11CF-9FA9-00AA006C42C4}"),
		_T("{7DD95802-9882-11CF-9FA9-00AA006C42C4}")
	};

	static struct { const TCHAR* m_classId; const TCHAR* m_progId; } ideToolsScriptingObjects[] = // automation objects defined in IDE Tools
	{
		{ _T("{0E44AB07-90E1-11D2-A2C9-006097B8DD84}"), _T("IDETools.WorkspaceProfile") },
		{ _T("{216EF195-4C10-11D3-A3C8-006097B8DD84}"), _T("IDETools.UserInterface") },
		{ _T("{E37FE177-CBB7-11D4-B57C-00D0B74ECB52}"), _T("IDETools.TextContent") },
		{ _T("{4064259A-55DC-4CD5-8A17-DD1CC7B59673}"), _T("IDETools.ModuleOptions") },
		{ _T("{4DFA7BE2-8484-11D2-A2C3-006097B8DD84}"), _T("IDETools.MenuFilePicker") },
		{ _T("{1006E3E7-1F6F-11D2-A275-006097B8DD84}"), _T("IDETools.IncludeFileTree") },
		{ _T("{C722D0B6-1E2D-11D5-B59B-00D0B74ECB52}"), _T("IDETools.FileSearch") },
		{ _T("{A0580B96-3350-11D5-B5A4-00D0B74ECB52}"), _T("IDETools.FileLocator") },
		{ _T("{1556FB25-22DB-11D2-A278-006097B8DD84}"), _T("IDETools.FileAccess") },
		{ _T("{C60E380C-3DE3-4D69-9120-D76A23ECDC8D}"), _T("IDETools.DspProject") },
		{ _T("{F70182C0-AE07-4DEB-AFEB-31BCC6BB244C}"), _T("IDETools.CodeProcessor") }
	};


	void registerScriptSafeObject( const TCHAR* classId );
	void unregisterClass( const TCHAR* classId );
	CString getGuidAsString( REFGUID guid, bool withEnclosingBraces = true );
	const TCHAR* getFactoryProgID( COleObjectFactory& objectFactory );


	// registers automation objects in component categories in order to make them safe for scripting
	//
	void registerAllScriptObjects( RegAction regAction )
	{
	#ifdef _AFXDLL
		// link dynamically to MFC
		AFX_MODULE_STATE* pModuleState = AfxGetModuleState();

		AfxLockGlobals( CRIT_DYNLINKLIST );

		size_t classIdCount = 0;

		// register extension DLL factories
		for ( CDynLinkLibrary* pDll = pModuleState->m_libraryList; pDll != NULL; pDll = pDll->m_pNextDLL )
			for ( COleObjectFactory* pFactory = pDll->m_factoryList; pFactory != NULL; pFactory = pFactory->m_pNextFactory )
			{
				CString classId = getGuidAsString( pFactory->GetClassID(), true ); // With "{}"

				TRACE( _T("\t%s %s %s\n"),
					   Register == regAction ? _T("Registering") : _T("Unregistering"),
					   getFactoryProgID( *pFactory ),
					   (const TCHAR*)classId );

				// register/unregister automation objects for safe scripting
				if ( Register == regAction )
					registerScriptSafeObject( classId );
				else
					unregisterClass( classId );

				++classIdCount;
			}

		ENSURE( classIdCount == COUNT_OF( ideToolsScriptingObjects ) ); // forgot to add new scriptable object classId to ideToolsScriptingObjects

		AfxUnlockGlobals( CRIT_DYNLINKLIST );
	#else
		// link statically to MFC
		for ( unsigned int i = 0; i != COUNT_OF( ideToolsScriptingObjects ); ++i )
		{
			// register/unregister automation objects for safe scripting
			const CString classId = ideToolsScriptingObjects[ i ].m_classId; // With "{}"
			const CString progId = ideToolsScriptingObjects[ i ].m_classId; // With "{}"
			UNUSED_ALWAYS( progId );

			TRACE( _T("\t%s %s %s\n"),
				   Register == regAction ? _T("Registering") : _T("Unregistering"), (const TCHAR*)progId, (const TCHAR*)classId );

			if ( Register == regAction )
				registerScriptSafeObject( classId );
			else
				unregisterClass( classId );
		}
	#endif
	}

	// registers automation object identified by classId as safe for scripting.
	void registerScriptSafeObject( const TCHAR* classId )
	{
		ASSERT_PTR( classId );

		CString implCategKeyPath;
		implCategKeyPath.Format( _T("HKEY_CLASSES_ROOT\\CLSID\\%s\\Implemented Categories\\"), classId );

		for ( unsigned int i = 0; i != COUNT_OF( categoryIds ); ++i )
		{
			CString categoryKeyFullPath = implCategKeyPath + categoryIds[ i ];
			reg::CKey categoryKey( categoryKeyFullPath, true );

			if ( !categoryKey.IsValid() )
				TRACE( _T("registerScriptSafeObject(): Invalid key: %s\n"), categoryKeyFullPath );
		}
	}

	void unregisterClass( const TCHAR* classId )
	{
		ASSERT( classId != NULL && classId[0] != _T('\0') );

		CString classesKeyFullPath = _T("HKEY_CLASSES_ROOT\\CLSID");
		reg::CKey classKey( classesKeyFullPath + _T("\\") + classId, false );

		if ( classKey.IsValid() )
		{
			classKey.RemoveAll();
			classKey.Close();

			reg::CKey classesKey( classesKeyFullPath, false );

			if ( classesKey.IsValid() )
				if ( !classesKey.RemoveSubKey( classId ) )
					TRACE( _T("unregisterClass(): cannot remove class key: %s\n"), classId );
		}
	}

	CString getGuidAsString( REFGUID guid, bool withEnclosingBraces /*= true*/ )
	{
		OLECHAR guidBuffer[ 64 ];

		::StringFromGUID2( guid, guidBuffer, 64 );
		if ( withEnclosingBraces )
			return guidBuffer;
		else
			return CString( guidBuffer + 1, (int)wcslen( guidBuffer ) - 2 );
	}

	struct OleObjectFactory : public COleObjectFactory
	{
		OleObjectFactory( REFCLSID classId, CRuntimeClass* runtimeClass, BOOL multiInstance, const TCHAR* progId );

		const TCHAR* getProgID( void ) const
		{
			return m_lpszProgID;
		}

		static const TCHAR* getProgID( COleObjectFactory& objectFactory )
		{
			return ( (OleObjectFactory&)objectFactory ).getProgID();
		}
	};

	const TCHAR* getFactoryProgID( COleObjectFactory& objectFactory )
	{
		return OleObjectFactory::getProgID( objectFactory );
	}

} // namespace scripting
