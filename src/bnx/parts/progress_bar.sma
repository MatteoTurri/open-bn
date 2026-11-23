/*
 * /opt0/bn/script/parts/progress_bar.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A progress bar 280 pixels long that follows progress; 300 ms after it reaches 100 the script ends, fading the
 * bar out first if auto_fade_out is set.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

public manager; /* unused, but part of the layout */
public focus; /* unused, but part of the layout */
public auto_fade_out = 1; /* fade the bar out before the end */
public Float:meta_alpha = 1.0; /* alpha of the fill, set by the "FADE" message */
new g_track;
new g_highlight;
new g_fill;
public progress; /* from 0 to 100 */

public main()
{
    new Float:bar_w = 280.0;
    new Float:track_w = 8.0 + bar_w;
    g_track = CreatePlane3x3(Self());
    SetScale(g_track, track_w, 5.0, 1.0);
    SetPos(g_track, 0.0, 0.0, 0.0, 0);
    SetColor(g_track, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_track, imgGetResource(RES_BAR1), 0, -1);
    SetOptionI(g_track, TAG("ALPH"), BLEND_ADD);
    {
        new Float:track_margin_tex[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_track, TAG("MGNT"), track_margin_tex, 4);
        new Float:track_margin[4] = {0.1, 0.1, 0.5, 0.5};
        SetOptionP(g_track, TAG("MGN"), track_margin, 4);
    }
    g_highlight = CreatePlane3x3(Self());
    SetPos(g_highlight, -0.5 * bar_w - 10.0 * 0.5, 0.5, 0.0, 0);
    SetScale(g_highlight, -10.0, 1.0, 1.0);
    SetColor(g_highlight, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_highlight, imgGetResource(RES_BAR2), 0, -1);
    SetOptionI(g_highlight, TAG("ALPH"), BLEND_ADD);
    {
        new Float:highlight_margin_tex[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_highlight, TAG("MGNT"), highlight_margin_tex, 4);
        new Float:highlight_margin[4] = {0.5, 0.0, 0.5, 0.5};
        SetOptionP(g_highlight, TAG("MGN"), highlight_margin, 4);
    }
    g_fill = CreatePlane3x3(Self());
    SetPos(g_fill, -0.5 * bar_w, 0.0, 0.0, 0);
    SetScale(g_fill, 0.0, 7.0, 1.0);
    SetColor(g_fill, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_fill, imgGetResource(RES_BAR3), 0, -1);
    SetOptionI(g_fill, TAG("ALPH"), BLEND_ADD);
    {
        new Float:fill_margin_tex[4] = {0.0, 0.5, 0.5, 0.5};
        SetOptionP(g_fill, TAG("MGNT"), fill_margin_tex, 4);
        new Float:fill_margin[4] = {0.0, 0.5, 0.5, 0.5};
        SetOptionP(g_fill, TAG("MGN"), fill_margin, 4);
    }
    AnimColor(g_track, 1.0, 1.0, 1.0, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
    new full_frames;
    for (; ; )
    {
        new msg;
        new Float:alpha;
        while (RecvMessage(msg, alpha))
        {
            if (msg == TAG("FADE"))
            {
                meta_alpha = alpha;
            }
        }
        progress = clamp(progress, 0, 100);
        AnimScale(g_highlight, bar_w * progress / 100 - 10.0, 1.0, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
        AnimMoveLine(g_highlight, -0.5 * bar_w + 0.5 * bar_w * progress / 100 - 10.0 * 0.5, 0.5, 0.0, 100.0, 0.0,
                ANIM_FUNC_LINEAR);
        AnimScale(g_fill, bar_w * progress / 100, 7.0, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
        AnimMoveLine(g_fill, -0.5 * bar_w + 0.5 * bar_w * progress / 100, 0.0, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
        {
            new Float:fill_scale[3];
            GetScale(g_fill, fill_scale[0], fill_scale[1], fill_scale[2]);
            /* a short fill fades with its length and has no highlight */
            if (fill_scale[0] < 20.0)
            {
                AnimColor(g_highlight, 1.0, 1.0, 1.0, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                AnimColor(g_fill, 1.0, 1.0, 1.0, fill_scale[0] / 20.0 * meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
            }
            else if (fill_scale[0] < 150.0)
            {
                new Float:margin[4] = {0.0, 0.5, 0.5, 0.5};
                margin[1] = 7.0 / fill_scale[0];
                SetOptionP(g_fill, TAG("MGN"), margin, 4);
                AnimColor(g_fill, 1.0, 1.0, 1.0, meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
            }
            else
            {
                new Float:margin[4] = {0.0, 0.5, 0.5, 0.5};
                margin[1] = 7.0 / fill_scale[0];
                margin[0] = 1.0 - (150.0 - 20.0 * 0.5) / fill_scale[0] - margin[1];
                SetOptionP(g_fill, TAG("MGN"), margin, 4);
                AnimColor(g_fill, 1.0, 1.0, 1.0, meta_alpha, 100.0, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        if (progress >= 100)
        {
            full_frames++;
        }
        if (full_frames > ms_to_frames(300))
        {
            break;
        }
        sleep;
    }
    if (auto_fade_out)
    {
        AnimColor(g_track, 1.0, 1.0, 1.0, 0.0, 500.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_highlight, 1.0, 1.0, 1.0, 0.0, 500.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_fill, 1.0, 1.0, 1.0, 0.0, 500.0, 0.0, ANIM_FUNC_LINEAR);
        wait_ms(500);
    }
    return;
}
