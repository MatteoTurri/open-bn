/*
 * /opt0/bn/script/parts/progress_window.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A message box with up to three lines (arguments "msg" to "msg3"), a progress bar that follows progress, the
 * percentage (argument "percent", a format) and a cancel button (argument "cancel", its label) pressed with the
 * circle; the script ends at 100 or on the cancel.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

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
public focus; /* the focus object */
public progress;  /* from 0 to 100 */
public progress2; /* a second value for the percent format */
public Float:width = 400.0; /* of the window, in pixels */
public Float:height = 260.0; /* of the window, in pixels */
public Float:red; /* color of the window */
public Float:green = 0.2353; /* color of the window */
public Float:blue = 0.3922; /* color of the window */
new g_defocus;
new g_base;
new g_window;
new g_glow;
new g_bar; /* the progress_bar object */
public Float:border_left; /* the window is this much wider on the left */
new g_message_count;
new g_messages[3];
new g_button;
new g_button_label;
new g_percent_label;
new g_has_cancel = 1;
new g_show_percent = 1;

#include "string.inc"

#include "misc.inc"

main()
{
    new texts[3][64];
    if (GetArg("msg", texts[g_message_count], 64))
    {
        g_message_count++;
    }
    if (GetArg("msg2", texts[g_message_count], 64))
    {
        g_message_count++;
    }
    if (GetArg("msg3", texts[g_message_count], 64))
    {
        g_message_count++;
    }
    new cancel_label[8];
    GetArg("cancel", cancel_label, 8);
    new percent_format[8];
    GetArg("percent", percent_format, 8);
    if (!strcmp(cancel_label, ""))
    {
        g_has_cancel = 0;
    }
    if (!strcmp(percent_format, ""))
    {
        g_show_percent = 0;
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
    SetScale(g_window, width + border_left, height, 1.0);
    SetPos(g_window, 0.5 * -border_left, 0.0, 0.0, 0);
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
    g_bar = CreateNull(g_defocus);
    SetOptionI(g_bar, TAG("ZSRT"), ZSORT_OVERLAY);
    AttachScript(g_bar, _, "progress_bar");
    SetPubVar(g_bar, "manager", Self());
    SetPubVar(g_bar, "progress", 0);
    {
        new Float:y = -30.0;
        for (new i = g_message_count - 1; i >= 0; i--)
        {
            g_messages[i] = create_button_label(g_defocus, 0.0, y);
            SetColor(g_messages[i], 0.7843, 0.7843, 0.7843, 0.0);
            SetOptionS(g_messages[i], TAG("UTF8"), texts[i]);
            y = y - 20.0;
        }
        if (g_has_cancel)
        {
            g_button = create_button_textured(g_defocus, 0.0, 90.0);
            g_button_label = create_button_label(g_defocus, 0.0, 90.0);
            SetColor(g_button_label, 0.215, 0.708, 0.999, 0.0);
            SetOptionS(g_button_label, TAG("UTF8"), cancel_label);
            SetOptionI(g_button_label, TAG("ALPH"), BLEND_ADD);
        }
        if (g_show_percent)
        {
            new percent_text[8];
            sprintf(percent_text, percent_format, 0, 0);
            g_percent_label = create_button_label(g_defocus, 0.0, 30.0);
            SetColor(g_percent_label, 1.0, 1.0, 1.0, 0.0);
            SetOptionS(g_percent_label, TAG("UTF8"), percent_text);
        }
    }
    sndSePlay(SE_DIALOG_OPEN);
    fade_window(1.0, 300.0);
    AnimColor(g_defocus, 0.35, 0.35, 0.35, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(300);
    show_pad_help();
    if (g_has_cancel)
    {
        send_focus(focus, g_button, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    }
    for (; ; )
    {
        if (g_has_cancel)
        {
            new pressed;
            PadState(_, pressed, _, _, -1);
            if (pressed & PAD_CIRCLE)
            {
                sndSePlay(SE_DECIDE);
                SetPubVar(g_bar, "progress", progress);
                break;
            }
        }
        if (focus)
        {
            new Float:focus_pos[3];
            GetPos(focus, focus_pos[0], focus_pos[1], focus_pos[2], g_window);
            SetPos(g_glow, -focus_pos[0], focus_pos[1], 0.0, 0);
            new Float:focus_alpha;
            GetColor(focus, _, _, _, focus_alpha);
            AnimColor(g_glow, 0.5 * focus_alpha, 0.5 * focus_alpha, 0.5 * focus_alpha, 1.0, 100.0, 0.0,
                    ANIM_FUNC_LINEAR);
        }
        if (g_show_percent)
        {
            new percent_text[8];
            sprintf(percent_text, percent_format, progress, progress2);
            SetOptionS(g_percent_label, TAG("UTF8"), percent_text);
        }
        SetPubVar(g_bar, "progress", progress);
        if (progress >= 100)
        {
            break;
        }
        sleep;
    }
    wait_ms(500);
    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    fade_window(0.0, 300.0);
    AnimColor(g_defocus, 1.0, 1.0, 1.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(300);
    return;
}

/* Fades the window, the messages, the button and the percentage to alpha over duration ms. */
fade_window(Float:alpha, Float:duration)
{
    AnimColor(g_window, red, green, blue, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    for (new i; i < g_message_count; i++)
    {
        AnimColor(g_messages[i], 0.7843, 0.7843, 0.7843, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_has_cancel)
    {
        AnimColor(g_button, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_has_cancel)
    {
        AnimColor(g_button_label, 0.7843, 0.7843, 0.7843, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_show_percent)
    {
        AnimColor(g_percent_label, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
}

/* Shows the circle in the pad help when the window can be cancelled. */
show_pad_help()
{
    new buttons = g_has_cancel ? PADHELP_CIRCLE : 0;
    SetSystemParam(SYS_PADHELP, buttons);
}
