#ifndef RegAutomationSvr_h
#define RegAutomationSvr_h
#pragma once


namespace ole
{
	enum RegAction { Register, Unregister };


	std::wstring FormatGUID( const GUID& guid );
	std::tstring GetProgID( const COleObjectFactory* pFactory );


	class CSafeForScripting
	{
	public:
		// register automation objects in component categories in order to make them safe for scripting for all object factories
		static size_t UpdateRegistryAll( RegAction action = ole::Register );
	private:
		static bool UpdateRegistry( const CLSID& coClassId, RegAction action );
		static bool RegisterCoClass( const TCHAR* pCoClassIdTag );
		static bool UnregisterCoClass( const TCHAR* pCoClassIdTag );
	private:
		static const TCHAR* s_categoryIds[];
	};
}


#endif // RegAutomationSvr_h
