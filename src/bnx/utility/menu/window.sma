/*
 * /opt0/bn/script/utility/menu/window.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The picture behind the menu window: shows the image it is sent once the requests have stopped for 200 ms, and
 * fades it in.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "tag.inc"

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

public main()
{
    new url[64];
    new decoding;
    new pending; /* an image was asked for at request_time and is not being decoded yet */
    new fading_in; /* the image is fading in; it then dims to 0.9 */
    new request_time;
    for (; ; )
    {
        new msg;
        new Float:alpha;
        new Float:duration;
        /* "FADE" (alpha, duration) and the string "IMAG": the url of the image to show */
        if (RecvMessage(msg, alpha, duration))
        {
            if (msg == TAG("FADE"))
            {
                AnimColor(Self(), 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
                pending = 0;
                decoding = 0;
                fading_in = 0;
            }
        }
        new text[64];
        if (RecvMessageStr(msg, text, 64))
        {
            if (msg == TAG("IMAG"))
            {
                if (strlen(text))
                {
                    copy_string(url, text, 64);
                    request_time = tickcount();
                    pending = 1;
                    decoding = 0;
                    fading_in = 0;
                    AnimColor(Self(), 1.0, 1.0, 1.0, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                }
            }
        }
        if (pending)
        {
            new elapsed = tickcount() - request_time;
            if (elapsed >= 200)
            {
                start_image_decode2(url);
                printf("load start : %s\n", url);
                pending = 0;
                decoding = 1;
            }
        }
        if (decoding)
        {
            new status = poll_image_decode();
            if (status == 1)
            {
                SetTexture(Self(), imgTexID(), 0, -1);
                AnimColor(Self(), 1.0, 1.0, 1.0, 1.0, float(750), 0.0, ANIM_FUNC_ANGULER);
                fading_in = 1;
                decoding = 0;
            }
            else if (status == -1)
            {
                decoding = 0;
            }
        }
        if (fading_in)
        {
            if (ANIM_DONE == AnimGetState(Self(), ANIM_COLOR))
            {
                AnimColor(Self(), 1.0, 1.0, 1.0, 0.9, float(3000), float(0), ANIM_FUNC_ANGULER);
                fading_in = 0;
            }
        }
        sleep;
    }
}
