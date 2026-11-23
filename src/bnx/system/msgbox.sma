/*
 * /opt0/bn/script/system/msgbox.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The system message box that msgboxStart opens, with its arguments in the aval variables msgbox-*: a
 * message, with Yes and No buttons for MSGBOX_YESNO. The answer goes back in msgbox-return.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

/* Values of msgbox-type. */
const MSGBOX_INFO = 0;
const MSGBOX_ERROR = 1; /* red */
const MSGBOX_YESNO = 2;

/* Same as wait_ms of wait.inc. */
wait_ms(ms)
{
    new frames = ms * FrameRate() / 1000;
    for (new i; i < frames; i++)
    {
        sleep;
    }
}

/* Same as TAG of tag.inc. */
TAG(fourcc[])
{
    return swapchars(fourcc[0]);
}

#include <float>

/* Creates the focus plane on scene 0 and attaches script to it. */
create_focus_plane(script[])
{
    new plane = CreatePlane3x3(Scene(0));
    SetTexture(plane, imgGetResource(RES_FOCUS), 0, -1);
    SetOptionI(plane, TAG("ALPH"), BLEND_ADD);
    AttachScript(plane, "", script, "");
    return plane;
}

is_animating(obj)
{
    return ANIM_DONE != AnimGetState(obj, 0);
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

new Float:g_screen_width;
new Float:g_screen_height;
new Float:g_screen_depth;
new g_type;
new g_msg[128];
new g_yes_label[16];
new g_no_label[16];
new g_choice = 1; /* the button with the focus: 0 Yes, 1 No */
new Float:g_width = 400.0;
new Float:g_height = 260.0;
/* The color of the window for each type. */
new Float:g_red[3] = {0.0, 0.549};
new Float:g_green[3] = {0.2353, 0.0784, 0.2353};
new Float:g_blue[3] = {0.3922, 0.1961, 0.3922};
new Float:g_extra_width;
new Float:g_text_offset_y[3] = {0.0, -8.0};
new g_focus;
new g_defocus;
new g_base;
new g_window;
new g_shadow;
new g_message;
new g_button_labels[2];
new g_buttons[2];
new g_button_flash;

/* Same as get_screen_size of text.inc. */
get_screen_size(&Float:screen_w, &Float:screen_h)
{
    GetScreen(Camera(0), g_screen_width, g_screen_height, g_screen_depth);
    screen_w = g_screen_width;
    screen_h = g_screen_height;
}

main()
{
    {
        new value[3];
        if (avalGetNum("msgbox-type"))
        {
            avalGetValue("msgbox-type", 0, value, 3);
            g_type = atoi(value);
            g_type = clamp(g_type, MSGBOX_INFO, MSGBOX_YESNO);
        }
        if (avalGetNum("msgbox-msg"))
        {
            avalGetValue("msgbox-msg", 0, g_msg, 128);
        }
        if (avalGetNum("msgbox-yes"))
        {
            avalGetValue("msgbox-yes", 0, g_yes_label, 16);
        }
        if (avalGetNum("msgbox-no"))
        {
            avalGetValue("msgbox-no", 0, g_no_label, 16);
        }
        if (avalGetNum("msgbox-flag"))
        {
            avalGetValue("msgbox-flag", 0, value, 3);
            new flags = atoi(value);
            if (flags & 1) /* Yes has the focus first */
            {
                g_choice = 0;
            }
        }
    }
    SetSystemParam(SYS_PADHELP, PADHELP_CIRCLE);
    new Float:screen_w;
    new Float:screen_h;
    get_screen_size(screen_w, screen_h);
    if (g_type == MSGBOX_YESNO)
    {
        g_focus = create_focus_plane("focus");
    }
    sndSePlay(SE_WINDOW_OPEN);
    g_defocus = CreateDefocus(Disp2D(0));
    SetPos(g_defocus, 320.0, 240.0, 0.0, 0);
    SetColor(g_defocus, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_defocus, TAG("LEVL"), 3);
    SetOptionI(g_defocus, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_defocus, TAG("ZOFS"), -3.0);
    g_base = CreatePlane(g_defocus);
    SetLocalScale(g_base, 2.0 * g_width, 2.0 * g_height, 1.0);
    SetColor(g_base, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_base, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_base, TAG("ZOFS"), -2.0);
    SetOptionI(g_base, TAG("ALPH"), BLEND_KEEP);
    g_window = CreatePlane3x3(g_base);
    SetScale(g_window, g_width + g_extra_width, g_height, 1.0);
    SetPos(g_window, 0.5 * -g_extra_width, 0.0, 0.0, 0);
    SetColor(g_window, g_red[g_type], g_green[g_type], g_blue[g_type], 0.0);
    SetTexture(g_window, imgGetResource(RES_WINDOW1), 0, -1);
    SetOptionI(g_window, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_window, TAG("ZOFS"), -1.0);
    SetOptionI(g_window, TAG("ALPH"), BLEND_ADD);
    {
        new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
        SetOptionP(g_window, TAG("MGNT"), tex_margins, 4);
        new Float:margins[4];
        margins[0] = margins[1] = 0.5 * 128.0 / g_width;
        margins[2] = margins[3] = 0.5 * 128.0 / g_height;
        SetOptionP(g_window, TAG("MGN"), margins, 4);
    }
    g_shadow = CreatePlane(g_window);
    SetPos(g_shadow, 0.3, -0.2, 0.0, 0);
    SetScale(g_shadow, 2.0, 2.0, 1.0);
    SetColor(g_shadow, 0.0, 0.0, 0.0, 0.0);
    SetTexture(g_shadow, imgGetResource(RES_DOT), 0, -1);
    SetOptionI(g_shadow, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(g_shadow, TAG("ALPH"), BLEND_ADD | BLEND_DEST_ALPHA);
    g_message = create_message_text_add(g_defocus, floatround(g_width - 64.0, 0), floatround(g_height - 48.0, 0));
    SetOptionS(g_message, TAG("UTF8"), g_msg);
    SetColor(g_message, 0.7843, 0.7843, 0.7843, 0.0);
    if (g_type == MSGBOX_YESNO)
    {
        g_buttons[0] = create_button(g_defocus, -64.0, 78.0);
        SetTexture(g_buttons[0], imgGetResource(RES_BUTTON0), 0, -1);
        g_button_labels[0] = create_button_label_add(g_defocus, -64.0, 78.0);
        SetColor(g_button_labels[0], 0.215, 0.708, 0.999, 0.0);
        SetOptionS(g_button_labels[0], TAG("UTF8"), g_yes_label);
        g_buttons[1] = create_button(g_defocus, 64.0, 78.0);
        SetTexture(g_buttons[1], imgGetResource(RES_BUTTON0), 0, -1);
        g_button_labels[1] = create_button_label_add(g_defocus, 64.0, 78.0);
        SetColor(g_button_labels[1], 0.215, 0.708, 0.999, 0.0);
        SetOptionS(g_button_labels[1], TAG("UTF8"), g_no_label);
        g_button_flash = create_button(g_defocus, 0.0, 78.0);
        SetTexture(g_button_flash, imgGetResource(RES_BUTTON1), 0, -1);
        Hide(g_button_flash);
        new Float:button_w;
        new Float:button_h;
        new Float:x;
        fit_buttons_to_label(0, button_w, button_h);
        fit_buttons_to_label(1, button_w, button_h);
        x = -12.0 - 0.5 * button_w;
        SetPos(g_buttons[0], x, 78.0, 0.0, 0);
        SetPos(g_button_labels[0], x, 78.0, 0.0, 0);
        SetScale(g_buttons[0], button_w, button_h, 1.0);
        x = 12.0 + 0.5 * button_w;
        SetPos(g_buttons[1], x, 78.0, 0.0, 0);
        SetPos(g_button_labels[1], x, 78.0, 0.0, 0);
        SetScale(g_buttons[1], button_w, button_h, 1.0);
        SetScale(g_button_flash, button_w, button_h, 1.0);
    }
    sndSePlay(SE_DIALOG_OPEN);
    fade_window(1.0, 300.0);
    AnimColor(g_defocus, 0.35, 0.35, 0.35, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(300);
    if (g_type == MSGBOX_YESNO)
    {
        send_focus(g_focus, g_buttons[g_choice], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        set_pad_help(PADHELP_CIRCLE | PADHELP_CROSS);
    }
    else
    {
        set_pad_help(PADHELP_CROSS);
    }
    for (; ; )
    {
        {
            new pressed;
            PadState(_, pressed, _, _, -1);
            if (g_type == MSGBOX_YESNO)
            {
                if (pressed & PAD_RIGHT && !g_choice)
                {
                    sndSePlay(SE_CURSOR);
                    g_choice++;
                    send_focus(g_focus, g_buttons[g_choice], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    fade_window(1.0, 100.0);
                }
                else if (pressed & PAD_LEFT && g_choice)
                {
                    sndSePlay(SE_CURSOR);
                    g_choice--;
                    send_focus(g_focus, g_buttons[g_choice], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    fade_window(1.0, 100.0);
                }
                if (pressed & PAD_CIRCLE && !is_animating(g_focus))
                {
                    sndSePlay(SE_DECIDE);
                    {
                        new Float:x;
                        new Float:y;
                        GetPos(g_buttons[g_choice], x, y, _, 0);
                        SetPos(g_button_flash, x, y, 0.0, 0);
                        Show(g_button_flash);
                        AnimColor(g_buttons[g_choice], 0.0, 0.54, 0.78, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                        AnimColor(g_button_flash, 0.0, 0.54, 0.78, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                        wait_ms(100);
                        AnimColor(g_buttons[g_choice], 0.0, 0.54, 0.78, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                        AnimColor(g_button_flash, 0.0, 0.54, 0.78, 0.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
                        wait_ms(100);
                        Hide(g_button_flash);
                    }
                    if (0 == g_choice)
                    {
                        set_answer(1);
                    }
                    else
                    {
                        set_answer(2);
                    }
                    break;
                }
                if (pressed & PAD_CROSS && !is_animating(g_focus))
                {
                    sndSePlay(SE_CANCEL);
                    g_choice = -1;
                    set_answer(-1);
                    break;
                }
            }
            else if (pressed & PAD_CROSS)
            {
                sndSePlay(SE_CANCEL);
                set_answer(-1);
                break;
            }
        }
        if (g_type == MSGBOX_YESNO && g_focus)
        {
            new Float:pos[3];
            GetPos(g_focus, pos[0], pos[1], pos[2], g_window);
            SetPos(g_shadow, -pos[0], pos[1], 0.0, 0);
            new Float:alpha;
            GetColor(g_focus, _, _, _, alpha);
            AnimColor(g_shadow, 0.5 * alpha, 0.5 * alpha, 0.5 * alpha, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
        }
        static text_shown;
        if (!text_shown)
        {
            new rect[5];
            GetOptionP(g_message, TAG("RECT"), rect, 5);
            if (rect[0])
            {
                text_shown = 1;
                SetPos(g_message, 0.5 * -float(rect[3] >> 6), g_text_offset_y[g_type] - 0.5 * float(rect[4] >> 6), 0.0,
                        0);
                AnimColor(g_message, 0.7843, 0.7843, 0.7843, 1.0, 100.0, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        sleep;
    }
    set_pad_help(0);
    send_focus(g_focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    fade_window(0.0, 300.0);
    AnimColor(g_message, 0.7843, 0.7843, 0.7843, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_defocus, 1.0, 1.0, 1.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(300);
    Destroy(Scene(0));
}

/* Same as create_message_text_add of dialog.inc, with "ZSRT" set before "ALPH". */
create_message_text_add(parent, text_w, text_h)
{
    new textbox = CreateTextbox(parent);
    SetOptionI(textbox, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(textbox, TAG("ALPH"), BLEND_ADD);
    SetOptionI(textbox, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(textbox, TAG("FSIZ"), 15);
    SetOptionI(textbox, TAG("TW64"), text_w << 6);
    SetOptionI(textbox, TAG("TH64"), text_h << 6);
    SetOptionI(textbox, TAG("LS64"), 4 * 64);
    return textbox;
}

fade_window(Float:alpha, Float:duration)
{
    AnimColor(g_window, g_red[g_type], g_green[g_type], g_blue[g_type], alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    if (g_type == MSGBOX_YESNO)
    {
        if (g_choice)
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
}

/* Sets the buttons the pad help shows (PADHELP_* bits). */
set_pad_help(buttons)
{
    SetSystemParam(SYS_PADHELP, buttons);
}

/* Same as create_button_label_add of dialog.inc. */
create_button_label_add(parent, Float:x, Float:y)
{
    new label = CreateLabel(parent);
    SetPos(label, x, y, 0.0, 0);
    SetOptionI(label, TAG("ALPH"), BLEND_ADD);
    SetOptionI(label, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("VALN"), ALIGN_CENTER);
    SetOptionI(label, TAG("FSIZ"), 15);
    SetOptionI(label, TAG("AP64"), 1 * 64); /* 1 pixel */
    SetOptionI(label, TAG("KP64"), 1 * 64); /* 1 pixel */
    SetOptionI(label, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(label, TAG("ZOFS"), 2.0);
    return label;
}

/* Same as create_button of button.inc. */
create_button(parent, Float:x, Float:y)
{
    new button = CreatePlane3x3(parent);
    SetPos(button, x, y, 0.0, 0);
    SetScale(button, 80.0, 32.0, 1.0);
    SetColor(button, 0.0, 0.54, 0.78, 0.0);
    SetOptionI(button, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(button, TAG("ZOFS"), 1.0);
    SetOptionI(button, TAG("ALPH"), BLEND_ADD);
    new Float:tex_margins[4] = {0.5, 0.5, 0.5, 0.5};
    SetOptionP(button, TAG("MGNT"), tex_margins, 4);
    new Float:scale[3];
    GetScale(button, scale[0], scale[1], scale[2]);
    new Float:margins[4];
    if (scale[0] > scale[1])
    {
        margins[0] = margins[1] = 0.5 * scale[1] / scale[0];
        margins[2] = margins[3] = 0.5;
    }
    else
    {
        margins[2] = margins[3] = 0.5 * scale[0] / scale[1];
        margins[0] = margins[1] = 0.5;
    }
    SetOptionP(button, TAG("MGN"), margins, 4);
    return button;
}

/* Like fit_button_to_label, for the label of button index. */
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

/* Answers 1 (Yes), 2 (No) or -1 (cross) in msgbox-return. */
set_answer(answer)
{
    new text[4];
    sprintf(text, "%d", answer);
    avalSet("msgbox-return", text);
}
