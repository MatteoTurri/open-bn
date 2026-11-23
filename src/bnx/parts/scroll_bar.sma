/*
 * /opt0/bn/script/parts/scroll_bar.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A vertical scroll bar: a back plane, a track and a thumb that follows top and length, at least 20 pixels
 * long; the first move of the thumb slows down at the end, the later ones are linear.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

public manager; /* unused, but part of the layout */
public focus; /* unused, but part of the layout */
public Float:meta_alpha = 1.0; /* alpha of the whole bar, set by the "FADE" message */
new g_back;
new g_track;
new g_thumb;
public Float:height = 300.0;   /* in pixels */
public Float:top;              /* start of the visible part, as a fraction of the whole */
public Float:length = 1.0;     /* size of the visible part, as a fraction of the whole */
public Float:animtime = 100.0; /* duration of a move of the thumb, in ms */

public main()
{
    g_back = CreatePlane(Self());
    SetPos(g_back, 22.0, 0.0, 0.0, 0);
    SetScale(g_back, 48.0, height, 1.0);
    SetColor(g_back, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_back, imgGetResource(RES_SCRBAR1), 0, -1);
    SetOptionI(g_back, TAG("ALPH"), BLEND_ADD);
    SetOptionF(g_back, TAG("ZOFS"), -2.0);
    g_track = CreatePlane3x3(Self());
    SetPos(g_track, 2.0, 0.0, 0.0, 0);
    SetScale(g_track, 5.0, height, 1.0);
    SetColor(g_track, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_track, imgGetResource(RES_SCRBAR2), 0, -1);
    SetOptionI(g_track, TAG("ALPH"), BLEND_ADD);
    SetOptionF(g_track, TAG("ZOFS"), -1.0);
    {
        new Float:track_margin_tex[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_track, TAG("MGNT"), track_margin_tex, 4);
        new Float:track_margin[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_track, TAG("MGN"), track_margin, 4);
    }
    g_thumb = CreatePlane3x3(Self());
    SetColor(g_thumb, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_thumb, imgGetResource(RES_SCRBAR3), 0, -1);
    SetOptionI(g_thumb, TAG("ALPH"), BLEND_ADD);
    SetOptionF(g_thumb, TAG("ZOFS"), 0.0);
    {
        new Float:thumb_margin_tex[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_thumb, TAG("MGNT"), thumb_margin_tex, 4);
        new Float:thumb_margin[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_thumb, TAG("MGN"), thumb_margin, 4);
    }
    fade(2.0 * animtime, 0.0);
    SetScale(g_thumb, 5.0, height * length, 1.0);
    SetPos(g_thumb, 0.0, height * (top + 0.5 * (-1.0 + length)), 0.0, 0);
    new Float:shown_top = shown_top; /* NOTE: bug: initialized with itself instead of top */
    new Float:shown_length = length;
    new Float:shown_height = height;
    new first_move = 1;
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
        if (first_move == 1)
        {
            if (shown_top != top || shown_length != length)
            {
                AnimMoveLine(g_thumb, 0.0, height * (top + 0.5 * (-1.0 + length)), 0.0, animtime, 0.0,
                        ANIM_FUNC_EXPONENT);
                shown_top = top;
                first_move = 0;
            }
            if (shown_length != length)
            {
                AnimScale(g_thumb, 5.0, height * length, 1.0, animtime, 0.0, ANIM_FUNC_EXPONENT);
                shown_length = length;
                first_move = 0;
            }
        }
        else
        {
            if (shown_top != top || shown_length != length)
            {
                AnimMoveLine(g_thumb, 0.0, height * (top + 0.5 * (-1.0 + length)), 0.0, animtime, 0.0,
                        ANIM_FUNC_LINEAR);
                shown_top = top;
            }
            if (shown_length != length)
            {
                AnimScale(g_thumb, 5.0, height * length, 1.0, animtime, 0.0, ANIM_FUNC_LINEAR);
                shown_length = length;
            }
        }
        if (shown_height != height)
        {
            AnimScale(g_back, 48.0, height, 1.0, animtime, 0.0, ANIM_FUNC_LINEAR);
            AnimScale(g_track, 5.0, height, 1.0, animtime, 0.0, ANIM_FUNC_LINEAR);
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
        if (length * height < 20.0)
        {
            length = 20.0 / height;
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
