/*
 * /opt0/bn/script/music/music_meter.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The time bar of the music player: a bar of the public length filled to the percent of the track played.
 * "PROG" (track, percent) moves it, hiding the fill for a moment when the track changes; "FADE" (alpha, time).
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

new g_bar;
new g_fill;
public Float:length = 150.0; /* set by the player */
new Float:g_end_width = 10.0; /* the fill is never narrower */
new g_track = -1;
new g_percent = 4;
new Float:g_alpha;
new Float:g_fill_shown; /* 0.0 while the fill is hidden after a change of track */
new Float:g_ratio = 0.04; /* the filled part of the bar */

public main()
{
    g_bar = CreatePlane3x3(Self());
    SetScale(g_bar, length, 5.0, 1.0);
    SetPos(g_bar, 0.0, 0.0, 0.0, 0);
    SetTexture(g_bar, imgGetResource(RES_BAR1), 0, -1);
    SetOptionI(g_bar, TAG("ALPH"), BLEND_ADD);
    {
        new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
        new Float:margins[4] = {0.2, 0.2, 0.5, 0.5};
        SetOptionP(g_bar, TAG("MGNT"), tex_margins, 4);
        SetOptionP(g_bar, TAG("MGN"), margins, 4);
    }
    g_fill = CreatePlane3x3(Self());
    SetTexture(g_fill, imgGetResource(RES_BAR3), 0, -1);
    SetOptionI(g_fill, TAG("ALPH"), BLEND_ADD);
    {
        new Float:fill_tex_margins[4] = {0.0, 0.5, 0.5, 0.5};
        new Float:fill_margins[4] = {0.0, 0.5, 0.5, 0.5};
        SetOptionP(g_fill, TAG("MGNT"), fill_tex_margins, 4);
        SetOptionP(g_fill, TAG("MGN"), fill_margins, 4);
    }
    fade_bar(0.0);
    update_fill(0.0);
    update_fill_margins();
    new show_frames;
    new since_update;
    new change_rate;
    for (; ; )
    {
        new msg;
        new a;
        new b;
        new c;
        /* "FADE" (alpha, duration); "PROG" (track, percent). */
        while (RecvMessage(msg, a, b, c))
        {
            /* A new percent moves the fill after 40 frames, or at once and faster when it changes often. */
            if (msg == TAG("PROG"))
            {
                new track = a;
                new percent = b;
                if (track != g_track)
                {
                    if (track >= 0)
                    {
                        g_fill_shown = 0.0;
                        if (g_track >= 0)
                        {
                            fade_bar(1000.0);
                            show_frames = 60;
                        }
                        else
                        {
                            fade_bar(0.0);
                            show_frames = 6;
                        }
                    }
                    else
                    {
                    }
                    g_track = track;
                }
                else
                {
                    if (percent != g_percent)
                    {
                        if (since_update > 40 || change_rate > 50)
                        {
                            g_ratio = 0.01 * clamp(percent, 4, 100);
                            if (0 == show_frames && 0 != percent)
                            {
                                if (change_rate > 50)
                                {
                                    update_fill(200.0);
                                }
                                else
                                {
                                    update_fill(1000.0);
                                }
                            }
                            g_percent = percent;
                            since_update = 0;
                        }
                        change_rate = change_rate + 10;
                        if (change_rate > 60)
                        {
                            change_rate = 60;
                        }
                    }
                    since_update++;
                    if (change_rate > 0)
                    {
                        change_rate--;
                    }
                }
            }
            if (msg == TAG("FADE"))
            {
                g_alpha = a;
                fade_bar(b);
            }
        }
        if (show_frames > 0)
        {
            if (--show_frames <= 0)
            {
                g_fill_shown = 1.0;
                fade_bar(500.0);
                update_fill(0.0);
            }
        }
        update_fill_margins();
        sleep;
    }
    return;
}

/* Fades the bar to half g_alpha, the fill also by g_fill_shown. */
fade_bar(Float:duration)
{
    AnimColor(g_bar, 1.0, 1.0, 1.0, 0.5 * g_alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_fill, 1.0, 1.0, 1.0, 0.5 * (g_alpha * g_fill_shown), duration, 0.0, ANIM_FUNC_LINEAR);
}

/* Sizes the fill to g_ratio of the bar, from its left end; at once when duration is 0.0. */
update_fill(Float:duration)
{
    new Float:width = length * g_ratio;
    new Float:x = 0.5 * (-length + width) + g_end_width;
    if (0.0 == duration)
    {
        SetScale(g_fill, width, 5.0, 1.0);
        SetPos(g_fill, x, 0.0, 0.0, 0);
    }
    else
    {
        AnimScale(g_fill, width, 5.0, 1.0, duration, 0.0, ANIM_FUNC_LINEAR);
        AnimMoveLine(g_fill, x, 0.0, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
    }
}

/* Sets the margins of the 3x3 fill so that its ends keep their width. */
update_fill_margins()
{
    new Float:scale[3];
    GetScale(g_fill, scale[0], scale[1], scale[2]);
    new Float:margins[4] = {0.0, 0.5, 0.5, 0.5};
    if (scale[0] < g_end_width)
    {
        margins[1] = 1.0;
    }
    else if (scale[0] < 200.0)
    {
        margins[1] = g_end_width / scale[0];
    }
    else
    {
        margins[1] = g_end_width / scale[0];
        margins[0] = 1.0 - 200.0 / scale[0];
    }
    SetOptionP(g_fill, TAG("MGN"), margins, 4);
}
