#ifndef VisualThemeFallback_h
#define VisualThemeFallback_h
#pragma once

#include <hash_map>
#include "ContainerUtilities.h"


class CVisualThemeFallback
{
private:
	CVisualThemeFallback( void );
	~CVisualThemeFallback();
public:
	static CVisualThemeFallback& Instance( void );

	bool DrawBackground( const wchar_t* pClass, int partId, int stateId, HDC hdc, const RECT& rect );			// DrawFrameControl
	bool DrawEdge( HDC hdc, const RECT& rect, UINT edge, UINT flags );
private:
	typedef int PartId;			// vt::BP_PUSHBUTTON
	typedef int StateId;		// vt::PBS_HOT
	typedef UINT CtrlType;		// DFC_BUTTON
	typedef UINT State;			// DFCS_BUTTONPUSH | DFCS_HOT

	typedef std::pair< std::wstring, PartId > ClassKey;
	typedef std::pair< StateId, State > StatePair;
	typedef std::vector< StatePair > StateSet;

	struct CCtrlStates
	{
		CtrlType m_ctrlType;
		StateSet m_stateSet;
	};

	StateSet* AddClassPart( const ClassKey& classKey, CtrlType ctrlType );

	void RegisterButtonClass( void );
	void RegisterMenuClass( void );
	void RegisterComboClass( void );
	void RegisterScrollClass( void );
	void RegisterCaptionClass( void );

	// custom draw background fallback functions
	typedef bool (*CustomDrawBkFunc)( int stateId, HDC hdc, const RECT& rect );

	static bool CustomDrawBk_MenuBackground( int stateId, HDC hdc, const RECT& rect );
	static bool CustomDrawBk_MenuItem( int stateId, HDC hdc, const RECT& rect );
	static bool CustomDrawBk_MenuChecked( int stateId, HDC hdc, const RECT& rect );
private:
	stdext::hash_map< ClassKey, CCtrlStates > m_classToStateMap;
	stdext::hash_map< ClassKey, CustomDrawBkFunc > m_classToCustomBkMap;
};


#endif // VisualThemeFallback_h
