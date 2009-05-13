//
//	glyphs.cpp
//
#include <workshop.h>

#include <agg_conv_transform.h>
#include <agg_conv_curve.h>
#include <agg_path_storage.h>
#include <agg_trans_affine.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_rasterizer_outline_aa.h>
#include <agg_renderer_scanline.h>
#include <agg_renderer_outline_aa.h>
#include <agg_rendering_buffer.h>
#include <agg_pixfmt_rgb.h>
#include <agg_pixfmt_gray.h>
#include <agg_scanline_u.h>
#include <agg_span_allocator.h>
#include "agg_span_image_filter_gray.h"
#include "agg_image_accessors.h"

#include <vector>

namespace agg {

//	Missing component; should be added to AGG in some form

   template<class Source, class DestColor> 
    class span_image_colorize
    {
    public:
        typedef Source						source_type;
        typedef DestColor					color_type;
        typedef typename Source::color_type	src_color_type;

        span_image_colorize() : source( NULL ) {}
        span_image_colorize( source_type& src, const color_type& col) :
            source( &src ), color( col ) {}

		void prepare() { source->prepare(); }
        
        void generate(color_type* span, int x, int y, unsigned len)
        {
			src_color_type*	s = allocator.allocate( len );
			source->generate( s, x, y, len ); 
			
			color_type	c = color;
			
			while( len-- ) {
				src_color_type	sc = *s++;
				sc.premultiply();				
				sc.a = sc.v;
				*span = color;
				span->opacity( sc.opacity() );
				++span;
			}
        }
        
    private:
		source_type*					source;  
		span_allocator<src_color_type>	allocator;
		color_type						color;
    };

}

namespace workshop {

using namespace agg;
using namespace std;

//
//	Font engine & cache
//
font_engine						engine_raster, engine_vector;
font_cache_manager<font_engine>	font_mgr_raster( engine_raster ), font_mgr_vector( engine_vector );

bool	init_font()
{
	engine_raster.hinting( true );
	engine_vector.hinting( true );
	engine_raster.create_font( "Times New Roman", agg::glyph_ren_agg_gray8, 20 );
	engine_vector.create_font( "Times New Roman", agg::glyph_ren_outline, 20 );
	return true;
}

//	String iterator for glyph rasterization; x and y are treated differently in outline and raster modes

struct	render_c_string
{
	double	x, y;

	render_c_string( const char* text, font_cache_manager<font_engine>& fm ) : 
		p( text ), font_mgr( fm ), glyph( NULL ), internal_offset( true ), _offset( 0 ), x( 0 ), y( 0 ) {
		render_glyph();
	}

	render_c_string( const char* text, font_cache_manager<font_engine>& fm, double offset ) : 
		p( text ), font_mgr( fm ), glyph( NULL ), internal_offset( false ), _offset( offset ), x( 0 ), y( 0 ) {
		render_glyph();
	}
	
	operator bool() const { return *p != 0; }
	
	void operator++() {
		if( glyph ) {
			x += glyph->advance_x;
			y += glyph->advance_y;
		}
		++p;
		if( *p )	render_glyph();
	}
	
private:

	const char*							p;
	font_cache_manager<font_engine>&	font_mgr;
	const glyph_cache*					glyph;
	bool								internal_offset;
	double								_offset;
	
