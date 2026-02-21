#ifndef MultiValueBase_h
#define MultiValueBase_h
#pragma once


class CEnumTags;


namespace multi
{
	enum MultiValueState { NullValue, SharedValue, MultipleValue };

	const CEnumTags& GetTags_MultiValueState( void );
	const std::tstring& GetMultipleValueTag( void );
}


namespace ui
{
	// abstract base for controls that optionally display multiple value mode using the "<different values>" tag.
	//
	abstract class CMultiValueBase
	{
	protected:
		CMultiValueBase( void );

		enum Metrics { HorizEdge = 4, TextFmt = DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX };

		void DrawMultiValueText( CDC* pDC, const CWnd* pCtrl, HBRUSH hBkBrush = ::GetSysColorBrush( COLOR_WINDOW ), UINT dtFmt = TextFmt );
	public:
		const std::tstring& GetMultiValueTag( void ) const { return m_multiValueTag; }
		void SetMultiValueTag( const std::tstring& multiValueTag ) { m_multiValueTag = multiValueTag; }

		virtual bool InMultiValuesMode( void ) const = 0;		// current contents are multiple values?
	private:
		std::tstring m_multiValueTag;		// e.g. "<different values>"
	};
}


#endif // MultiValueBase_h
