/*
 * /opt0/bn/script/parts/window.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A window frame of width by height pixels, with a glow that mirrors the place and the brightness of the focus
 * object across the window.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

public manager; /* unused, but part of the layout */
public focus; /* the focus object */
new g_backdrop; /* a plane twice the size of the window that draws nothing */
new g_frame;
new g_glow;
public Float:width = 300.0;  /* in pixels */
public Float:height = 300.0; /* in pixels */
public Float:meta_alpha = 1.0; /* alpha of the frame, set by the "FADE" message */

public main()
{
    g_backdrop = CreatePlane(Self());
    SetLocalScale(g_backdrop, 2.0 * width, 2.0 * height, 1.0);
    SetColor(g_backdrop, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_backdrop, TAG("ALPH"), BLEND_KEEP);
    SetOptionI(g_backdrop, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetOptionF(g_backdrop, TAG("ZOFS"), -2.0);
    g_frame = CreatePlane3x3(g_backdrop);
    SetScale(g_frame, width, height, 1.0);
    SetColor(g_frame, 0.0, 0.2353, 0.3922, 0.0);
    SetTexture(g_frame, imgGetResource(RES_WINDOW1), 0, -1);
    SetOptionI(g_frame, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetOptionF(g_frame, TAG("ZOFS"), -1.0);
    SetOptionI(g_frame, TAG("ALPH"), BLEND_ADD);
    {
        new Float:margin_tex[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_frame, TAG("MGNT"), margin_tex, 4);
        new Float:margin[4];
        margin[0] = margin[1] = 0.5 * 128.0 / width;
        margin[2] = margin[3] = 0.5 * 128.0 / height;
        SetOptionP(g_frame, TAG("MGN"), margin, 4);
    }
    g_glow = CreatePlane(g_frame);
    SetPos(g_glow, 0.3, -0.2, 0.0, 0);
    SetScale(g_glow, 2.0, 2.0, 1.0);
    SetColor(g_glow, 0.0, 0.0, 0.0, 0.0);
    SetTexture(g_glow, imgGetResource(RES_DOT), 0, -1);
    SetOptionI(g_glow, TAG("ZSRT"), ZSORT_BACKGROUND);
    SetOptionI(g_glow, TAG("ALPH"), BLEND_ADD | BLEND_DEST_ALPHA);
    AnimColor(g_frame, 0.0, 0.2353, 0.3922, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(100);
    new Float:shown_width = width;
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
        if (focus)
        {
            new Float:focus_pos[3];
            GetPos(focus, focus_pos[0], focus_pos[1], focus_pos[2], g_frame);
            new Float:glow_pos[2];
            glow_pos[0] = -focus_pos[0];
            glow_pos[1] = focus_pos[1];
            SetPos(g_glow, glow_pos[0], glow_pos[1], 0.0, 0);
            new Float:focus_alpha;
            GetColor(focus, _, _, _, focus_alpha);
            AnimColor(g_glow, 0.5 * focus_alpha, 0.5 * focus_alpha, 0.5 * focus_alpha, 1.0, 100.0, 0.0,
                    ANIM_FUNC_LINEAR);
        }
        if (shown_width != width || shown_height != height)
        {
            SetScale(g_frame, width, height, 1.0);
        }
        shown_width = width;
        shown_height = height;
        sleep;
    }
}

/* Fades the frame to meta_alpha. */
fade(Float:duration, Float:delay)
{
    AnimColor(g_frame, 0.0, 0.2353, 0.3922, meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
}