	void	render_glyph() {
		if( glyph = font_mgr.glyph( *p ) ) {
			font_mgr.add_kerning( &x, &y );
			font_mgr.init_embedded_adaptors( glyph, internal_offset ? x : 0.0, internal_offset ? y : _offset );
		}
	}
};


void	render_image( image_buffer& ib )
{
//	Rendering buffers & blenders

	ib.init( BGR_COLOR, 512, 512 );
	rendering_buffer	buffer( ib.pixels(), ib.width(), ib.height(), ib.stride() );
	pixfmt_bgr24		pfmt( buffer );
	
	vector<unsigned char>	gray_storage( 32 * 32 );
	rendering_buffer	gray_buffer( &gray_storage.front(), 32, 32, 32 );
	pixfmt_gray8		gray_pfmt( gray_buffer );	
	
// Model-view transformations

	trans_affine	xform = trans_affine_scaling( 10, -10 ) * trans_affine_translation( 20, 400 );
	
//	* Leaving space for up to 12 pixels of descent, 20 pixels of ascent	
	
	trans_affine	img_xform = trans_affine_translation( 0, 12 ) * xform;
	trans_affine	img_xform_inv;

	static bool once = init_font();

//	Geometry pipeline

//	* Outline

	conv_curve< font_cache_manager<font_engine>::path_adaptor_type >	curve( font_mgr_vector.path_adaptor() );
	curve.approximation_scale( 0.01 );
	curve.angle_tolerance( 0.1 );

//	* Box for enlarged image -- there ought to be an easier way to do it
	
	conv_transform< conv_curve< font_cache_manager<font_engine>::path_adaptor_type > >	vectors( curve, xform );

	path_storage	square_path;
	square_path.move_to( 0, 0 );
	square_path.line_to( 0, 32 );
	square_path.line_to( 32, 32 );
	square_path.line_to( 32, 0 );
	square_path.move_to( 0, 0 );
	square_path.close_polygon();
	
	conv_transform< path_storage >	square( square_path, img_xform );
		
//	Rasterization/rendering pipelines

	scanline_u8				sl;

//	* Outline

	rasterizer_scanline_aa<>	rasterizer;
	renderer_base<pixfmt_bgr24>	renderer( pfmt );
	
	line_profile_aa				line_profile;
	line_profile.width( 1.0 );	
	
	renderer_outline_aa< renderer_base<pixfmt_bgr24> >	outline_renderer( renderer, line_profile );
	outline_renderer.color( rgba( 0.7, 0.0, 0.0 ) );

//	* Raster

	renderer_base<pixfmt_gray8>			gray_renderer( gray_pfmt );

//	* Enlarged raster
	
	rasterizer_scanline_aa<>			square_rasterizer;
	span_allocator<rgba8>				span_alloc;

//	Image filtering pipeline for scaling the raster

	image_accessor_clip<pixfmt_gray8>	gray_accessor( gray_pfmt, rgba( 0.0, 0.0, 0.0, 0.0 ) );
	span_interpolator_linear<>			scale_interpolator( img_xform_inv );
	
	span_image_filter_gray_nn< image_accessor_clip<pixfmt_gray8>, span_interpolator_linear<> >	scaler( gray_accessor, scale_interpolator );
	span_image_colorize< span_image_filter_gray_nn< image_accessor_clip<pixfmt_gray8>, span_interpolator_linear<> >, rgba8 >	colorizer( scaler, rgba( 0.0, 0.4, 0.0 ) );

//=================================================
//
//	Output sequence
//

	renderer.clear( rgba( 1.0, 1.0, 1.0 ) );
	
	for( render_c_string r( "Angie", font_mgr_raster, 12.0 ); r; ++r ) {

		//	* Render raster glyph

		gray_renderer.clear( 0 );
		render_scanlines_aa_solid( font_mgr_raster.gray8_adaptor(), font_mgr_raster.gray8_scanline(), gray_renderer, rgba( 1.0, 1.0, 1.0 ) );

		double	x, y, rx = r.x, ry = r.y - 12;
		
		//	* Determine enlarged glyph placement
		
		xform.transform( &rx, &ry );
		img_xform.translation( &x, &y );
		img_xform.translate( rx - x, ry - y );
		
		img_xform_inv = img_xform;
		img_xform_inv.invert();
		
		//	* Scale the raster glyph
		
		square_rasterizer.reset();
		square_rasterizer.add_path( square );
		render_scanlines_bin( square_rasterizer, sl, renderer, span_alloc, colorizer );
	}
	
	for( render_c_string r( "Angie", font_mgr_vector ); r; ++r ) {
	
		//	* Render outline
	
		rasterizer_outline_aa< renderer_outline_aa< renderer_base<pixfmt_bgr24> > >	outline_rasterizer( outline_renderer );
		outline_rasterizer.add_path( vectors );
		outline_rasterizer.render( false );
	}
}

}