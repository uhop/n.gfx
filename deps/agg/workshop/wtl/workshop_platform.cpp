//
//	workshop_platform.cpp
//
#include <workshop.h>
#include <atlbase.h>

namespace workshop {

inline RGBQUAD	make_RGBQUAD( unsigned r, unsigned g, unsigned b, unsigned a = 255 )
{
	RGBQUAD	v = { b, g, r, a };
	return v;
}

struct	image_buffer_info : public DIBSECTION
{
	image_buffer_info( HGDIOBJ h ) {
		::GetObject( h, sizeof(DIBSECTION), static_cast<DIBSECTION*>( this ) );
	}
};

struct	image_buffer_params : public BITMAPINFO
{
	RGBQUAD	more_colors[255];
	
	image_buffer_params( unsigned, unsigned, unsigned );
};

image_buffer_params::image_buffer_params( unsigned pfmt, unsigned w, unsigned h )
{
	ATLASSERT( (pfmt & ~BGR_COLOR_ORDER) != RGB_COLOR || (pfmt & BGR_COLOR_ORDER) != 0 );
	memset( this, 0, sizeof(*this) );
	bmiHeader.biWidth = w;
	bmiHeader.biHeight = -int( h );
	bmiHeader.biBitCount = pfmt & ~BGR_COLOR_ORDER;
	bmiHeader.biPlanes = 1;
	
	bmiHeader.biSize = unsigned( sizeof(BITMAPINFOHEADER) );

	if( pfmt == MONOCHROME ) {
		bmiHeader.biClrImportant = bmiHeader.biClrUsed = 2;
		bmiColors[1] = make_RGBQUAD( 255, 255, 255 );
	} else if( pfmt == GRAYSCALE ) {
		bmiHeader.biClrImportant = bmiHeader.biClrUsed = 256;
		for( unsigned i=0; i<256; ++i )	
			bmiColors[i] = make_RGBQUAD( i, i, i );
	}

	bmiHeader.biSize += unsigned( sizeof(RGBQUAD)*bmiHeader.biClrUsed );
}

image_buffer::~image_buffer()
{
	if( _bitmap )	::DeleteObject( _bitmap );
}

unsigned	image_buffer::width() const
{
	image_buffer_info	info( _bitmap );
	return info.dsBmih.biWidth;
}

unsigned	image_buffer::height() const
{
	image_buffer_info	info( _bitmap );
	return abs( info.dsBmih.biHeight );
}

unsigned	image_buffer::stride() const
{
	image_buffer_info	info( _bitmap );
	return 4 * ( 1 + ( info.dsBmih.biBitCount * info.dsBmih.biWidth - 1 ) / 32 );
}
	
void	image_buffer::init( unsigned pfmt, unsigned w, unsigned h )
{
	if( _bitmap ) {
		image_buffer_info	info( _bitmap );
		if( info.dsBmih.biBitCount == (pfmt & ~BGR_COLOR_ORDER) && info.dsBmih.biWidth == w && info.dsBmih.biHeight == h )
			return;
		::DeleteObject( _bitmap );
		_data = NULL;
		_bitmap = NULL;
	}

	image_buffer_params	params( pfmt, w, h );
	
	HWND	desktop = ::GetDesktopWindow();
	HDC		dc = ::GetDC( desktop );
	
	_bitmap = ::CreateDIBSection( dc, &params, DIB_RGB_COLORS, &_data, NULL, 0 );
	
	::ReleaseDC( desktop, dc );
}
	
void	image_buffer::draw( HDC dc, int x, int y )
{
	image_buffer_info	info( _bitmap );
	HDC	src_dc = ::CreateCompatibleDC( dc );
	HGDIOBJ old_bmp = ::SelectObject( src_dc, _bitmap );
	::BitBlt( dc, x, y, info.dsBmih.biWidth, abs(info.dsBmih.biHeight), src_dc, 0, 0, SRCCOPY );
	::SelectObject( src_dc, old_bmp );
	::DeleteDC( src_dc );
}

}