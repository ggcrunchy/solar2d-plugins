/*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* [ MIT license: http://www.opensource.org/licenses/mit-license.php ]
*/

#include "CoronaLua.h"
#include "utils/LuaEx.h"

#define CANVAS_ITY_IMPLEMENTATION
#include "canvas_ity.hpp"

//
//
//

#define CANVAS_ITY_METATABLE_NAME "metatable.canvas_ity"

//
//
//

static canvas_ity::canvas & Get (lua_State * L)
{
    return *(canvas_ity::canvas *)luaL_checkudata(L, 1, CANVAS_ITY_METATABLE_NAME);
}

//
//
//

#define ENUM(x) canvas_ity::x
#define NAME(x) #x

//
//
//

canvas_ity::composite_operation GetCompositeOperation (lua_State * L, int arg)
{
    #define COMPOSITE_OPS(OP) OP(source_in), OP(source_copy), OP(source_out), OP(destination_in), \
                                OP(destination_atop), OP(lighter), OP(destination_over), OP(destination_out), \
                                OP(source_atop), OP(source_over), OP(exclusive_or)

    const char * names[] = { COMPOSITE_OPS(NAME), nullptr };
    canvas_ity::composite_operation ops[] = { COMPOSITE_OPS(ENUM) };

    return ops[luaL_checkoption(L, arg, nullptr, names)];
}

canvas_ity::cap_style GetCapStyle (lua_State * L, int arg)
{
    #define CAP_STYLES(OP) OP(butt), OP(square), OP(circle)

    const char * names[] = { CAP_STYLES(NAME), nullptr };
    canvas_ity::cap_style styles[] = { CAP_STYLES(ENUM) };

    return styles[luaL_checkoption(L, arg, nullptr, names)];
}

canvas_ity::join_style GetJoinStyle (lua_State * L, int arg)
{
    #define JOIN_STYLES(OP) OP(miter), OP(bevel), OP(rounded)

    const char * names[] = { JOIN_STYLES(NAME), nullptr };
    canvas_ity::join_style styles[] = { JOIN_STYLES(ENUM) };

    return styles[luaL_checkoption(L, arg, nullptr, names)];
}

canvas_ity::brush_type GetBrushType (lua_State * L, int arg)
{
    #define BRUSH_TYPES(OP) OP(fill_style), OP(stroke_style)

    const char * names[] = { BRUSH_TYPES(NAME), nullptr };
    canvas_ity::brush_type types[] = { BRUSH_TYPES(ENUM) };

    return types[luaL_checkoption(L, arg, nullptr, names)];
}

canvas_ity::repetition_style GetRepetitionStyle (lua_State * L, int arg)
{
    #define REPETITION_STYLES(OP) OP(repeat), OP(repeat_x), OP(repeat_y), OP(no_repeat)

    const char * names[] = { REPETITION_STYLES(NAME), nullptr };
    canvas_ity::repetition_style styles[] = { REPETITION_STYLES(ENUM) };

    return styles[luaL_checkoption(L, arg, nullptr, names)];
}

canvas_ity::align_style GetAlignStyle (lua_State * L, int arg)
{
    #define ALIGN_STYLES(OP) OP(leftward), OP(rightward), OP(center), OP(start), OP(ending)

    const char * names[] = { ALIGN_STYLES(NAME), nullptr };
    canvas_ity::align_style styles[] = { ALIGN_STYLES(ENUM) };

    return styles[luaL_checkoption(L, arg, nullptr, names)];
}

canvas_ity::baseline_style GetBaselineStyle (lua_State * L, int arg)
{
    #define BASELINE_STYLES(OP) OP(alphabetic), OP(top), OP(middle), OP(bottom), OP(hanging), OP(ideographic)

    const char * names[] = { BASELINE_STYLES(NAME), nullptr };
    canvas_ity::baseline_style styles[] = { BASELINE_STYLES(ENUM) };

    return styles[luaL_checkoption(L, arg, nullptr, names)];

}
   
//
//
//

