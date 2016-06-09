/* wrap libwebp libray for write
 *
 * 6/8/13
 * 	- from vips2jpeg.c
 * 31/5/16
 * 	- buffer write ignored lossless, thanks aaron42net
 * 2/5/16 Felix Bünemann
 * 	- used advanced encoding API, expose controls 
 */

/*

    This file is part of VIPS.
    
    VIPS is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA

 */

/*

    These files are distributed with VIPS - http://www.vips.ecs.soton.ac.uk

 */

/*
#define DEBUG
#define VIPS_DEBUG
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#ifdef HAVE_LIBWEBP

#include <stdlib.h>
#include <string.h>

#include <vips/vips.h>

#include <webp/encode.h>

#include "webp.h"

typedef int (*webp_import)( WebPPicture *picture,
	const uint8_t *rgb, int stride );

static WebPPreset
get_preset( VipsForeignWebpPreset preset )
{
	switch( preset ) {
	case VIPS_FOREIGN_WEBP_PRESET_DEFAULT:
		return( WEBP_PRESET_DEFAULT );
	case VIPS_FOREIGN_WEBP_PRESET_PICTURE:
		return( WEBP_PRESET_PICTURE );
	case VIPS_FOREIGN_WEBP_PRESET_PHOTO:
		return( WEBP_PRESET_PHOTO );
	case VIPS_FOREIGN_WEBP_PRESET_DRAWING:
		return( WEBP_PRESET_DRAWING );
	case VIPS_FOREIGN_WEBP_PRESET_ICON:
		return( WEBP_PRESET_ICON );
	case VIPS_FOREIGN_WEBP_PRESET_TEXT:
		return( WEBP_PRESET_TEXT );

	default:
		g_assert_not_reached();
	}

	/* Keep -Wall happy.
	 */
	return( -1 );
}

typedef struct {
	uint8_t *mem;
	size_t size;
	size_t max_size;
} VipsWebPMemoryWriter;

static void
init_memory_writer( VipsWebPMemoryWriter *writer ) {
	writer->mem = NULL;
	writer->size = 0;
	writer->max_size = 0;
}

static int
memory_write( const uint8_t *data, size_t data_size,
	const WebPPicture *picture ) {
	VipsWebPMemoryWriter * const writer = 
		(VipsWebPMemoryWriter*) picture->custom_ptr;

	size_t next_size;

	if( !writer )
		return( 0 );

	next_size = writer->size + data_size;

	if( next_size > writer->max_size ) {
		uint8_t *new_mem;
		const size_t next_max_size =
			VIPS_MAX( 8192, VIPS_MAX( next_size, 
				writer->max_size * 2 ) );

		if( !(new_mem = (uint8_t*) g_try_malloc( next_max_size )) ) 
			return( 0 );

		if( writer->size > 0 )
			memcpy( new_mem, writer->mem, writer->size );

		g_free( writer->mem );
		writer->mem = new_mem;
		writer->max_size = next_max_size;
	}

	if( data_size > 0 ) {
		memcpy( writer->mem + writer->size, data, data_size );
		writer->size += data_size;
	}

	return( 1 );
}

