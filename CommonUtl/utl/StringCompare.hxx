
#include "StringCompare.h"
#include "StdHashValue.h"
#include <unordered_set>


namespace str
{
	namespace ignore_case
	{
		typedef std::unordered_set<std::string, Hash, EqualTo> TUnorderedSet_String;
		typedef std::unordered_set<std::wstring, Hash, EqualTo> TUnorderedSet_WString;
	}
}
