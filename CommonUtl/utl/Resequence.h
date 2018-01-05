#ifndef Resequence_h
#define Resequence_h
#pragma once


namespace seq
{
	enum Direction { Prev = -1, Next = 1 };
	enum MoveTo { MovePrev = -1, MoveNext = 1, MoveToStart = INT_MIN, MoveToEnd = INT_MAX };

} // namespace seq


#endif // Resequence_h
