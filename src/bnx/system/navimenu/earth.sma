/*
 * /opt0/bn/script/system/navimenu/earth.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The globe of the navigation menu: a textured sphere turning between a back and a front plane.
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

public manager; /* unused, but part of the layout */
public focus; /* unused, but part of the layout */
public Float:meta_alpha; /* alpha of the planes, set by "FADE" */
new Float:g_earth_color[4] = {0.6, 0.8, 1.0, 0.4}; /* the color of the sphere, the alpha of the planes */
new g_back;
new g_globe;
new g_fore;

main()
{
    HideChildren(Self());
    new loaded = 1;
    new url[64];
    new obj;
    g_back = obj = CreatePlane(Self());
    SetPos(obj, -0.05, 0.0, 0.0, 0);
    SetScale(obj, 1.5, 1.5, 1.5);
    SetColor(obj, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(obj, TAG("ALPH"), BLEND_KEEP);
    get_tag_attr("IMG", "earth_back", "src", url, 64);
    if (!load_plane_texture(obj, url))
    {
        loaded = 0;
    }
    g_globe = obj = CreatePrim(Self());
    SetOptionI(obj, TAG("ALPH"), BLEND_ADD | BLEND_DEST_ALPHA);
    SetOptionI(obj, TAG("PIXW"), 7);
    SetColor(obj, g_earth_color[0], g_earth_color[1], g_earth_color[2], 1.0);
    SetOptionI(obj, TAG("TYPE"), 1);
    SetOptionI(obj, TAG("FACE"), 3);
    SetOptionI(obj, TAG("HDIV"), 24);
    SetOptionI(obj, TAG("VDIV"), 12);
    get_tag_attr("IMG", "earth_tex", "src", url, 64);
    if (!load_plane_texture(obj, url))
    {
        loaded = 0;
    }
    g_fore = obj = CreatePlane(Self());
    SetPos(obj, -0.05, 0.0, 0.0, 0);
    SetScale(obj, 1.5, 1.5, 1.5);
    SetColor(obj, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(obj, TAG("ALPH"), BLEND_ADD);
    get_tag_attr("IMG", "earth_fore", "src", url, 64);
    if (!load_plane_texture(obj, url))
    {
        loaded = 0;
    }
    if (loaded)
    {
        ShowChildren(Self());
    }
    meta_alpha = 1.0;
    fade_planes(1000.0, 0.0);
    new Float:angle;
    for (; ; )
    {
        new msg;
        new Float:alpha;
        new Float:duration;
        /* "FADE" (alpha, time): fades the planes to alpha times their own alpha */
        while (RecvMessage(msg, alpha, duration))
        {
            if (msg == TAG("FADE"))
            {
                meta_alpha = alpha;
                fade_planes(duration, 0.0);
            }
        }
        angle = angle + 0.002 * 60 / FrameRate();
        SetRot(g_globe, 0.0, angle, 0.0, 0, 0);
        sleep;
    }
}

fade_planes(Float:duration, Float:delay)
{
    AnimColor(g_back, 1.0, 1.0, 1.0, g_earth_color[3] * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_fore, 1.0, 1.0, 1.0, g_earth_color[3] * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
}

load_plane_texture(obj, url[])
{
    start_image_decode2(url);
    if (IMG_DONE != finish_image_decode())
    {
        printf("cannot load texture ( %s )\n", url);
        return 0;
    }
    SetTexture(obj, imgTexID(), 0, -1);
    return 1;
}
