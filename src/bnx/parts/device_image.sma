/*
 * /opt0/bn/script/parts/device_image.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The picture of a device: the image of the url sent with "IMAG", at the 2D position sent with POS; a new image
 * fades in brightly, then settles at a dimmer alpha.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include "imgsync.inc"

/* Returns whether str is a packed string. */
is_packed(str[])
{
    return str[0] <= 0 || str[0] > 255;
}

/* Copies the packed or unpacked string src into dest, at most maxlen cells. */
copy_string(dest[], src[], maxlen)
{
    new len = strlen(src) + 1;
    if (is_packed(src))
    {
        len = (len + 3) / 4;
    }
    len = min(len, maxlen);
    for (new i; i < len; i++)
    {
        dest[i] = src[i];
    }
}

#include "imgdec.inc"

main()
{
    new image[256];
    new changed;
    new fading_in;
    new Float:x;
    new Float:y;
    new Float:alpha = 1.0;
    for (; ; )
    {
        new msg;
        new Float:a;
        new Float:b;
        new str[256];
        while (RecvMessageStr(msg, str, 256))
        {
            if (msg == TAG("IMAG"))
            {
                if (strcmp(image, str))
                {
                    copy_string(image, str, 256);
                    changed = 1;
                }
            }
        }
        /* "POS" (x, y), "FADE" (alpha, duration) */
        while (RecvMessage(msg, a, b))
        {
            if (msg == TAG("POS"))
            {
                x = a;
                y = b;
            }
            if (msg == TAG("FADE"))
            {
                AnimColor(Self(), 1.0, 1.0, 1.0, 0.4 * a, b, 0.0, ANIM_FUNC_ANGULER);
                alpha = a;
            }
        }
        if (changed)
        {
            changed = 0;
            if (strlen(image))
            {
                AnimColor(Self(), 1.0, 1.0, 1.0, 0.0, float(100), 0.0, ANIM_FUNC_LINEAR);
                start_image_decode2(image);
                wait_ms(50);
                if (finish_image_decode() == IMG_DONE)
                {
                    SetTexture(Self(), imgTexID(), 0, -1);
                    SetPos(Self(), x, y, 0.0, Disp2D(0));
                    AnimColor(Self(), 1.0, 1.0, 1.0, 0.8 * alpha, float(350), 0.0, ANIM_FUNC_ANGULER);
                    fading_in = 1;
                }
            }
            else
            {
                AnimColor(Self(), 1.0, 1.0, 1.0, 0.0, float(100), 0.0, ANIM_FUNC_LINEAR);
            }
        }
        if (ANIM_DONE == AnimGetState(Self(), ANIM_COLOR) && fading_in == 1)
        {
            AnimColor(Self(), 1.0, 1.0, 1.0, 0.4 * alpha, float(1000), float(100), ANIM_FUNC_ANGULER);
            fading_in = 0;
        }
        sleep;
    }
}
