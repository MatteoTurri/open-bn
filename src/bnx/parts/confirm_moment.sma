/*
 * /opt0/bn/script/parts/confirm_moment.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A message box with no button: it dims the screen and shows the message (argument "msg"). The script ends once
 * the message is shown, and the caller destroys the box when it is done with it.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

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

/* Same as send_focus of dom.inc. */
send_focus(obj, target, z_offset, Float:sharp, Float:extra_w, Float:extra_h, Float:move_time, smoothing, blinking)
{
    if (!obj)
    {
        return;
    }
    SendMessage(obj, TAG("FOC0"), target, TAG(""), z_offset, sharp);
    SendMessage(obj, TAG("FOC1"), extra_w, extra_h, move_time, smoothing);
    SendMessage(obj, TAG("FOC2"), blinking, 0, 0, 0);
}

public manager; /* unused, but part of the layout */
public focus; /* the focus object, told that nothing is focused */
public Float:width = 400.0; /* of the window, in pixels */
public Float:height = 260.0; /* of the window, in pixels */
public Float:red; /* color of the window */
public Float:green = 0.2353; /* color of the window */
public Float:blue = 0.3922; /* color of the window */
new g_defocus;
new g_base;
new g_window;
new g_glow;
public life = 1000; /* unused, but part of the layout */
new g_message;
new g_message_shown;

#include "misc.inc"

main()
{
    new message[128];
    GetArg("msg", message, 128);
    new ok_label[8]; /* read, but the box has no button */
    if (!GetArg("ok", ok_label, 8))
    {
        copy_string(ok_label, "OK", 8);
    }
    g_defocus = CreateDefocus(Self());
    SetColor(g_defocus, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_defocus, TAG("LEVL"), 3);
    SetOptionI(g_defocus, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_defocus, TAG("ZOFS"), -3.0);
    g_base = CreatePlane(g_defocus);
    SetLocalScale(g_base, 2.0 * width, 2.0 * height, 1.0);
    SetColor(g_base, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_base, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_base, TAG("ZOFS"), -2.0);
    SetOptionI(g_base, TAG("ALPH"), BLEND_KEEP);
    g_window = CreatePlane3x3(g_base);
    SetScale(g_window, width, height, 1.0);
    SetColor(g_window, red, green, blue, 0.0);
    SetTexture(g_window, imgGetResource(RES_WINDOW1), 0, -1);
    SetOptionI(g_window, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_window, TAG("ZOFS"), -1.0);
    SetOptionI(g_window, TAG("ALPH"), BLEND_ADD);
    {
        new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_window, TAG("MGNT"), tex_margins, 4);
        new Float:margins[4];
        margins[0] = margins[1] = 0.5 * 128.0 / width;
        margins[2] = margins[3] = 0.5 * 128.0 / height;
        SetOptionP(g_window, TAG("MGN"), margins, 4);
    }
    g_glow = CreatePlane(g_window);
    SetPos(g_glow, 0.3, -0.2, 0.0, 0);
    SetScale(g_glow, 2.0, 2.0, 1.0);
    SetColor(g_glow, 0.0, 0.0, 0.0, 0.0);
    SetTexture(g_glow, imgGetResource(RES_DOT), 0, -1);
    SetOptionI(g_glow, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(g_glow, TAG("ALPH"), BLEND_ADD | BLEND_DEST_ALPHA);
    g_message = create_message_text(g_defocus, floatround(width - 64.0, 0), floatround(height - 48.0, 0));
    SetOptionS(g_message, TAG("UTF8"), message);
    SetColor(g_message, 0.7843, 0.7843, 0.7843, 0.0);
    sndSePlay(SE_DIALOG_OPEN);
    fade_window(1.0, 300.0);
    AnimColor(g_defocus, 0.35, 0.35, 0.35, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(300);
    hide_pad_help();
    if (focus)
    {
        send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    }
    for (; ; )
    {
        /* Centers the message once the textbox has laid it out, then fades it in. */
        if (!g_message_shown)
        {
            new rect[5];
            GetOptionP(g_message, TAG("RECT"), rect, 5);
            if (rect[0])
            {
                g_message_shown = 1;
                SetPos(g_message, 0.5 * -float(rect[3] >> 6), 0.5 * -float(rect[4] >> 6), 0.0, 0);
                AnimColor(g_message, 0.7843, 0.7843, 0.7843, 1.0, 50.0, 0.0, ANIM_FUNC_LINEAR);
                break;
            }
        }
        sleep;
    }
    wait_ms(50);
    return;
}

/* Fades the window to alpha over duration ms. */
fade_window(Float:alpha, Float:duration)
{
    AnimColor(g_window, red, green, blue, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
}

hide_pad_help()
{
    SetSystemParam(SYS_PADHELP, 0);
}
