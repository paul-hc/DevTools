
#include "pch.h"
#include "Subject.h"
#include "Path.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace dbg
{
#ifdef _DEBUG

	const std::tstring GetSafeFileName( const utl::ISubject* pItem )
	{
		return pItem != nullptr ? path::FindFilename( pItem->GetCode().c_str() ) : str::GetEmpty();
	}

#endif
}
