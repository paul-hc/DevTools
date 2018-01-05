#ifndef MenuUtilitiesOLD_h
#define MenuUtilitiesOLD_h
#pragma once


struct MenuItem
{
	MenuItem( void );
	~MenuItem();

	bool isValidCommand( void ) const;
	bool build( HMENU _hPopup, int _index );
	void dump( void );
public:
	HMENU hPopup;
	int index;
	int ID;
	UINT state;
};


namespace menu
{
	void addSubPopups( std::vector< HMENU >& rOutPopups, HMENU hMenu );
	UINT getItemFromPosID( MenuItem& rOutFoundItem, const CPoint& screenPos, const std::vector< HMENU >& rPopups );
	HWND getMenuWindowFromPoint( CPoint screenPos = CPoint( -1, -1 ) );
}


#endif // MenuUtilitiesOLD_h
