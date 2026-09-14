/*
    TMS9918A video modes for SG-1000 cartridges.
    Ported from SMS Plus GX tms.c (C) 1998-2007 Charles MacDonald, GPL v2.
*/

#ifndef _TMS_H_
#define _TMS_H_

/* Fixed TMS9918A palette, RGB888, indexed by the 4-bit colour code */
extern const uint8 tms_palette_rgb[16][3];

/* Render the background of one active line into linebuf */
void tms_render_bg(int line);

/* Render the sprites of one active line into linebuf, updating 5S/C status */
void tms_render_obj(int line);

#endif /* _TMS_H_ */
