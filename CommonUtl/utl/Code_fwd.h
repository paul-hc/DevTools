#ifndef Code_fwd_h
#define Code_fwd_h
#pragma once


class CRuntimeException;


namespace code
{
	typedef CRuntimeException TSyntaxError;


	enum Spacing { TrimSpace, AddSpace, RetainSpace };
}


#endif // Code_fwd_h
