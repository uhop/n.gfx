//
//	workshop.h -- interface between the workshop demo and the platform-specific host app
//
#ifndef	_workshop_h_
#define	_workshop_h_

//	Included from one of platform directories
#include <workshop_platform.h>

namespace workshop {

enum pixel_format { 
	MONOCHROME=1, 
	GRAYSCALE=8, 
	RGB_COLOR = 24,
	RGBA_COLOR = 32,
	BGR_COLOR=24|256, 
	BGRA_COLOR=32|256,
	BGR_COLOR_ORDER = 256 
};

//	Implemented by the demo code
void	render_image( image_buffer& );

}

#endif