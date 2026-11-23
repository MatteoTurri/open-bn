/*
 * /opt0/bn/script/parts/labelsize.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * Squeezes a text label horizontally, once its text is laid out, so that it is no wider than max_width.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

public Float:max_width = 200.0; /* in pixels */

main()
{
    new rect[5];
    for (; ; )
    {
        /* a valid flag, then the x, y, width and height of the text in 1/64 pixel */
        GetOptionP(Self(), TAG("RECT"), rect, 5);
        if (rect[0])
        {
            break;
        }
        sleep;
        continue;
    }
    if (rect[3] / 64.0 > max_width)
    {
        SetLocalScale(Self(), max_width / (rect[3] / 64.0), 1.0, 1.0);
        rect[3] = floatround(64.0 * max_width, 0); /* redundant as written */
    }
    Abort(Self());
}
