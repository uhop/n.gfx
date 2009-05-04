//
//	helloAGG.cpp
//
#include <workshop.h>
#include <vector>

#include <agg_path_storage.h>
#include <agg_conv_curve.h>
#include <agg_conv_stroke.h>
#include <agg_conv_dash.h>
#include <agg_conv_transform.h>
#include <agg_trans_affine.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_renderer_scanline.h>
#include <agg_rendering_buffer.h>
#include <agg_pixfmt_rgb.h>
#include <agg_pixfmt_gray.h>
#include <agg_scanline_u.h>
#include <agg_span_interpolator_linear.h>
#include <agg_span_gradient.h>
#include <agg_span_allocator.h>
#include <agg_blur.h>

namespace workshop {

using namespace agg;
using namespace std;

void	render_image( image_buffer& ib )
{
	ib.init( BGR_COLOR, 512, 512 );
	rendering_buffer	buffer( ib.pixels(), ib.width(), ib.height(), ib.stride() );
	pixfmt_bgr24		pfmt( buffer );

	vector<unsigned char>	shadow_storage( ib.width() * ib.height() );
	rendering_buffer	shadow_buffer( &shadow_storage.front(), ib.width(), ib.height(), ib.width() );
	pixfmt_gray8		shadow_pfmt( shadow_buffer );

// Geometry pipeline
	
	path_storage	path;
	
	path.vertices().add_vertex( -1, 0, path_cmd_move_to );
	path.vertices().add_vertex( -1, 1, path_cmd_curve4 );
	path.vertices().add_vertex( 1, -1, path_cmd_curve4 );
	path.vertices().add_vertex( 1, 0, path_cmd_curve4 );
	path.vertices().add_vertex( 1, 1, path_cmd_curve4 );
	path.vertices().add_vertex( -1, -1, path_cmd_curve4 );
	path.vertices().add_vertex( -1, 0, path_cmd_curve4 );
	path.vertices().add_vertex( 0, 0, path_cmd_end_poly  | path_flags_close );
	
	conv_curve< path_storage >	curve_gen( path );
	curve_gen.approximation_scale( 0.1 );
	curve_gen.angle_tolerance( 0.1 );
	
	conv_dash< conv_curve< path_storage > >	dash_gen( curve_gen );
	dash_gen.add_dash( 0.01, 0.3 );
	
	conv_stroke< conv_dash< conv_curve< path_storage > > >	stroke_gen( dash_gen );
	stroke_gen.width( 0.25 );
	stroke_gen.line_cap( round_cap );
	stroke_gen.approximation_scale( 100 );

	trans_affine	xform = trans_affine_scaling( 200 ) * trans_affine_translation( 256, 256 );
	conv_transform< conv_stroke< conv_dash< conv_curve< path_storage > > > >	vectors( stroke_gen, xform );

//	Rasterization & rendering pipeline
	
	rasterizer_scanline_aa<>	rasterizer;
	rasterizer.add_path( vectors );
	
	renderer_base<pixfmt_bgr24>	renderer( pfmt );
	renderer.clear( rgba( 1.0, 1.0, 1.0 ) );

	span_interpolator_linear<>	gradient_interpolator( xform.invert() );
	gradient_radial				gradient_function;
	gradient_linear_color<rgba>	gradient_color( rgba( 0.0, 0.0, 1.0 ), rgba( 0.0, 0.0, 0.5 ) );
	
	span_gradient< rgba8, span_interpolator_linear<>, gradient_radial, gradient_linear_color<rgba> >	gradient( gradient_interpolator, gradient_function, gradient_color, 0.0, 1.0 );
	
//	Shadow rendering

	renderer_base<pixfmt_gray8>	shadow_renderer( shadow_pfmt );
	shadow_renderer.clear( 0 );
	
//	Rendering loop

	scanline_u8				sl;
	span_allocator<rgba8>	span_alloc;
	
	render_scanlines_bin_solid( rasterizer, sl, shadow_renderer, rgba( 1.0, 1.0, 1.0 ) );
	stack_blur_gray8( shadow_pfmt, 10, 10 );
	
	renderer.blend_from_color( shadow_pfmt, rgba( 0.0, 0.3, 0.0 ), 0, 15, 15 );
	render_scanlines_aa( rasterizer, sl, renderer, span_alloc, gradient );	
}

}