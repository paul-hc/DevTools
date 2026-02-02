#ifndef SubjectAdapter_h
#define SubjectAdapter_h
#pragma once

#include "utl/ISubject.h"
#include <shobjidl_core.h>


namespace ui
{
	interface ISubjectAdapter
	{
		virtual std::tstring FormatCode( const utl::ISubject* pSubject ) const = 0;
	};


	class CCodeAdapter : public ui::ISubjectAdapter
	{
	public:
		static ui::ISubjectAdapter* Instance( void );

		// ui::ISubjectAdapter interface
		virtual std::tstring FormatCode( const utl::ISubject* pSubject ) const;
	};


	class CDisplayCodeAdapter : public ui::CCodeAdapter
	{
	public:
		static ui::ISubjectAdapter* Instance( void );

		// ui::ISubjectAdapter interface
		virtual std::tstring FormatCode( const utl::ISubject* pSubject ) const;
	};


	class CPathPidlAdapter : public ui::ISubjectAdapter
	{
	public:
		CPathPidlAdapter( SIGDN pidlDisplayNameType ) : m_pidlDisplayNameType( pidlDisplayNameType ) {}

		static ui::ISubjectAdapter* InstanceUI( void );			// SIGDN_DESKTOPABSOLUTEEDITING: "Control Panel\\All Control Panel Items\\Region"
		static ui::ISubjectAdapter* InstanceParsing( void );	// SIGDN_DESKTOPABSOLUTEPARSING: "::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"
		static ui::ISubjectAdapter* InstanceDisplay( void );	// SIGDN_NORMALDISPLAY: "Region"

		// ui::ISubjectAdapter interface
		virtual std::tstring FormatCode( const utl::ISubject* pSubject ) const;
	private:
		SIGDN m_pidlDisplayNameType;
	};


	inline ui::ISubjectAdapter* GetFullPathAdapter( void ) { return ui::CCodeAdapter::Instance(); }
}


#endif // SubjectAdapter_h
