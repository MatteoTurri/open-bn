/*
 * /opt0/bn/script/top_item.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The icon of an item of the top page list: a plane that shows the image named by "DCDE", fitted in 256x256, and
 * fades in when it is decoded.
 */

#include <core>
#include <bn>

#pragma dynamic 512

#include "tag.inc"

#include <float>

#include "text.inc"

#include "imgdec.inc"

#include "string.inc"

/* How the image of the item is being decoded. */
enum
{
    DECODER_NONE,
    DECODER_TM2,
    DECODER_IMAGE
}

new g_tm2_ext[] = "tm2";

/* Starts decoding url, a .tm2 file at its own size; returns the DECODER_* that decodes it. */
start_decode(url[], req_width, req_height, image_mode)
{
    if (0 == strlen(url))
    {
        return DECODER_NONE;
    }
    new path[256];
    sprintf(path, url);
    new decoder;
    if (has_extension(path, g_tm2_ext))
    {
        start_image_decode(path);
        decoder = DECODER_TM2;
    }
    else
    {
        imgDecStart(path, req_width, req_height, image_mode, 100, 0);
        decoder = DECODER_IMAGE;
    }
    return decoder;
}

public item_main()
{
    new shown;
    new decoder = DECODER_IMAGE;
    new plane = CreatePlane(Self());
    SetColor(plane, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(plane, TAG("ALPH"), BLEND_ADD);
    sleep;
    for (; ; )
    {
        new msg;
        new Float:alpha;
        new Float:duration;
        new Float:delay;
        new curve;
        new str[128];
        /* "FADE" alpha, duration, delay, curve (0: linear) fades the icon once it is shown. */
        while (RecvMessage(msg, alpha, duration, delay, curve))
        {
            if (msg == TAG("FADE"))
            {
                if (shown)
                {
                    if (0 == curve)
                    {
                        AnimColor(plane, 1.0, 1.0, 1.0, alpha, duration, delay, ANIM_FUNC_LINEAR);
                    }
                    else
                    {
                        AnimColor(plane, 1.0, 1.0, 1.0, alpha, duration, delay, curve);
                    }
                }
            }
            if (msg == TAG("LOAD"))
            {
            }
            if (msg == TAG("BUSY"))
            {
            }
            if (msg == TAG("SLEP"))
            {
            }
        }
        /* "DCDE" url decodes the icon; "FLWR" is ignored. */
        while (RecvMessageStr(msg, str, 128))
        {
            if (msg == TAG("DCDE"))
            {
                decoder = start_decode(str, 256, 256, 0);
            }
            if (msg == TAG("FLWR"))
            {
            }
        }
        if (!shown)
        {
            new status;
            if (decoder == DECODER_TM2)
            {
                status = poll_image_decode();
            }
            else if (decoder == DECODER_IMAGE)
            {
                status = imgDecSync();
            }
            else if (DECODER_NONE == decoder)
            {
                status = IMG_FAILED;
            }
            if (status == IMG_DONE)
            {
                SetTexture(plane, imgTexID(), 0, -1);
                new Float:aspect = 1.0 * imgTexOrgWidth() / imgTexOrgHeight();
                if (aspect > 1.0)
                {
                    SetLocalScale(plane, 256.0, 256.0 / aspect, 256.0);
                }
                else
                {
                    SetLocalScale(plane, 256.0 * aspect, 256.0, 256.0);
                }
                AnimColor(plane, 1.0, 1.0, 1.0, 1.0, 1000.0, 0.0, ANIM_FUNC_FASTER);
                shown = 1;
            }
        }
        sleep;
    }
}
