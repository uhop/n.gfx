// helloAGG.cpp : Implementation of WinMain

#include "stdafx.h"
#include "resource.h"
#include "wtlApp_i.h"

#include "workshop.h"

class	wtlWindow : public CWindowImpl< wtlWindow >
{
	typedef CWindowImpl< wtlWindow >	Impl;
	
public:

	void	Create() {
		Impl::Create( NULL, initial_frame_rect, "AGG Workshop", WS_OVERLAPPEDWINDOW );
	}
	
	BEGIN_MSG_MAP(wtlWindow)
	
		MESSAGE_HANDLER( WM_PAINT, OnPaint )
		MESSAGE_HANDLER( WM_ERASEBKGND, OnEraseBkgnd )
	
	END_MSG_MAP()
	
private:

	workshop::image_buffer	image;

	static RECT	initial_frame_rect;
	
	void OnFinalMessage( HWND ) {
		::PostQuitMessage(0);
	}
	
	LRESULT OnPaint( UINT, WPARAM, LPARAM, BOOL& ) {

		workshop::render_image( image );

		PAINTSTRUCT	ps;
		BeginPaint( &ps );

		if( image.valid() ) {
			::SelectClipRgn( ps.hdc, NULL );
			RECT	r;
			GetCenterRect( r );
			image.draw( ps.hdc, r.left, r.top );
		}
		
		EndPaint( &ps );
		return 0;	
	}
	
	LRESULT OnEraseBkgnd( UINT, WPARAM _dc, LPARAM, BOOL& handled ) {
		if( image.valid() ) {
			RECT	r;
			GetCenterRect( r );
			::ExcludeClipRect( HDC(_dc), r.left, r.top, r.right, r.bottom );
		}
		handled = FALSE;
		return 0;
	}
	
	void	GetCenterRect( RECT& r ) {
		GetClientRect( &r );
		int		extra_w = r.right - r.left - int( image.width() ),
				extra_h = r.bottom - r.top - int( image.height() );
		r.left += extra_w/2;
		r.right -= (extra_w+1)/2;
		r.top += extra_h/2;
		r.bottom -= (extra_h+1)/2;
	}
};

RECT wtlWindow::initial_frame_rect = { 200, 200, 1024, 768 };

class CwtlAppModule : public CAtlExeModuleT< CwtlAppModule >
{
public :
	DECLARE_LIBID(LIBID_wtlAppLib)

	HRESULT	PreMessageLoop( UINT cmd ) {
		app_window.Create();
		app_window.ShowWindow( cmd );
		return S_OK;
	}

private:
	wtlWindow	app_window;
};

CwtlAppModule _AtlModule;



//
extern "C" int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, 
                                LPTSTR /*lpCmdLine*/, int nShowCmd)
{
    return _AtlModule.WinMain(nShowCmd);
}

