#ifndef SafeForScripting_h
#define SafeForScripting_h
#pragma once


namespace scripting
{
	enum RegAction { Register, Unregister };

	void registerAllScriptObjects( RegAction regAction );
}


#endif // SafeForScripting_h
