/*
 * /opt0/bn/script/parts/confirm_yesno.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A question box with two buttons (labels: arguments "yes" and "no"), chosen with left and right and pressed with
 * the circle button; the cross button cancels. The answer is left in select and the script ends when the box is gone.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "wait.inc"

#include "tag.inc"

#include <float>

#include "text.inc"

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
public focus; /* the focus object, moved onto the selected button */
public select; /* in: the button selected first; out: 0 yes, 1 no, -1 cancelled */
public Float:width = 400.0; /* of the window, in pixels */
public Float:height = 260.0; /* of the window, in pixels */
public Float:red; /* color of the window */
public Float:green = 0.2353; /* color of the window */
public Float:blue = 0.3922; /* color of the window */
new g_defocus;
new g_base;
new g_window;
new g_glow;
public Float:border_left; /* the window is this much wider on the left */
new g_message;
new g_button_labels[2];
new g_buttons[2];
new g_button_lit;
new g_message_shown;

#include "dialog.inc"

#include "button.inc"

/* Waits until the label of button index has laid out its text, then enlarges button_w and button_h to hold it. */
fit_buttons_to_label(index, &Float:button_w, &Float:button_h)
{
    for (new rect[5]; ; )
    {
        GetOptionP(g_button_labels[index], TAG("RECT"), rect, 5);
        if (rect[0])
        {
            new Float:extent = float(rect[3] >> 6) + float(30);
            if (button_w < extent)
            {
                button_w = extent;
            }
            extent = 20.0 + float(rect[4] >> 6);
            if (button_h < extent)
            {
                button_h = extent;
            }
            break;
        }
        sleep;
    }
}

main()
{
    new message[128];
    GetArg("msg", message, 128);
    new yes_label[8];
    GetArg("yes", yes_label, 8);
    new no_label[8];
    GetArg("no", no_label, 8);
    select = clamp(select, 0, 1);
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
    g_message = create_message_text_add(g_defocus, floatround(width - 64.0, 0), floatround(height - 48.0, 0));
    SetOptionS(g_message, TAG("UTF8"), message);
    SetColor(g_message, 0.7843, 0.7843, 0.7843, 0.0);
    g_buttons[0] = create_button(g_defocus, -64.0, 78.0);
    SetTexture(g_buttons[0], imgGetResource(RES_BUTTON0), 0, -1);
    g_button_labels[0] = create_button_label_add(g_defocus, -64.0, 78.0);
    SetColor(g_button_labels[0], 0.215, 0.708, 0.999, 0.0);
    SetOptionS(g_button_labels[0], TAG("UTF8"), yes_label);
    g_buttons[1] = create_button(g_defocus, 64.0, 78.0);
    SetTexture(g_buttons[1], imgGetResource(RES_BUTTON0), 0, -1);
    g_button_labels[1] = create_button_label_add(g_defocus, 64.0, 78.0);
    SetColor(g_button_labels[1], 0.215, 0.708, 0.999, 0.0);
    SetOptionS(g_button_labels[1], TAG("UTF8"), no_label);
    g_button_lit = create_button(g_defocus, 0.0, 78.0);
    SetTexture(g_button_lit, imgGetResource(RES_BUTTON1), 0, -1);
    Hide(g_button_lit);
    {
        new Float:button_w;
        new Float:button_h;
        new Float:button_x;
        fit_buttons_to_label(0, button_w, button_h);
        fit_buttons_to_label(1, button_w, button_h);
        button_x = -12.0 - 0.5 * button_w;
        SetPos(g_buttons[0], button_x, 78.0, 0.0, 0);
        SetPos(g_button_labels[0], button_x, 78.0, 0.0, 0);
        SetScale(g_buttons[0], button_w, button_h, 1.0);
        button_x = 12.0 + 0.5 * button_w;
        SetPos(g_buttons[1], button_x, 78.0, 0.0, 0);
        SetPos(g_button_labels[1], button_x, 78.0, 0.0, 0);
        SetScale(g_buttons[1], button_w, button_h, 1.0);
        SetScale(g_button_lit, button_w, button_h, 1.0);
    }
    sndSePlay(SE_DIALOG_OPEN);
    fade_window(1.0, 300.0);
    AnimColor(g_defocus, 0.35, 0.35, 0.35, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(300);
    SetOptionI(focus, TAG("ZSRT"), ZSORT_OVERLAY);
    send_focus(focus, g_buttons[select], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    set_pad_help(PADHELP_CIRCLE | PADHELP_CROSS);
    for (; ; )
    {
        {
            new pressed;
            PadState(_, pressed, _, _, -1);
            if (pressed & PAD_RIGHT && !select)
            {
                sndSePlay(SE_CURSOR);
                select++;
                send_focus(focus, g_buttons[select], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                fade_window(1.0, 100.0);
            }
            else if (pressed & PAD_LEFT && select)
            {
                sndSePlay(SE_CURSOR);
                select--;
                send_focus(focus, g_buttons[select], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                fade_window(1.0, 100.0);
            }
            if (pressed & PAD_CIRCLE && !is_animating(focus))
            {
                sndSePlay(SE_DECIDE);
                {
                    new Float:x;
                    new Float:y;
                    GetPos(g_buttons[select], x, y, _, 0);
                    SetPos(g_button_lit, x, y, 0.0, 0);
                    /* Flashes the lit texture over the selected button. */
                    Show(g_button_lit);
                    AnimColor(g_buttons[select], 0.0, 0.54, 0.78, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    AnimColor(g_button_lit, 0.0, 0.54, 0.78, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    wait_ms(100);
                    AnimColor(g_buttons[select], 0.0, 0.54, 0.78, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    AnimColor(g_button_lit, 0.0, 0.54, 0.78, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                    wait_ms(100);
                    Hide(g_button_lit);
                }
                break;
            }
            if (pressed & PAD_CROSS && !is_animating(focus))
            {
                sndSePlay(SE_CANCEL);
                select = -1;
                break;
            }
        }
        /* The glow on the window mirrors the position of the focus and follows its brightness. */
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
        /* Centers the message once the textbox has laid it out, then fades it in. */
        if (!g_message_shown)
        {
            new rect[5];
            GetOptionP(g_message, TAG("RECT"), rect, 5);
            if (rect[0])
            {
                g_message_shown = 1;
                SetPos(g_message, 0.5 * -float(rect[3] >> 6), -8.0 - 0.5 * float(rect[4] >> 6), 0.0, 0);
                AnimColor(g_message, 0.7843, 0.7843, 0.7843, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        sleep;
    }
    set_pad_help(0);
    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    fade_window(0.0, 300.0);
    AnimColor(g_message, 0.7843, 0.7843, 0.7843, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_defocus, 1.0, 1.0, 1.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(300);
    SetOptionI(focus, TAG("ZSRT"), 1); /* acts as ZSORT_NONE */
    return;
}

/* Fades the window and the buttons to alpha over duration ms, the label of the selected button in white. */
fade_window(Float:alpha, Float:duration)
{
    AnimColor(g_window, red, green, blue, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    if (select)
    {
        AnimColor(g_button_labels[0], 0.215, 0.708, 0.999, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_button_labels[1], 0.7843, 0.7843, 0.7843, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    else
    {
        AnimColor(g_button_labels[0], 0.7843, 0.7843, 0.7843, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_button_labels[1], 0.215, 0.708, 0.999, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    AnimColor(g_buttons[0], 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_buttons[1], 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
}

/* Sets the buttons the pad help shows (PADHELP_* bits). */
set_pad_help(buttons)
{
    SetSystemParam(SYS_PADHELP, buttons);
}
