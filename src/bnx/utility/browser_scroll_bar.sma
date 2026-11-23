/*
 * /opt0/bn/script/utility/browser_scroll_bar.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The vertical scroll bar of the file browser: a back plane, a track and a thumb that follows top and length,
 * drawn with the browser's own textures.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

#include "imgsync.inc"

#include "imgdec.inc"

public manager; /* unused, but part of the layout */
public focus; /* unused, but part of the layout */
public Float:meta_alpha = 1.0; /* alpha of the whole bar, set by the "FADE" message */
new g_back;
new g_track;
new g_thumb;
public Float:height = 300.0; /* in pixels */
public Float:top;            /* start of the visible part, as a fraction of the whole */
public Float:length = 1.0;   /* size of the visible part, as a fraction of the whole */
new Float:g_thumb_width = 2.0; /* the thumb narrows to it from its first width, 10 */

public main()
{
    new back_tex = load_kept_texture("file:/opt0/bn/script/utility/texture/scrlbar3.tm2");
    new track_tex = load_kept_texture("file:/opt0/bn/script/utility/texture/scrlbar1.tm2");
    new thumb_tex = load_kept_texture("file:/opt0/bn/script/utility/texture/scrlbar2.tm2");
    g_back = CreatePlane(Self());
    SetPos(g_back, 22.0, 0.0, 0.0, 0);
    SetScale(g_back, 48.0, height, 1.0);
    SetColor(g_back, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_back, TAG("ALPH"), BLEND_DARKEN);
    SetTexture(g_back, back_tex, 0, -1);
    SetOptionF(g_back, TAG("ZOFS"), -2.0);
    g_track = CreatePlane3x3(Self());
    SetPos(g_track, 2.0, 0.0, 0.0, 0);
    SetScale(g_track, 5.0, height, 1.0);
    SetColor(g_track, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_track, track_tex, 0, -1);
    SetOptionF(g_track, TAG("ZOFS"), -1.0);
    {
        new Float:track_margin_tex[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_track, TAG("MGNT"), track_margin_tex, 4);
        new Float:track_margin[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_track, TAG("MGN"), track_margin, 4);
    }
    g_thumb = CreatePlane3x3(Self());
    SetColor(g_thumb, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_thumb, thumb_tex, 0, -1);
    SetOptionF(g_thumb, TAG("ZOFS"), 0.0);
    {
        new Float:thumb_margin_tex[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_thumb, TAG("MGNT"), thumb_margin_tex, 4);
        new Float:thumb_margin[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_thumb, TAG("MGN"), thumb_margin, 4);
    }
    fade(100.0, 0.0);
    SetScale(g_thumb, 10.0, height * length, 1.0);
    SetPos(g_thumb, 0.0, -0.5 * height + height * (top + length / 2.0), 0.0, 0);
    new Float:shown_height = height;
    for (; ; )
    {
        new msg;
        new Float:alpha;
        new Float:duration;
        while (RecvMessage(msg, alpha, duration))
        {
            if (msg == TAG("FADE"))
            {
                meta_alpha = alpha;
                fade(duration, 0.0);
            }
        }
        AnimScale(g_thumb, g_thumb_width, height * length, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
        AnimMoveLine(g_thumb, 0.0, -0.5 * height + height * (top + length / 2.0), 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
        if (_:shown_height != height)
        {
            AnimScale(g_back, 48.0, height, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
            AnimScale(g_track, 5.0, height, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
            shown_height = height;
        }
        {
            new Float:thumb_scale[3];
            GetScale(g_thumb, thumb_scale[0], thumb_scale[1], thumb_scale[2]);
            new Float:margin[4];
            margin[2] = margin[3] = 0.5 * 20.0 / thumb_scale[1];
            margin[0] = margin[1] = 0.5;
            SetOptionP(g_thumb, TAG("MGN"), margin, 4);
        }
        sleep;
        if (length > 1.0)
        {
            length = 1.0;
        }
    }
    return;
}

/* Fades to meta_alpha in duration ms, after delay ms. */
fade(Float:duration, Float:delay)
{
    AnimColor(g_back, 1.0, 1.0, 1.0, meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_track, 1.0, 1.0, 1.0, meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
    AnimColor(g_thumb, 1.0, 1.0, 1.0, meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
}

/* Decodes the image url and keeps its texture; returns it, 0 on failure. */
load_kept_texture(url[])
{
    start_image_decode2(url);
    if (IMG_DONE != finish_image_decode())
    {
        return 0;
    }
    new texture = imgTexID();
    imgKeepTex(texture);
    return texture;
}
