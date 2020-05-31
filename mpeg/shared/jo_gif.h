/* public domain, Simple, Minimalistic GIF writer - http://jonolick.com
 *
 * Quick Notes:
 * 	Supports only 4 component input, alpha is currently ignored. (RGBX)
 *
 * Latest revisions:
 * 	1.00 (2015-11-03) initial release
 *
 * Basic usage:
 *	char *frame = new char[128*128*4]; // 4 component. RGBX format, where X is unused 
 *	jo_gif_t gif = jo_gif_start("foo.gif", 128, 128, 0, 32); 
 *	jo_gif_frame(&gif, frame, 4, false); // frame 1
 *	jo_gif_frame(&gif, frame, 4, false); // frame 2
 *	jo_gif_frame(&gif, frame, 4, false); // frame 3, ...
 *	jo_gif_end(&gif);
 * */

#ifndef JO_INCLUDE_GIF_H
#define JO_INCLUDE_GIF_H

#include <stdio.h>

// To get a header file for this, either cut and paste the header,
// or create jo_gif.h, #define JO_GIF_HEADER_FILE_ONLY, and
// then include jo_gif.cpp from it.

// STEVE CHANGE
#include "jo_file.h"

extern void jo_write_mpeg(/*FILE *fp*/JO_File * fp, unsigned char *rgbx, int width, int height, int fps); // not really GIF-related, just put in same file
extern bool jo_write_jpg(/*const char *filename*/JO_File * fp, const void *data, int width, int height, int comp, int quality);	// ...ditto
// /STEVE CHANGE

typedef struct {
// STEVE CHANGE
	// FILE *fp;
	JO_FileAlloc * gf;
// /STEVE CHANGE
	unsigned char palette[0x300];
	short width, height, repeat;
	int numColors, palSize;
	int frame;
} jo_gif_t;

// width/height	| the same for every frame
// repeat       | 0 = loop forever, 1 = loop once, etc...
// palSize		| must be power of 2 - 1. so, 255 not 256.
extern jo_gif_t jo_gif_start(/*const char *filename*/JO_FileAlloc * gf, short width, short height, short repeat, int palSize);	// <- STEVE CHANGE

// gif			| the state (returned from jo_gif_start)
// rgba         | the pixels
// delayCsec    | amount of time in between frames (in centiseconds)
// localPalette | true if you want a unique palette generated for this frame (does not effect future frames)
extern void jo_gif_frame(jo_gif_t *gif, unsigned char *rgba, short delayCsec, bool localPalette);

// gif          | the state (returned from jo_gif_start)
extern void jo_gif_end(jo_gif_t *gif);

#endif
