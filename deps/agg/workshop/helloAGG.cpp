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
#include <agg_pixfmt_amask_adaptor.h>
#include <agg_alpha_mask_u8.h>

namespace workshop {

using namespace agg;
using namespace std;

void	render_image( image_buffer& ib )
{
//	Rendering buffers & blenders

	ib.init( BGR_COLOR, 512, 512 );
	rendering_buffer	buffer( ib.pixels(), ib.width(), ib.height(), ib.stride() );
	pixfmt_bgr24		pfmt( buffer );

	vector<unsigned char>	gray_storage( ib.width() * ib.height() );
	rendering_buffer	gray_buffer( &gray_storage.front(), ib.width(), ib.height(), ib.width() );
	pixfmt_gray8		gray_pfmt( gray_buffer );

// Model-view transformations

	trans_affine	xform = trans_affine_scaling( 200 ) * trans_affine_translation( 256, 256 );
	trans_affine	inv_xform = xform;
	inv_xform.invert();

//=================================================
//
// Geometry pipelines

//	* Common part
	
	path_storage	path;
	
	conv_curve< path_storage >	curve_gen( path );
	curve_gen.approximation_scale( 0.1 );
	curve_gen.angle_tolerance( 0.1 );

//	* Object: dash + outline

	conv_dash< conv_curve< path_storage > >	dash_gen( curve_gen );
	dash_gen.add_dash( 0.01, 0.3 );
	
	conv_stroke< conv_dash< conv_curve< path_storage > > >	stroke_gen( dash_gen );
	stroke_gen.width( 0.25 );
	stroke_gen.line_cap( round_cap );
	stroke_gen.approximation_scale( 100 );
	
	conv_transform< conv_stroke< conv_dash< conv_curve< path_storage > > > >	vectors( stroke_gen, xform );

//	* Clipping: outline only

	conv_stroke< conv_curve< path_storage > >	clip_stroke_gen( curve_gen );
	clip_stroke_gen.width( 0.1 );

	conv_transform< conv_stroke< conv_curve< path_storage > > >	clip_vectors( clip_stroke_gen, xform );

//	Rasterization/rendering pipelines

	scanline_u8				sl;
	span_allocator<rgba8>	span_alloc;

//	* Gradient for the object

	span_interpolator_linear<>	gradient_interpolator( inv_xform );
	gradient_radial				gradient_function;
	gradient_linear_color<rgba>	gradient_color( rgba( 0.0, 0.0, 1.0 ), rgba( 0.0, 0.0, 0.5 ) );
	
	span_gradient< rgba8, span_interpolator_linear<>, gradient_radial, gradient_linear_color<rgba> >	gradient( gradient_interpolator, gradient_function, gradient_color, 0.0, 1.0 );

//	* Common for object & shadow

	rasterizer_scanline_aa<>	rasterizer;
	
//	* Clip

	rasterizer_scanline_aa<>	clip_rasterizer;

//	* Common for shadow and clip

	renderer_base<pixfmt_gray8>	gray_renderer( gray_pfmt );
	
//	* Shadow (blending into main buffer)	
	
	renderer_base<pixfmt_bgr24>	renderer( pfmt );

//	* Object

	amask_no_clip_gray8															amask( gray_buffer );
	pixfmt_amask_adaptor< pixfmt_bgr24, amask_no_clip_gray8 >					pfmt_masked( pfmt, amask );
	renderer_base< pixfmt_amask_adaptor< pixfmt_bgr24, amask_no_clip_gray8 > >	renderer_masked( pfmt_masked );

//=================================================
//
//	Output sequence
//
//	* Populate the path (infinity curve)

	path.vertices().add_vertex( -1, 0, path_cmd_move_to );
	path.vertices().add_vertex( -1, 1, path_cmd_curve4 );
	path.vertices().add_vertex( 1, -1, path_cmd_curve4 );
	path.vertices().add_vertex( 1, 0, path_cmd_curve4 );
	path.vertices().add_vertex( 1, 1, path_cmd_curve4 );
	path.vertices().add_vertex( -1, -1, path_cmd_curve4 );
	path.vertices().add_vertex( -1, 0, path_cmd_curve4 );
	path.vertices().add_vertex( 0, 0, path_cmd_end_poly  | path_flags_close );
	
//	* Rasterize object

	rasterizer.add_path( vectors );	
	
//	* Render shadow
	
	gray_renderer.clear( 0 );
	render_scanlines_bin_solid( rasterizer, sl, gray_renderer, rgba( 1.0, 1.0, 1.0 ) );
	stack_blur_gray8( gray_pfmt, 10, 10 );

//	* Blend shadow
	
	renderer.clear( rgba( 1.0, 1.0, 1.0 ) );
	renderer.blend_from_color( gray_pfmt, rgba( 0.0, 0.3, 0.0 ), 0, 15, 15 );

//	* Rasterize clip mask

	clip_rasterizer.add_path( clip_vectors );

//	* Render clip mask

	gray_renderer.clear( gray8( 255 ) );
	render_scanlines_aa_solid( clip_rasterizer, sl, gray_renderer, gray8( 0 ) );

//	* Render object through the clip mask

	render_scanlines_aa( rasterizer, sl, renderer_masked, span_alloc, gradient );	
}

}