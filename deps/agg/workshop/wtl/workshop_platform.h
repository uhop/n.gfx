//
//	workshop_platform.h -- Win32-specific workshop code
//
#ifndef	_workshop_platform_h_
#define	_workshop_platform_h_

#include <windows.h>

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

}

#endif