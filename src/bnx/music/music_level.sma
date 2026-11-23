/*
 * /opt0/bn/script/music/music_level.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A level meter (texture/meter.tm2) that shows the public level, 0 to 100, with a mask that moves with it.
 * "FADE" (alpha, time) fades it; "KILL" (time) fades it out and destroys it.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

#include "imgsync.inc"

#include "imgdec.inc"

public level; /* 0 to 100, written by the owner */
public level2; /* unused, but part of the layout */
new g_back;
new g_mask;
new g_meter;
new g_meter_tex;

public main()
{
    g_meter_tex = load_kept_texture("file:/opt0/bn/script/music/texture/meter.tm2");
    g_back = CreatePlane(Self());
    SetColor(g_back, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_back, TAG("ALPH"), BLEND_KEEP);
    SetOptionF(g_back, TAG("ZOFS"), -20000.0);
    g_mask = CreatePlane(Self());
    SetColor(g_mask, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_mask, TAG("ALPH"), BLEND_KEEP);
    SetOptionF(g_mask, TAG("ZOFS"), -10000.0);
    SetTexture(g_mask, g_meter_tex, 0, -1);
    g_meter = CreatePlane(Self());
    SetPos(g_meter, 0.0, 0.0, 0.0, 0);
    SetColor(g_meter, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_meter, g_meter_tex, 0, -1);
    SetOptionI(g_meter, TAG("ALPH"), BLEND_ADD | BLEND_DEST_ALPHA);
    set_level(0.0, 0.0);
    new shown_level;
    new kill_frames;
    for (; ; )
    {
        new msg;
        new Float:a;
        new Float:b;
        new c;
        /* "FADE" (alpha, duration); "KILL" (duration): fades out, then destroys the object. */
        while (RecvMessage(msg, a, b, c))
        {
            if (msg == TAG("FADE"))
            {
                new Float:alpha = a;
                new Float:duration = b;
                AnimColor(g_mask, 1.0, 1.0, 1.0, 0.8 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
                if (0.0 == alpha)
                {
                    set_level(0.0, duration);
                }
            }
            if (msg == TAG("KILL"))
            {
                new Float:duration = a;
                AnimColor(g_mask, 1.0, 1.0, 1.0, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
                set_level(0.0, duration);
                kill_frames = floatround(duration / 16.7, 0);
                if (kill_frames <= 0)
                {
                    kill_frames = 1;
                }
            }
        }
        level = clamp(level, 0, 100);
        if (level != shown_level)
        {
            set_level(0.01 * level, 200.0);
            shown_level = level;
        }
        if (kill_frames > 0)
        {
            if (--kill_frames <= 0)
            {
                Destroy(Self());
                break;
            }
        }
        sleep;
    }
    return;
}

/* Moves the mask to fraction of the height, 0.0 to 1.0. */
set_level(Float:fraction, Float:duration)
{
    AnimMoveLine(g_mask, 0.0, 1.0 - fraction, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
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
