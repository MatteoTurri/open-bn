/*
 * /opt0/bn/script/utility/menu/title.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The title label of the menu window: a new text fades the old one out and itself in.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "wait.inc"

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

main()
{
    new shown[64]; /* the text on the label */
    SetColor(Self(), 0.902, 0.902, 0.215, 0.0);
    for (; ; )
    {
        sleep;
        new msg;
        new Float:alpha;
        new Float:duration;
        /* "FADE" (alpha, duration) and the string "UTF8": the text to show, "" for none */
        if (RecvMessage(msg, alpha, duration))
        {
            if (msg == TAG("FADE"))
            {
                AnimColor(Self(), 0.902, 0.902, 0.215, 1.0 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        new text[64];
        if (RecvMessageStr(msg, text, 64))
        {
            if (msg == TAG("UTF8"))
            {
                if (strcmp(shown, text))
                {
                    if (strlen(shown))
                    {
                        AnimColor(Self(), 0.902, 0.902, 0.215, 0.0, float(300), 0.0, ANIM_FUNC_LINEAR);
                        wait_ms(300);
                    }
                    SetOptionS(Self(), TAG("UTF8"), text);
                    if (strlen(text))
                    {
                        AnimColor(Self(), 0.902, 0.902, 0.215, 1.0, float(400), 0.0, ANIM_FUNC_LINEAR);
                    }
                    copy_string(shown, text, 64);
                }
            }
        }
    }
}