CORONA_EXPORT int luaopen_plugin_canvasity (lua_State* L)
{
    luaL_Reg funcs[] = {
        {
            "New", [](lua_State * L)
            {
                int w = luaL_checkint(L, 1), h = luaL_checkint(L, 2);

                canvas_ity::canvas * canvas = LuaXS::NewTyped<canvas_ity::canvas>(L, w, h); // w, h, canvas

                LuaXS::AttachMethods(L, CANVAS_ITY_METATABLE_NAME, [](lua_State * L) {
                    luaL_Reg funcs[] = {
/*
    /// @brief  Add a color stop to a linear or radial gradient.
    ///
    /// Each color stop has an offset which defines its position from 0.0 at
    /// the start of the gradient to 1.0 at the end.  Colors and opacity are
    /// linearly interpolated along the gradient between adjacent pairs of
    /// stops without premultiplying the alpha.  If more than one stop is
    /// added for a given offset, the first one added is considered closest
    /// to 0.0 and the last is closest to 1.0.  If no stop is at offset 0.0
    /// or 1.0, the stops with the closest offsets will be extended.  If no
    /// stops are added, the gradient will be fully transparent black.  Set a
    /// new linear or radial gradient to clear all the stops and redefine the
    /// gradient colors.  Color stops may be added to a gradient at any time.
    /// The color and opacity values will be clamped to the 0.0 to 1.0 range,
    /// inclusive.  The offset must be in the 0.0 to 1.0 range, inclusive.
    /// If it is not, or if chosen style type is not currently set to a
    /// gradient, this does nothing.
    ///
    /// @param type    whether to add to the fill_style or stroke_style
    /// @param offset  position of the color stop along the gradient
    /// @param red     sRGB red component of the color stop
    /// @param green   sRGB green component of the color stop
    /// @param blue    sRGB blue component of the color stop
    /// @param alpha   opacity of the color stop (not premultiplied)
    ///
    void add_color_stop(
        brush_type type,
        float offset,
        float red,
        float green,
        float blue,
        float alpha );

    /// @brief  Extend the current subpath with an arc between two angles.
    ///
    /// The arc is from the circle centered at the given point and with the
    /// given radius.  A straight line will be added from the current end
    /// point to the starting point of the arc (unless the current path is
    /// empty), then the arc along the circle from the starting angle to the
    /// ending angle in the given direction will be added.  If they are more
    /// than two pi radians apart in the given direction, the arc will stop
    /// after one full circle.  The point at the ending angle will become
    /// the new end point of the path.  Initially, the angles are clockwise
    /// relative to the x-axis.  The current transform at the time that
    /// this is called will affect the passed in point, angles, and arc.
    /// The radius must be non-negative.
    ///
    /// @param x                  horizontal coordinate of the circle center
    /// @param y                  vertical coordinate of the circle center
    /// @param radius             radius of the circle containing the arc
    /// @param start_angle        radians clockwise from x-axis to begin
    /// @param end_angle          radians clockwise from x-axis to end
    /// @param counter_clockwise  true if the arc turns counter-clockwise
    ///
    void arc(
        float x,
        float y,
        float radius,
        float start_angle,
        float end_angle,
        bool counter_clockwise = false );
            
    /// @brief  Extend the current subpath with an arc tangent to two lines.
    ///
    /// The arc is from the circle with the given radius tangent to both
    /// the line from the current end point to the vertex, and to the line
    /// from the vertex to the given point.  A straight line will be added
    /// from the current end point to the first tangent point (unless the
    /// current path is empty), then the shortest arc from the first to the
    /// second tangent points will be added.  The second tangent point will
    /// become the new end point.  If the radius is large, these tangent
    /// points may fall outside the line segments.  The current transform
    /// at the time that this is called will affect the passed in points
    /// and the arc.  If the current path was empty, this is equivalent to
    /// a move.  If the arc would be degenerate, it is equivalent to a line
    /// to the vertex point.  The radius must be non-negative.  If it is not,
    /// or if the current transform is not invertible, this does nothing.
    ///
    /// Tip: to draw a polygon with rounded corners, call this once for each
    ///      vertex and pass the midpoint of the adjacent edge as the second
    ///      point; this works especially well for rounded boxes.
    ///
    /// @param vertex_x  horizontal coordinate where the tangent lines meet
    /// @param vertex_y  vertical coordinate where the tangent lines meet
    /// @param x         a horizontal coordinate on the second tangent line
    /// @param y         a vertical coordinate on the second tangent line
    /// @param radius    radius of the circle containing the arc
    ///
    void arc_to(
        float vertex_x,
        float vertex_y,
        float x,
        float y,
        float radius );

    /// @brief  Reset the current path.
    ///
    /// The current path and all subpaths will be cleared after this, and a
    /// new path can be built.
    ///
    void begin_path();

    /// @brief  Extend the current subpath with a cubic Bezier curve.
    ///
    /// The curve will go from the current end point (or the first control
    /// point if the current path is empty) to the given point, which will
    /// become the new end point.  The curve will be tangent toward the first
    /// control point at the beginning and tangent toward the second control
    /// point at the end.  The current transform at the time that this is
    /// called will affect all points passed in.
    ///
    /// Tip: to make multiple curves join smoothly, ensure that each new end
    ///      point is on a line between the adjacent control points.
    ///
    /// @param control_1_x  horizontal coordinate of the first control point
    /// @param control_1_y  vertical coordinate of the first control point
    /// @param control_2_x  horizontal coordinate of the second control point
    /// @param control_2_y  vertical coordinate of the second control point
    /// @param x            horizontal coordinate of the new end point
    /// @param y            vertical coordinate of the new end point
    ///
    void bezier_curve_to(
        float control_1_x,
        float control_1_y,
        float control_2_x,
        float control_2_y,
        float x,
        float y );

    /// @brief  Restrict the clip region by the current path.
    ///
    /// Intersects the current clip region with the interior of the current
    /// path (the region that would be filled), and replaces the current
    /// clip region with this intersection.  Subsequent calls to clip can
    /// only reduce this further.  When filling or stroking, only pixels
    /// within the current clip region will change.  The current path is
    /// left unchanged by updating the clip region; begin a new path to
    /// clear it.  Defaults to the entire canvas.
    ///
    /// Tip: to be able to reset the current clip region, save the canvas
    ///      state first before clipping then restore the state to reset it.
    ///
    void clip();

    /// @brief  Clear a rectangular area back to transparent black.
    ///
    /// The clip region may limit the area cleared.  The current path is not
    /// affected by this clearing.  The current transform at the time that
    /// this is called will affect the given point and rectangle.  The width
    /// and/or the height may be negative or zero.  If either is zero, or the
    /// current transform is not invertible, this does nothing.
    ///
    /// @param x       horizontal coordinate of a rectangle corner
    /// @param y       vertical coordinate of a rectangle corner
    /// @param width   width of the rectangle
    /// @param height  height of the rectangle
    ///
    void clear_rectangle(
        float x,
        float y,
        float width,
        float height );

    /// @brief  Close the current subpath.
    ///
    /// Adds a straight line from the end of the current subpath back to its
    /// first point and marks the subpath as closed so that this line will
    /// join with the beginning of the path at this point.  A new, empty
    /// subpath will be started beginning with the same first point.  If the
    /// current path is empty, this does nothing.
    ///
    void close_path();
        
    /// @brief  Draw the interior of the current path using the fill style.
    ///
    /// Interior pixels are determined by the non-zero winding rule, with
    /// all open subpaths implicitly closed by a straight line beforehand.
    /// If shadows have been enabled by setting the shadow color with any
    /// opacity and either offsetting or blurring the shadows, then the
    /// shadows of the filled areas will be drawn first, followed by the
    /// filled areas themselves.  Both will be blended into the canvas
    /// according to the global alpha, the global compositing operation,
    /// and the clip region.  If the fill style is a gradient or a pattern,
    /// it will be affected by the current transform.  The current path is
    /// left unchanged by filling; begin a new path to clear it.  If the
    /// current transform is not invertible, this does nothing.
    ///
    void fill();

    /// @brief  Fill a rectangular area.
    ///
    /// This behaves as though the current path were reset to a single
    /// rectangle and then filled as usual.  However, the current path is
    /// not actually changed.  The current transform at the time that this is
    /// called will affect the given point and rectangle.  The width and/or
    /// the height may be negative but not zero.  If either is zero, or the
    /// current transform is not invertible, this does nothing.
    ///
    /// @param x       horizontal coordinate of a rectangle corner
    /// @param y       vertical coordinate of a rectangle corner
    /// @param width   width of the rectangle
    /// @param height  height of the rectangle
    ///
    void fill_rectangle(
        float x,
        float y,
        float width,
        float height );

    /// @brief  Draw a line of text by filling its outline.
    ///
    /// This behaves as though the current path were reset to the outline
    /// of the given text in the current font and size, positioned relative
    /// to the given anchor point according to the current alignment and
    /// baseline, and then filled as usual.  However, the current path is
    /// not actually changed.  The current transform at the time that this
    /// is called will affect the given anchor point and the text outline.
    /// However, the comparison to the maximum width in pixels and the
    /// condensing if needed is done before applying the current transform.
    /// The maximum width (if given) must be positive.  If it is not, or
    /// the text pointer is null, or the font has not been set yet, or the
    /// last setting of it was unsuccessful, or the current transform is not
    /// invertible, this does nothing.
    ///
    /// @param text           null-terminated UTF-8 string of text to fill
    /// @param x              horizontal coordinate of the anchor point
    /// @param y              vertical coordinate of the anchor point
    /// @param maximum_width  horizontal width to condense wider text to
    ///
    void fill_text(
        char const *text,
        float x,
        float y,
        float maximum_width = 1.0e30f );

    /// @brief  Tests whether a point is in or on the current path.
    ///
    /// Interior areas are determined by the non-zero winding rule, with
    /// all open subpaths treated as implicitly closed by a straight line
    /// beforehand.  Points exactly on the boundary are also considered
    /// inside.  The point to test is interpreted without being affected by
    /// the current transform, nor is the clip region considered.  The current
    /// path is left unchanged by this test.
    ///
    /// @param x  horizontal coordinate of the point to test
    /// @param y  vertical coordinate of the point to test
    /// @return   true if the point is in or on the current path
    ///
    bool is_point_in_path(
        float x,
        float y );

    /// @brief  Extend the current subpath with a straight line.
    ///
    /// The line will go from the current end point (if the current path is
    /// not empty) to the given point, which will become the new end point.
    /// Its position is affected by the current transform at the time that
    /// this is called.  If the current path was empty, this is equivalent
    /// to just a move.
    ///
    /// @param x  horizontal coordinate of the new end point
    /// @param y  vertical coordinate of the new end point
    ///
    void line_to(
        float x,
        float y );

    /// @brief  Create a new subpath.
    ///
    /// The given point will become the first point of the new subpath and
    /// is subject to the current transform at the time this is called.
    ///
    /// @param x  horizontal coordinate of the new first point
    /// @param y  vertical coordinate of the new first point
    ///
    void move_to(
        float x,
        float y );

    /// @brief  Extend the current subpath with a quadratic Bezier curve.
    ///
    /// The curve will go from the current end point (or the control point
    /// if the current path is empty) to the given point, which will become
    /// the new end point.  The curve will be tangent toward the control
    /// point at both ends.  The current transform at the time that this
    /// is called will affect both points passed in.
    ///
    /// Tip: to make multiple curves join smoothly, ensure that each new end
    ///      point is on a line between the adjacent control points.
    ///
    /// @param control_x  horizontal coordinate of the control point
    /// @param control_y  vertical coordinate of the control point
    /// @param x          horizontal coordinate of the new end point
    /// @param y          vertical coordinate of the new end point
    ///
    void quadratic_curve_to(
        float control_x,
        float control_y,
        float x,
        float y );

    /// @brief  Add a closed subpath in the shape of a rectangle.
    ///
    /// The rectangle has one corner at the given point and then goes in the
    /// direction along the width before going in the direction of the height
    /// towards the opposite corner.  The current transform at the time that
    /// this is called will affect the given point and rectangle.  The width
    /// and/or the height may be negative or zero, and this can affect the
    /// winding direction.
    ///
    /// @param x       horizontal coordinate of a rectangle corner
    /// @param y       vertical coordinate of a rectangle corner
    /// @param width   width of the rectangle
    /// @param height  height of the rectangle
    ///
    void rectangle(
        float x,
        float y,
        float width,
        float height );
    
    /// @brief  Set the font to use for text drawing.
    ///
    /// The font must be a TrueType font (TTF) file which has been loaded or
    /// mapped into memory.  Following some basic validation, the relevant
    /// sections of the font file contents are copied, and it is safe to
    /// change or destroy after this call.  As an optimization, calling this
    /// with either a null pointer or zero for the number of bytes will allow
    /// for changing the size of the previous font without recopying from
    /// the file.  Note that the font parsing is not meant to be secure;
    /// only use this with trusted TTF files!
    ///
    /// @param font   pointer to the contents of a TrueType font (TTF) file
    /// @param bytes  number of bytes in the font file
    /// @param size   size in pixels per em to draw at
    /// @return       true if the font was set successfully
    ///
    bool set_font(
        unsigned char const *font,
        int bytes,
        float size );

    /// @brief  Set filling or stroking to draw with an image pattern.
    ///
    /// Initially, pixels in the pattern correspond exactly to pixels on the
    /// canvas, with the pattern starting in the upper left.  The pattern
    /// is affected by the current transform at the time of drawing, and
    /// the pattern will be resampled as needed (with the filtering always
    /// wrapping regardless of the repetition setting).  The pattern can be
    /// repeated either horizontally, vertically, both, or neither, relative
    /// to the source image.  If the pattern is not repeated, then beyond it
    /// will be considered transparent black.  The pattern image, which should
    /// be in top to bottom rows of contiguous pixels from left to right,
    /// is copied and it is safe to change or destroy it after this call.
    /// The width and height must both be positive.  If either are not, or
    /// the image pointer is null, this does nothing.
    ///
    /// Tip: to use a small piece of a larger image, reduce the width and
    ///      height, and offset the image pointer while keeping the stride.
    ///
    /// @param type        whether to set the fill_style or stroke_style
    /// @param image       pointer to unpremultiplied sRGB RGBA8 image data
    /// @param width       width of the pattern image in pixels
    /// @param height      height of the pattern image in pixels
    /// @param stride      number of bytes between the start of each image row
    /// @param repetition  repeat, repeat_x, repeat_y, or no_repeat
    ///
    void set_pattern(
        brush_type type,
        unsigned char const *image,
        int width,
        int height,
        int stride,
        repetition_style repetition );

    /// @brief  Draw the edges of the current path using the stroke style.
    ///
    /// Edges of the path will be expanded into strokes according to the
    /// current dash pattern, dash offset, line width, line join style
    /// (and possibly miter limit), line cap, and transform.  If shadows
    /// have been enabled by setting the shadow color with any opacity and
    /// either offsetting or blurring the shadows, then the shadow will be
    /// drawn for the stroked lines first, then the stroked lines themselves.
    /// Both will be blended into the canvas according to the global alpha,
    /// the global compositing operation, and the clip region.  If the stroke
    /// style is a gradient or a pattern, it will be affected by the current
    /// transform.  The current path is left unchanged by stroking; begin a
    /// new path to clear it.  If the current transform is not invertible,
    /// this does nothing.
    ///
    /// Tip: to draw with a calligraphy-like angled brush effect, add a
    ///      non-uniform scale transform just before stroking.
    ///
    void stroke();

    /// @brief  Stroke a rectangular area.
    ///
    /// This behaves as though the current path were reset to a single
    /// rectangle and then stroked as usual.  However, the current path is
    /// not actually changed.  The current transform at the time that this
    /// is called will affect the given point and rectangle.  The width
    /// and/or the height may be negative or zero.  If both are zero, or
    /// the current transform is not invertible, this does nothing.  If only
    /// one is zero, this behaves as though it strokes a single horizontal or
    /// vertical line.
    ///
    /// @param x       horizontal coordinate of a rectangle corner
    /// @param y       vertical coordinate of a rectangle corner
    /// @param width   width of the rectangle
    /// @param height  height of the rectangle
    ///
    void stroke_rectangle(
        float x,
        float y,
        float width,
        float height );

    /// @brief  Draw a line of text by stroking its outline.
    ///
    /// This behaves as though the current path were reset to the outline
    /// of the given text in the current font and size, positioned relative
    /// to the given anchor point according to the current alignment and
    /// baseline, and then stroked as usual.  However, the current path is
    /// not actually changed.  The current transform at the time that this
    /// is called will affect the given anchor point and the text outline.
    /// However, the comparison to the maximum width in pixels and the
    /// condensing if needed is done before applying the current transform.
    /// The maximum width (if given) must be positive.  If it is not, or
    /// the text pointer is null, or the font has not been set yet, or the
    /// last setting of it was unsuccessful, or the current transform is not
    /// invertible, this does nothing.
    ///
    /// @param text           null-terminated UTF-8 string of text to stroke
    /// @param x              horizontal coordinate of the anchor point
    /// @param y              vertical coordinate of the anchor point
    /// @param maximum_width  horizontal width to condense wider text to
    ///
    void stroke_text(
        char const *text,
        float x,
        float y,
        float maximum_width = 1.0e30f );

    /// @brief  Measure the width in pixels of a line of text.
    ///
    /// The measured width is the advance width, which includes the side
    /// bearings of the first and last glyphs.  However, text as drawn may
    /// go outside this (e.g., due to glyphs that spill beyond their nominal
    /// widths or stroked text with wide lines).  Measurements ignore the
    /// current transform.  If the text pointer is null, or the font has
    /// not been set yet, or the last setting of it was unsuccessful, this
    /// returns zero.
    ///
    /// @param text  null-terminated UTF-8 string of text to measure
    /// @return      width of the text in pixels
    ///
    float measure_text(
        char const *text );

*/
                        {
                            "draw_image", [](lua_State * L)
                            {
/*
/// @brief  Draw an image onto the canvas.
///
/// The position of the rectangle that the image is drawn to is affected
/// by the current transform at the time of drawing, and the image will
/// be resampled as needed (with the filtering always clamping to the
/// edges of the image).  The drawing is also affected by the shadow,
/// global alpha, global compositing operation settings, and by the
/// clip region.  The current path is not affected by drawing an image.
/// The image data, which should be in top to bottom rows of contiguous
/// pixels from left to right, is not retained and it is safe to change
/// or destroy it after this call.  The width and height must both be
/// positive and the width and/or the height to scale to may be negative
/// but not zero.  Otherwise, or if the image pointer is null, or the
/// current transform is not invertible, this does nothing.
///
/// Tip: to use a small piece of a larger image, reduce the width and
///      height, and offset the image pointer while keeping the stride.
///
/// @param image      pointer to unpremultiplied sRGB RGBA8 image data
/// @param width      width of the image in pixels
/// @param height     height of the image in pixels
/// @param stride     number of bytes between the start of each image row
/// @param x          horizontal coordinate to draw the corner at
/// @param y          vertical coordinate to draw the corner at
/// @param to_width   width to scale the image to
/// @param to_height  height to scale the image to
///
void draw_image(
    unsigned char const *image,
    int width,
    int height,
    int stride,
    float x,
    float y,
    float to_width,
    float to_height );
*/
                                return 0;
                            }
                        }, {
                            "__gc", LuaXS::TypedGC<canvas_ity::canvas>
                        }, {
                            "get_image_data", [](lua_State * L)
                            {
/*
/// @brief  Fetch a rectangle of pixels from the canvas to an image.
///
/// This call is akin to a direct pixel-for-pixel copy from the canvas
/// buffer without resampling.  The position and size of the canvas
/// rectangle is not affected by the current transform.  The image data
/// is copied into, from top to bottom in rows of contiguous pixels from
/// left to right, and it is safe to change or destroy it after this call.
/// The requested rectangle may safely extend outside the bounds of the
/// canvas; these pixels will be set to transparent black.  The width
/// and height must be positive.  If not, or if the image pointer is
/// null, this does nothing.
///
/// Tip: to copy into a section of a larger image, reduce the width and
///      height, and offset the image pointer while keeping the stride.
/// Tip: use this to get the rendered image from the canvas after drawing.
///
/// @param image   pointer to unpremultiplied sRGB RGBA8 image data
/// @param width   width of the image in pixels
/// @param height  height of the image in pixels
/// @param stride  number of bytes between the start of each image row
/// @param x       horizontal coordinate of upper-left pixel to fetch
/// @param y       vertical coordinate of upper-left pixel to fetch
///
void get_image_data(
    unsigned char *image,
    int width,
    int height,
    int stride,
    int x,
    int y );
*/
                                return 0;
                            }
                        }, {
                            "put_image_data", [](lua_State * L)
                            {
/*
/// @brief  Replace a rectangle of pixels on the canvas with an image.
///
/// This call is akin to a direct pixel-for-pixel copy into the canvas
/// buffer without resampling.  The position and size of the canvas
/// rectangle is not affected by the current transform.  Nor is the
/// result affected by the current settings for the global alpha, global
/// compositing operation, shadows, or the clip region.  The image data,
/// which should be in top to bottom rows of contiguous pixels from left
/// to right, is copied from and it is safe to change or destroy it after
/// this call.  The width and height must be positive.  If not, or if the
/// image pointer is null, this does nothing.
///
/// Tip: to copy from a section of a larger image, reduce the width and
///      height, and offset the image pointer while keeping the stride.
/// Tip: use this to prepopulate the canvas before drawing.
///
/// @param image   pointer to unpremultiplied sRGB RGBA8 image data
/// @param width   width of the image in pixels
/// @param height  height of the image in pixels
/// @param stride  number of bytes between the start of each image row
/// @param x       horizontal coordinate of upper-left pixel to replace
/// @param y       vertical coordinate of upper-left pixel to replace
///
void put_image_data(
    unsigned char const *image,
    int width,
    int height,
    int stride,
    int x,
    int y );

/// @brief  Rotate the current transform.
///
/// The rotation is applied clockwise in a direction around the origin.
///
/// Tip: to rotate around another point, first translate that point to
///      the origin, then do the rotation, and then translate back.
///
/// @param angle  clockwise angle in radians
///
void rotate(
    float angle );

/// @brief  Scale the current transform.
///
/// Scaling may be non-uniform if the x and y scaling factors are
/// different.  When a scale factor is less than one, things will be
/// shrunk in that direction.  When a scale factor is greater than
/// one, they will grow bigger.  Negative scaling factors will flip or
/// mirror it in that direction.  The scaling factors must be non-zero.
/// If either is zero, most drawing operations will do nothing.
///
/// @param x  horizontal scaling factor
/// @param y  vertical scaling factor
///
void scale(
    float x,
    float y );

/// @brief  Set filling or stroking to use a constant color and opacity.
///
/// The color and opacity values will be clamped to the 0.0 to 1.0 range,
/// inclusive.  Filling and stroking defaults a constant color with 0.0,
/// 0.0, 0.0, 1.0 (opaque black).
///
/// @param type   whether to set the fill_style or stroke_style
/// @param red    sRGB red component of the painting color
/// @param green  sRGB green component of the painting color
/// @param blue   sRGB blue component of the painting color
/// @param alpha  opacity to paint with (not premultiplied)
///
void set_color(
    brush_type type,
    float red,
    float green,
    float blue,
    float alpha );

/// @brief  Set the degree of opacity applied to all drawing operations.
///
/// If an operation already uses a transparent color, this can make it
/// yet more transparent.  It must be in the range from 0.0 for fully
/// transparent to 1.0 for fully opaque, inclusive.  If it is not, this
/// does nothing.  Defaults to 1.0 (opaque).
///
/// @param alpha  degree of opacity applied to all drawing operations
///
void set_global_alpha(
    float alpha );

/// @brief  Set filling or stroking to use a linear gradient.
///
/// Positions the start and end points of the gradient and clears all
/// color stops to reset the gradient to transparent black.  Color stops
/// can then be added again.  When drawing, pixels will be painted with
/// the color of the gradient at the nearest point on the line segment
/// between the start and end points.  This is affected by the current
/// transform at the time of drawing.
///
/// @param type     whether to set the fill_style or stroke_style
/// @param start_x  horizontal coordinate of the start of the gradient
/// @param start_y  vertical coordinate of the start of the gradient
/// @param end_x    horizontal coordinate of the end of the gradient
/// @param end_y    vertical coordinate of the end of the gradient
///
void set_linear_gradient(
    brush_type type,
    float start_x,
    float start_y,
    float end_x,
    float end_y );

/// @brief  Set or clear the line dash pattern.
///
/// Takes an array with entries alternately giving the lengths of dash
/// and gap segments.  All must be non-negative; if any are not, this
/// does nothing.  These will be used to draw with dashed lines when
/// stroking, with each subpath starting at the length along the dash
/// pattern indicated by the line dash offset.  Initially these lengths
/// are measured in pixels, though the current transform at the time of
/// drawing can affect this.  The count must be non-negative.  If it is
/// odd, the array will be appended to itself to make an even count.  If
/// it is zero, or the pointer is null, the dash pattern will be cleared
/// and strokes will be drawn as solid lines.  The array is copied and
/// it is safe to change or destroy it after this call.  Defaults to
/// solid lines.
///
/// @param segments  pointer to array for dash pattern
/// @param count     number of entries in the array
///
void set_line_dash(
    float const *segments,
    int count );

/// @brief  Set the width of the lines when stroking.
///
/// Initially this is measured in pixels, though the current transform
/// at the time of drawing can affect this.  Must be positive.  If it
/// is not, this does nothing.  Defaults to 1.0.
///
/// @param width  width of the lines when stroking
///
void set_line_width(
    float width );

/// @brief  Set the limit on maximum pointiness allowed for miter joins.
///
/// If the distance from the point where the lines intersect to the
/// point where the outside edges of the join intersect exceeds this
/// ratio relative to the line width, then a bevel join will be used
/// instead, and the miter will be lopped off.  Larger values allow
/// pointier miters.  Only affects stroking and only when the line join
/// style is miter.  Must be positive.  If it is not, this does nothing.
/// Defaults to 10.0.
///
/// @param limit  the limit on maximum pointiness allowed for miter joins
///
void set_miter_limit(
    float limit );

/// @brief  Set filling or stroking to use a radial gradient.
///
/// Positions the start and end circles of the gradient and clears all
/// color stops to reset the gradient to transparent black.  Color stops
/// can then be added again.  When drawing, pixels will be painted as
/// though the starting circle moved and changed size linearly to match
/// the ending circle, while sweeping through the colors of the gradient.
/// Pixels not touched by the moving circle will be left transparent
/// black.  The radial gradient is affected by the current transform
/// at the time of drawing.  The radii must be non-negative.
///
/// @param type          whether to set the fill_style or stroke_style
/// @param start_x       horizontal starting coordinate of the circle
/// @param start_y       vertical starting coordinate of the circle
/// @param start_radius  starting radius of the circle
/// @param end_x         horizontal ending coordinate of the circle
/// @param end_y         vertical ending coordinate of the circle
/// @param end_radius    ending radius of the circle
///
void set_radial_gradient(
    brush_type type,
    float start_x,
    float start_y,
    float start_radius,
    float end_x,
    float end_y,
    float end_radius );

/// @brief  Set the level of Gaussian blurring on the shadow.
///
/// Zero produces no blur, while larger values will blur the shadow
/// more strongly.  This is not affected by the current transform.
/// Must be non-negative.  If it is not, this does nothing.  Defaults to
/// 0.0 (no blur).
///
/// @param level  the level of Gaussian blurring on the shadow
///
void set_shadow_blur(
    float level );

/// @brief  Set the color and opacity of the shadow.
///
/// Most drawing operations can optionally draw a blurred drop shadow
/// before doing the main drawing.  The shadow is modulated by the opacity
/// of the drawing and will be blended into the existing pixels subject to
/// the compositing settings and clipping region.  Shadows will only be
/// drawn if the shadow color has any opacity and the shadow is either
/// offset or blurred.  The color and opacity values will be clamped to
/// the 0.0 to 1.0 range, inclusive.  Defaults to 0.0, 0.0, 0.0, 0.0
/// (transparent black).
///
/// @param red    sRGB red component of the shadow color
/// @param green  sRGB green component of the shadow color
/// @param blue   sRGB blue component of the shadow color
/// @param alpha  opacity of the shadow (not premultiplied)
///
void set_shadow_color(
    float red,
    float green,
    float blue,
    float alpha );

/// @brief  Replace the current transform.
///
/// This takes six values for the upper two rows of a homogenous 3x3
/// matrix (i.e., {{a, c, e}, {b, d, f}, {0.0, 0.0, 1.0}}) describing
/// an arbitrary affine transform and replaces the current transform
/// with it.  The values can represent any affine transform such as
/// scaling, rotation, translation, or skew, or any composition of
/// affine transforms.  The matrix must be invertible.  If it is not,
/// most drawing operations will do nothing.
///
/// Tip: to reset the current transform back to the default, use
///      1.0, 0.0, 0.0, 1.0, 0.0, 0.0.
///
/// @param a  horizontal scaling factor (m11)
/// @param b  vertical skewing (m12)
/// @param c  horizontal skewing (m21)
/// @param d  vertical scaling factor (m22)
/// @param e  horizontal translation (m31)
/// @param f  vertical translation (m32)
///
void set_transform(
    float a,
    float b,
    float c,
    float d,
    float e,
    float f );

/// @brief  Add an arbitrary transform to the current transform.
///
/// This takes six values for the upper two rows of a homogenous 3x3
/// matrix (i.e., {{a, c, e}, {b, d, f}, {0.0, 0.0, 1.0}}) describing an
/// arbitrary affine transform and appends it to the current transform.
/// The values can represent any affine transform such as scaling,
/// rotation, translation, or skew, or any composition of affine
/// transforms.  The matrix must be invertible.  If it is not, most
/// drawing operations will do nothing.
///
/// @param a  horizontal scaling factor (m11)
/// @param b  vertical skewing (m12)
/// @param c  horizontal skewing (m21)
/// @param d  vertical scaling factor (m22)
/// @param e  horizontal translation (m31)
/// @param f  vertical translation (m32)
///
void transform(
    float a,
    float b,
    float c,
    float d,
    float e,
    float f );

/// @brief  Translate the current transform.
///
/// By default, positive x values shift that many pixels to the right,
/// while negative y values shift left, positive y values shift up, and
/// negative y values shift down.
///
/// @param x  amount to shift horizontally
/// @param y  amount to shift vertically
///
void translate(
    float x,
    float y );
*/
                                return 0;
                            }
                        }, {
                            "restore", [](lua_State * L)
                            {
                                Get(L).restore();

                                return 0;
                            }
                        }, {
                            "save", [](lua_State * L)
                            {
                                Get(L).save();

                                return 0;
                            }
                        },
                        { nullptr, nullptr }
                    };

                    luaL_register(L, nullptr, funcs);
                });

                return 1;
            }
        },
        { nullptr, nullptr }
    };

    luaL_register(L, nullptr, funcs);

    //
    //
    //

    // TODO: properties...
/*
/// @brief  Compositing operation for blending new drawing and old pixels.
///
/// The source_copy, source_in, source_out, destination_atop, and
/// destination_in operations may clear parts of the canvas outside the
/// new drawing but within the clip region.  Defaults to source_over.
///
/// source_atop:       Show new over old where old is opaque.
/// source_copy:       Replace old with new.
/// source_in:         Replace old with new where old was opaque.
/// source_out:        Replace old with new where old was transparent.
/// source_over:       Show new over old.
/// destination_atop:  Show old over new where new is opaque.
/// destination_in:    Clear old where new is transparent.
/// destination_out:   Clear old where new is opaque.
/// destination_over:  Show new under old.
/// exclusive_or:      Show new and old but clear where both are opaque.
/// lighter:           Sum the new with the old.
///
composite_operation global_composite_operation;

/// @brief  Horizontal offset of the shadow in pixels.
///
/// Negative shifts left, positive shifts right.  This is not affected
/// by the current transform.  Defaults to 0.0 (no offset).
///
float shadow_offset_x;

/// @brief  Vertical offset of the shadow in pixels.
///
/// Negative shifts up, positive shifts down.  This is not affected by
/// the current transform.  Defaults to 0.0 (no offset).
///
float shadow_offset_y;

/// @brief  Cap style for the ends of open subpaths and dash segments.
///
/// The actual shape may be affected by the current transform at the time
/// of drawing.  Only affects stroking.  Defaults to butt.
///
/// butt:    Use a flat cap flush to the end of the line.
/// square:  Use a half-square cap that extends past the end of the line.
/// circle:  Use a semicircular cap.
///
cap_style line_cap;

/// @brief  Join style for connecting lines within the paths.
///
/// The actual shape may be affected by the current transform at the time
/// of drawing.  Only affects stroking.  Defaults to miter.
///
/// miter:  Continue the ends until they intersect, if within miter limit.
/// bevel:  Connect the ends with a flat triangle.
/// round:  Join the ends with a circular arc.
///
join_style line_join;

/// @brief  Offset where each subpath starts the dash pattern.
///
/// Changing this shifts the location of the dashes along the path and
/// animating it will produce a marching ants effect.  Only affects
/// stroking and only when a dash pattern is set.  May be negative or
/// exceed the length of the dash pattern, in which case it will wrap.
/// Defaults to 0.0.
///
float line_dash_offset;

/// @brief  Horizontal position of the text relative to the anchor point.
///
/// When drawing text, the positioning of the text relative to the anchor
/// point includes the side bearings of the first and last glyphs.
/// Defaults to leftward.
///
/// leftward:   Draw the text's left edge at the anchor point.
/// rightward:  Draw the text's right edge at the anchor point.
/// center:     Draw the text's horizontal center at the anchor point.
/// start:      This is a synonym for leftward.
/// ending:     This is a synonym for rightward.
///
align_style text_align;

/// @brief  Vertical position of the text relative to the anchor point.
///
/// Defaults to alphabetic.
///
/// alphabetic:   Draw with the alphabetic baseline at the anchor point.
/// top:          Draw the top of the em box at the anchor point.
/// middle:       Draw the exact middle of the em box at the anchor point.
/// bottom:       Draw the bottom of the em box at the anchor point.
/// hanging:      Draw 60% of an em over the baseline at the anchor point.
/// ideographic:  This is a synonym for bottom.
///
baseline_style text_baseline;
*/
    //
    //
    //

	return 1;
}
