#ifndef ImageStore_fwd_h
#define ImageStore_fwd_h
#pragma once


namespace ui
{
	struct CStripBtnInfo
	{
		CStripBtnInfo( UINT cmdId = 0, int imagePos = -1 ) : m_cmdId( cmdId ), m_imagePos( imagePos ) {}
	public:
		UINT m_cmdId;
		int m_imagePos;
	};


	class CToolbarDescr : private utl::noncopyable
	{
	public:
		CToolbarDescr( UINT toolbarId, const UINT buttonIds[] = nullptr, size_t buttonCount = 0 );

		UINT GetToolbarId( void ) const { return m_toolbarId; }
		const std::tstring& GetToolbarTitle( void ) const { return m_toolbarTitle; }
		const std::vector<ui::CStripBtnInfo>& GetBtnInfos( void ) const { return m_btnInfos; }

		const ui::CStripBtnInfo* FindBtnInfo( UINT cmdId ) const;
		bool ContainsBtn( UINT cmdId ) const { return FindBtnInfo( cmdId ) != nullptr; }

		void StoreBtnInfos( const UINT buttonIds[], size_t buttonCount );
		void Clear( void ) { m_toolbarId = 0; m_toolbarTitle.clear(); m_btnInfos.clear(); }
	private:
		UINT m_toolbarId;
		std::tstring m_toolbarTitle;
		std::vector<ui::CStripBtnInfo> m_btnInfos;		// excluding separators
	};
}


namespace utl
{
	struct CStripBtnInfoHasher		// use with std::unordered_map<> for pairs
	{
		size_t operator()( const ui::CStripBtnInfo& stripBtnInfo ) const;
	};
}


namespace pred
{
	struct HasCmdId : public std::unary_function<ui::CStripBtnInfo, bool>
	{
		HasCmdId( UINT cmdId ) : m_cmdId( cmdId ) {}

		bool operator()( const ui::CStripBtnInfo& btnInfo ) const
		{
			return m_cmdId == btnInfo.m_cmdId;
		}

		bool operator()( const ui::CToolbarDescr* pToolbarDescr ) const
		{
			ASSERT_PTR( pToolbarDescr );
			return pToolbarDescr->ContainsBtn( m_cmdId );
		}
	private:
		UINT m_cmdId;
	};
}


#endif // ImageStore_fwd_h
