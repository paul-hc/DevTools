#ifndef VisualThemeFallback_h
#define VisualThemeFallback_h
#pragma once

#include <unordered_map>
#include "StdHashValue.h"


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
	typedef int TPartId;		// vt::BP_PUSHBUTTON
	typedef int TStateId;		// vt::PBS_HOT
	typedef UINT TCtrlType;		// DFC_BUTTON
	typedef UINT TState;		// DFCS_BUTTONPUSH | DFCS_HOT

	typedef std::pair<std::wstring, TPartId> TClassKey;
	typedef std::pair<TStateId, TState> TStatePair;
	typedef std::vector<TStatePair> TStateSet;

	struct CCtrlStates
	{
		TCtrlType m_ctrlType;
		TStateSet m_stateSet;
	};

	TStateSet* AddClassPart( const TClassKey& classKey, TCtrlType ctrlType );

	void RegisterButtonClass( void );
	void RegisterMenuClass( void );
	void RegisterComboClass( void );
	void RegisterScrollClass( void );
	void RegisterCaptionClass( void );

	// custom draw background fallback functions
	typedef bool (*TCustomDrawBkFunc)( int stateId, HDC hdc, const RECT& rect );

	static bool CustomDrawBk_MenuBackground( int stateId, HDC hdc, const RECT& rect );
	static bool CustomDrawBk_MenuItem( int stateId, HDC hdc, const RECT& rect );
	static bool CustomDrawBk_MenuChecked( int stateId, HDC hdc, const RECT& rect );
private:
	typedef std::unordered_map<TClassKey, CCtrlStates, utl::CPairHasher> TClassToStateMap;
	typedef std::unordered_map<TClassKey, TCustomDrawBkFunc, utl::CPairHasher> TClassToCustomBkMap;

	TClassToStateMap m_classToStateMap;
	TClassToCustomBkMap m_classToCustomBkMap;
};


#endif // VisualThemeFallback_h
