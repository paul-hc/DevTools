#ifndef Application_fwd_h
#define Application_fwd_h
#pragma once


class CEnumTags;


namespace app
{
	enum Action { RenameFiles, TouchFiles };

	const CEnumTags& GetTags_Action( void );
}


#endif // Application_fwd_h
