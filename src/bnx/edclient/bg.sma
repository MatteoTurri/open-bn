/*
 * /opt0/bn/script/edclient/bg.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The background of the edclient pages: the picture IMG_BG over the whole screen, which "FADE" fades.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

#include "imgsync.inc"

/* Same as get_tag_attr2 of text.inc. */
get_tag_attr(tag[], id[], attr[], value[], maxlen)
{
    if (0 == tag[0] || 0 == id[0])
    {
        return 0;
    }
    new node = domFindTag(domRoot(), tag, id, 0);
    if (node)
    {
        new len = domGetData(node, attr, value, maxlen);
        if (0 == strcmp(attr, "src") || 0 == strcmp(attr, "href"))
        {
            xmlMakeFullPath(value, maxlen);
        }
        return len;
    }
    return 0;
}

#include "imgdec.inc"

#include "misc.inc"

/*
 * Creates a nine-slice plane at rect (x, y, z, width, height; with no size, the texture's), whose edges keep the
 * size in pixels given by border (left, right, top, bottom).
 */
create_frame_plane(parent, rect[], zsort, color, texture, Float:tex_w, Float:tex_h, Float:border[])
{
    new plane;
    new Float:x;
    new Float:y;
    new Float:z;
    new Float:plane_w;
    new Float:plane_h;
    new Float:margins[4];
    new Float:red;
    new Float:green;
    new Float:blue;
    new Float:alpha;
    unpack_color(color, red, green, blue, alpha);
    plane = CreatePlane3x3(parent);
    x = rect[0];
    y = rect[1];
    z = rect[2];
    plane_w = rect[3];
    plane_h = rect[4];
    if (0.0 == plane_w && 0.0 == plane_h)
    {
        plane_w = tex_w;
        plane_h = tex_h;
    }
    SetPos(plane, x, y, z, 0);
    SetOptionI(plane, TAG("ZSRT"), zsort);
    SetColor(plane, red, green, blue, 0.0);
    if (0 != texture)
    {
        SetTexture(plane, texture, 0, -1);
    }
    SetLocalScale(plane, plane_w, plane_h, 0.0);
    margins[0] = border[0] / plane_w;
    margins[1] = border[1] / plane_w;
    margins[2] = border[2] / plane_h;
    margins[3] = border[3] / plane_h;
    SetOptionP(plane, TAG("MGN"), margins, 4);
    margins[0] = border[0] / tex_w;
    margins[1] = border[1] / tex_w;
    margins[2] = border[2] / tex_h;
    margins[3] = border[3] / tex_h;
    SetOptionP(plane, TAG("MGNT"), margins, 4);
    return plane;
}

new g_bg;

main()
{
    {
        new url[64];
        new texture;
        new Float:tex_w;
        new Float:tex_h;
        new zsort;
        new color;
        new rect[5];
        new Float:border[4];
        get_tag_attr("IMG", "IMG_BG", "src", url, 64);
        start_image_decode2(url);
        finish_image_decode();
        texture = imgTexID();
        tex_w = float(imgTexWidth());
        tex_h = float(imgTexHeight());
        imgKeepTex(texture);
        rect[0] = 320.0;
        rect[1] = 240.0;
        rect[2] = 0;
        rect[3] = 640.0;
        rect[4] = 480.0;
        zsort = ZSORT_NONE;
        color = pack_color(255, 255, 255, 255);
        border[0] = 0.0;
        border[1] = 0.0;
        border[2] = 0.0;
        border[3] = 0.0;
        g_bg = create_frame_plane(Disp2D(0), rect, zsort, color, texture, tex_w, tex_h, border);
        SetOptionI(g_bg, TAG("ALPH"), BLEND_ALPHA);
    }
    for (; ; )
    {
        new msg;
        new Float:arg_a;
        new Float:arg_b;
        new arg_c;
        while (RecvMessage(msg, arg_a, arg_b, arg_c))
        {
            /* "FADE" alpha, duration (ms) */
            if (msg == TAG("FADE"))
            {
                new Float:alpha = arg_a;
                new Float:duration = arg_b;
                AnimColor(g_bg, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        sleep;
    }
}
