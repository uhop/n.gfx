//
//	workshop_platform.h -- Win32-specific workshop code
//
#ifndef	_workshop_platform_h_
#define	_workshop_platform_h_

#include <windows.h>
#include <../font_win32_tt/agg_font_win32_tt.h>

namespace workshop {

class	image_buffer
{
public:

	image_buffer() : _data( NULL ), _bitmap( NULL ) {}
	~image_buffer();
	
	bool	valid() const { return _data != NULL; }
	
//	Portable API	
	
	unsigned char*	pixels() const { return static_cast<unsigned char*>( _data ); }
	unsigned	width() const;
	unsigned	height() const;
	unsigned	stride() const;
	
	void	init( unsigned, unsigned, unsigned );
	
//	Platform-specific API

	void	draw( HDC, int, int );
	
private:

	HBITMAP		_bitmap;
	void*		_data;
};

struct	desktop_dc
{
	desktop_dc() : wnd( ::GetDesktopWindow() ),	dc( ::GetDC( wnd ) ) {}
	
	operator HDC() const { return dc; }
	
	~desktop_dc() {
		::ReleaseDC( wnd, dc );
	}

private:
	HWND	wnd;
	HDC		dc;
};

struct font_engine : 
	private desktop_dc,
	public agg::font_engine_win32_tt_int32
{
	font_engine() : agg::font_engine_win32_tt_int32( operator HDC() ) {}
};

}

#endif