static int
write_webp( WebPPicture *pic, VipsImage *in,
	int Q, gboolean lossless, VipsForeignWebpPreset preset,
	gboolean smart_subsample, gboolean near_lossless,
	int alpha_q )
{
	VipsImage *memory;
	WebPConfig config;
	webp_import import;

	if ( !WebPConfigPreset( &config, get_preset( preset ), Q ) ) {
		vips_error( "vips2webp",
			"%s", _( "config version error" ) );
		return( -1 );
	}

#if WEBP_ENCODER_ABI_VERSION >= 0x0100
	config.lossless = lossless || near_lossless;
	config.alpha_quality = alpha_q;
	/* Smart subsampling requires use_argb because
	 * it is applied during RGB to YUV conversion.
	 */
	pic->use_argb = lossless || near_lossless || smart_subsample;
#else
	if( lossless || near_lossless )
		vips_warn( "vips2webp", 
			"%s", _( "lossless unsupported" ) );
	if( alpha_q != 100 )
		vips_warn( "vips2webp", 
			"%s", _( "alpha_q unsupported" ) );
#endif

#if WEBP_ENCODER_ABI_VERSION >= 0x0209
	if( near_lossless )
		config.near_lossless = Q;
	if( smart_subsample )
		config.preprocessing |= 4;
#else
	if( near_lossless )
		vips_warn( "vips2webp", 
			"%s", _( "near_lossless unsupported" ) );
	if( smart_subsample )
		vips_warn( "vips2webp", 
			"%s", _( "smart_subsample unsupported" ) );
#endif

	if( !WebPValidateConfig( &config ) ) {
		vips_error( "vips2webp",
			"%s", _( "invalid configuration" ) );
		return( -1 );
	}

	if( !(memory = vips_image_copy_memory( in )) )
		return( -1 );

	pic->width = memory->Xsize;
	pic->height = memory->Ysize;

	if( in->Bands == 4 )
		import = WebPPictureImportRGBA;
	else
		import = WebPPictureImportRGB;

	if( !import( pic, VIPS_IMAGE_ADDR( memory, 0, 0 ),
		VIPS_IMAGE_SIZEOF_LINE( memory ) ) ) {
		VIPS_UNREF( memory );
		vips_error( "vips2webp",
			"%s", _( "picture memory error" ) );
		return( -1 );
	}

	if( !WebPEncode( &config, pic ) ) {
		VIPS_UNREF( memory );
		vips_error( "vips2webp",
			"%s", _( "unable to encode" ) );
		return( -1 );
	}

	VIPS_UNREF( memory );

	return( 0 );
}

int
vips__webp_write_file( VipsImage *in, const char *filename, 
	int Q, gboolean lossless, VipsForeignWebpPreset preset,
	gboolean smart_subsample, gboolean near_lossless,
	int alpha_q )
{
	WebPPicture pic;
	VipsWebPMemoryWriter writer;
	FILE *fp;

	if( !WebPPictureInit( &pic ) ) {
		vips_error( "vips2webp",
			"%s", _( "picture version error" ) );
		return( -1 );
	}

	init_memory_writer( &writer );
	pic.writer = memory_write;
	pic.custom_ptr = &writer;

	if( write_webp( &pic, in, Q, lossless, preset, smart_subsample,
		near_lossless, alpha_q ) ) {
		WebPPictureFree( &pic );
		g_free( writer.mem );
		return -1;
	}

	WebPPictureFree( &pic );

	if( !(fp = vips__file_open_write( filename, FALSE )) ) {
		g_free( writer.mem );
		return( -1 );
	}

	if( vips__file_write( writer.mem, writer.size, 1, fp ) ) {
		fclose( fp );
		g_free( writer.mem );
		return( -1 );
	}

	fclose( fp );
	g_free( writer.mem );

	return( 0 );
}

int
vips__webp_write_buffer( VipsImage *in, void **obuf, size_t *olen, 
	int Q, gboolean lossless, VipsForeignWebpPreset preset,
	gboolean smart_subsample, gboolean near_lossless,
	int alpha_q )
{
	WebPPicture pic;
	VipsWebPMemoryWriter writer;

	if( !WebPPictureInit( &pic ) ) {
		vips_error( "vips2webp", 
			"%s", _( "picture version error" ) );
		return( -1 );
	}

	init_memory_writer( &writer );
	pic.writer = memory_write;
	pic.custom_ptr = &writer;

	if( write_webp( &pic, in, Q, lossless, preset, smart_subsample,
		near_lossless, alpha_q ) ) {
		WebPPictureFree( &pic );
		g_free( writer.mem );
		return -1;
	}

	WebPPictureFree( &pic );

	*obuf = writer.mem;
	*olen = writer.size;

	return( 0 );
}

#endif /*HAVE_LIBWEBP*/
