/*
 * /opt0/bn/script/parts/dialog.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A modal dialog, described by the "dialog" tag of the page whose id is the argument "id" (root) or by its
 * arguments: a title, a message (a textbox, centered lines or a content script), up to two buttons for the type
 * "confirm" and a progress bar for the type "progress". The answer is left in select and canceled.
 */

#include <core>
#include <bn>

#pragma dynamic 512

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

/* Appends src to dest (maxlen cells), packed or unpacked like dest. */
append_string(dest[], src[], maxlen)
{
    new dest_len;
    new src_len;
    new i;
    new room;
    dest_len = strlen(dest);
    src_len = strlen(src);
    if (is_packed(dest))
    {
        room = maxlen * 4 - dest_len - 1;
        if (is_packed(src))
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest{dest_len + i} = src{i};
            }
        }
        else
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest{dest_len + i} = src[i];
            }
        }
        new pad = 4 - (dest_len + i) % 4;
        for (new k; k < pad; k++)
        {
            dest{dest_len + i + k} = 0;
        }
    }
    else
    {
        room = maxlen - dest_len - 1;
        if (is_packed(src))
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest[dest_len + i] = src{i};
            }
        }
        else
        {
            i = 0;
            for (; i < src_len && i < room; i++)
            {
                dest[dest_len + i] = src[i];
            }
        }
        dest[dest_len + i] = 0;
    }
    return;
}

#include "text.inc"

/* Copies src into dest (maxlen cells) with every occurrence of from replaced by to. */
replace_all(dest[], src[], from[], to[], maxlen)
{
    new count;
    new from_len = strlen(from);
    new to_len = strlen(to);
    new src_pos;
    new dest_pos;
    for (; ; )
    {
        new found = find_substring(src, from, src_pos);
        if (found < 0)
        {
            copy_substring(dest, dest_pos, src, src_pos, -1, maxlen);
            return count;
        }
        copy_substring(dest, dest_pos, src, src_pos, found - src_pos, maxlen);
        dest_pos = dest_pos + (found - src_pos);
        append_string(dest, to, maxlen);
        dest_pos = dest_pos + to_len;
        src_pos = found + from_len;
    }
    return 0;
}

/* Same as is_animating of text.inc. */
is_animating2(obj)
{
    return ANIM_DONE != AnimGetState(obj, ANIM_MOVE);
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

public root; /* the "dialog" tag that describes the dialog, or 0 to take its arguments */
public manager; /* unused, but part of the layout */
public focus; /* the focus object */
public padhelp; /* the buttons that answer: PADHELP_CIRCLE, PADHELP_CROSS */
public ready; /* 1 once the dialog is open */
public canceled; /* 1 when the cross closed the dialog; set by the page, it closes the dialog */
public select; /* in: the button selected first; out: the button pressed */
/* Sound effects for soundSePlay; 0 plays the default one. */
public se_open; /* when the dialog opens */
public se_close; /* when it closes (no default) */
public se_cursor; /* when the selection moves */
public se_ok; /* when a button is pressed */
public se_cancel; /* when the cross cancels */
public Float:width = 400.0; /* of the window, in pixels */
public Float:height = 260.0; /* of the window, in pixels */
public Float:red; /* color of the window */
public Float:green = 0.2353; /* color of the window */
public Float:blue = 0.3922; /* color of the window */
public Float:bgcolor = 0.35; /* how bright the dimmed screen stays */
public Float:border_left; /* the window is this much wider on the left */
new g_defocus;
new g_base;
new g_window;
new g_glow;
new g_title;
new g_content; /* the message textbox, or the object of the content script */
new g_content_is_script;
new g_is_confirm;
new g_button_count;
new g_buttons[2];
new g_button_lit;
new g_button_labels[2];
public Float:progress; /* percent shown by the progress bar */
new Float:g_shown_progress;
new g_bar_frame;
new g_bar_glow;
new g_bar_fill;
new g_percent_label;
new g_is_progress;
new g_show_percent;
new g_percent_format[8]; /* format of the percent label, from the setting "percent" */

/* Plays the sound effect sound, or the stock effect default_se if sound is 0 (none if negative). */
play_sound(sound, default_se)
{
    if (sound)
    {
        soundSePlay(sound, 127);
    }
    else if (default_se >= 0)
    {
        sndSePlay(default_se);
    }
}

/* Creates g_base, the window and the glow over it. */
create_window(parent, Float:window_w, Float:window_h)
{
    g_base = CreatePlane(parent);
    SetLocalScale(g_base, 2.0 * window_w, 2.0 * window_h, 1.0);
    SetColor(g_base, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_base, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_base, TAG("ZOFS"), -2.0);
    SetOptionI(g_base, TAG("ALPH"), BLEND_KEEP);
    g_window = CreatePlane(g_base);
    SetScale(g_window, window_w + border_left, window_h, 1.0);
    SetPos(g_window, 0.5 * -border_left, 0.0, 0.0, 0);
    SetColor(g_window, red, green, blue, 0.0);
    SetTexture(g_window, imgGetResource(RES_WINDOW1), 0, -1);
    SetOptionI(g_window, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_window, TAG("ZOFS"), -1.0);
    SetOptionI(g_window, TAG("ALPH"), BLEND_ADD);
    SetOptionF(g_window, TAG("MGNT"), 0.5, 0.5, 0.5, 0.5);
    window_w = 0.5 * 128.0 / window_w;
    window_h = 0.5 * 128.0 / window_h;
    SetOptionF(g_window, TAG("MGN"), window_w, window_w, window_h, window_h);
    g_glow = CreatePlane(g_window);
    SetPos(g_glow, 0.3, -0.2, 0.0, 0);
    SetScale(g_glow, 2.0, 2.0, 1.0);
    SetColor(g_glow, 0.0, 0.0, 0.0, 0.0);
    SetTexture(g_glow, imgGetResource(RES_DOT), 0, -1);
    SetOptionI(g_glow, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(g_glow, TAG("ALPH"), BLEND_ADD | BLEND_DEST_ALPHA);
}

/* Same as create_message_text_add of dialog.inc, with the size as floats. */
create_message_textbox(parent, Float:text_w, Float:text_h)
{
    new textbox = CreateTextbox(parent);
    SetOptionI(textbox, TAG("ALPH"), BLEND_ADD);
    SetOptionI(textbox, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(textbox, TAG("HALN"), ALIGN_CENTER);
    SetOptionI(textbox, TAG("FSIZ"), 15);
    SetOptionI(textbox, TAG("TW64"), floatround(text_w, 0) << 6);
    SetOptionI(textbox, TAG("TH64"), floatround(text_h, 0) << 6);
    SetOptionI(textbox, TAG("LS64"), 4 * 64);
    return textbox;
}

/* Creates a label for each line of text, in the list; line is the buffer for one line. */
add_line_labels(parent, text[], line[], maxlen)
{
    for (new pos; (pos = get_line(line, text, pos, maxlen)) > 0; )
    {
        new label = create_centered_label(parent);
        SetOptionS(label, TAG("UTF8"), line);
        SetColor(label, 0.215, 0.708, 0.999, 0.0);
        listAddLast(label);
    }
}

/* Places the labels of the list 20 apart, from (x, y). */
place_line_labels(Float:x, Float:y)
{
    for (new i; i < listSize(); i++)
    {
        new label = listGetAt(i);
        SetPos(label, x, y, 0.0, 0);
        y = 20.0 + y;
    }
}

/* Runs script on a new object as the content of the dialog, which then gets the focus with up and down. */
attach_content(parent, script[], Float:content_w, Float:content_h)
{
    new content = CreateNull(parent);
    AttachScript(content, "", script, "");
    SetPubVar(content, "root", root);
    SetPubVar(content, "manager", Self());
    SetPubVar(content, "focus", focus);
    SetPubVar(content, "padhelp", padhelp);
    SetPubVar(content, "width", content_w);
    SetPubVar(content, "height", content_h);
    return content;
}

/* Creates the frame, the fill and the glow of the progress bar, 280 wide, at height y. */
create_progress_bar(parent, Float:y)
{
    g_bar_frame = CreatePlane(parent);
    SetPos(g_bar_frame, 0.0, y, 0.0, 0);
    SetScale(g_bar_frame, 280.0 + 8.0, 5.0, 1.0);
    SetColor(g_bar_frame, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_bar_frame, imgGetResource(RES_BAR1), 0, -1);
    SetOptionI(g_bar_frame, TAG("ALPH"), BLEND_ADD);
    SetOptionI(g_bar_frame, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_bar_frame, TAG("MGNT"), 0.5, 0.5, 0.5, 0.5);
    SetOptionF(g_bar_frame, TAG("MGN"), 0.1, 0.1, 0.5, 0.5);
    g_bar_glow = CreatePlane(parent);
    SetPos(g_bar_glow, 0.5 * -(280.0 + 10.0), 0.5 + y, 0.0, 0);
    SetScale(g_bar_glow, -10.0, 1.0, 1.0);
    SetColor(g_bar_glow, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_bar_glow, imgGetResource(RES_BAR2), 0, -1);
    SetOptionI(g_bar_glow, TAG("ALPH"), BLEND_ADD);
    SetOptionI(g_bar_glow, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_bar_glow, TAG("MGNT"), 0.5, 0.5, 0.5, 0.5);
    SetOptionF(g_bar_glow, TAG("MGN"), 0.5, 0, 0.5, 0.5);
    g_bar_fill = CreatePlane(parent);
    SetPos(g_bar_fill, -280.0 * 0.5, y, 0.0, 0);
    SetScale(g_bar_fill, 0.0, 7.0, 1.0);
    SetColor(g_bar_fill, 1.0, 1.0, 1.0, 0.0);
    SetTexture(g_bar_fill, imgGetResource(RES_BAR3), 0, -1);
    SetOptionI(g_bar_fill, TAG("ALPH"), BLEND_ADD);
    SetOptionI(g_bar_fill, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_bar_fill, TAG("MGNT"), 0.0, 0.5, 0.5, 0.5);
    SetOptionF(g_bar_fill, TAG("MGN"), 0.0, 0.5, 0.5, 0.5);
}

/* Moves the fill of the bar to progress over duration ms and updates the percent label. */
update_progress_bar(Float:duration)
{
    if (g_shown_progress != progress)
    {
        new Float:y;
        GetPos(g_bar_frame, _, y, _, 0);
        AnimScale(g_bar_glow, 280.0 * progress / 100.0 - 10.0, 1.0, 1.0, duration, 0.0, ANIM_FUNC_LINEAR);
        AnimMoveLine(g_bar_glow, 280.0 * -0.5 + 280.0 * 0.5 * progress / 100.0 - 10.0 * 0.5, 0.5 + y, 0.0, duration,
                0.0, ANIM_FUNC_LINEAR);
        AnimScale(g_bar_fill, 280.0 * progress / 100.0, 7.0, 1.0, duration, 0.0, ANIM_FUNC_LINEAR);
        AnimMoveLine(g_bar_fill, 280.0 * -0.5 + 280.0 * 0.5 * progress / 100.0, y, 0.0, duration, 0.0,
                ANIM_FUNC_LINEAR);
        g_shown_progress = progress;
    }
    new Float:fill_w;
    GetScale(g_bar_fill, fill_w);
    if (fill_w < 20.0)
    {
        AnimColor(g_bar_glow, 1.0, 1.0, 1.0, 0.0, duration, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_bar_fill, 1.0, 1.0, 1.0, fill_w / 20.0, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    else if (fill_w < 150.0)
    {
        SetOptionF(g_bar_fill, TAG("MGN"), 0.0, 7.0 / fill_w, 0.5, 0.5);
        AnimColor(g_bar_fill, 1.0, 1.0, 1.0, 1.0, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    else
    {
        new Float:cap = 7.0 / fill_w;
        SetOptionF(g_bar_fill, TAG("MGN"), 1.0 - (150.0 - 20.0 * 0.5) / fill_w - cap, cap, 0.5, 0.5);
        AnimColor(g_bar_fill, 1.0, 1.0, 1.0, 1.0, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    if (g_show_percent)
    {
        new text[8];
        sprintf(text, g_percent_format, 100.0 * fill_w / 280.0);
        SetOptionS(g_percent_label, TAG("UTF8"), text);
    }
}

create_centered_label(parent)
{
    new label = CreateLabel(parent);
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

create_button_plane(parent)
{
    new plane = CreatePlane(parent);
    SetColor(plane, 0.0, 0.54, 0.78, 0.0);
    SetOptionI(plane, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(plane, TAG("ZOFS"), 1.0);
    SetOptionI(plane, TAG("ALPH"), BLEND_ADD);
    SetOptionF(plane, TAG("MGNT"), 0.5, 0.5, 0.5, 0.5);
    return plane;
}

set_button_size(button, Float:button_w, Float:button_h)
{
    SetScale(button, button_w, button_h, 1.0);
    if (button_w > button_h)
    {
        button_w = 0.5 * button_h / button_w;
        button_h = 0.5;
    }
    else
    {
        button_h = 0.5 * button_w / button_h;
        button_w = 0.5;
    }
    SetOptionF(button, TAG("MGN"), button_w, button_w, button_h, button_h);
}

/* Crossfades the selected button to its lit texture (alpha 1.0) or back (0.0), and waits. */
flash_button(Float:alpha, Float:duration)
{
    AnimColor(g_buttons[select], 0.0, 0.54, 0.78, 1.0 - alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    AnimColor(g_button_lit, 0.0, 0.54, 0.78, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(floatround(duration, 0));
}

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

/* Returns the tag called name whose id is the argument "id", or 0. */
find_by_id(name[])
{
    new id[16];
    if (GetArg("id", id, 16))
    {
        return domFindTag(domRoot(), name, id, 0);
    }
    return 0;
}

/* Gets the attribute name of tag, or the argument name without a tag. */
get_setting(tag, name[], value[], maxlen)
{
    return tag ? domGetData(tag, name, value, maxlen) : GetArg(name, value, maxlen);
}

/*
 * Gets the text of the next child name of root after tag (the "TEXT" tag of its id if it has one), or without
 * root the argument name followed by index (name alone if index is negative). Returns 0 when there is none.
 */
get_text(&tag=0, name[], text[], index, maxlen)
{
    if (root)
    {
        if (tag = tag ? domNext(tag, name) : domFirstChild(root, name))
        {
            if (domGetData(tag, "id", text, maxlen))
            {
                new text_tag = domFindTag(domRoot(), "TEXT", text, 0);
                return domGetData(text_tag, "value", text, maxlen);
            }
            return domGetData(tag, "", text, maxlen);
        }
        return 0;
    }
    if (index >= 0)
    {
        new key[4];
        sprintf(key, "%s%d", name, index);
        return GetArg(key, text, maxlen);
    }
    return GetArg(name, text, maxlen);
}

#include "fileop.inc"

main()
{
    if (!root)
    {
        root = find_by_id("dialog");
    }
    {
        new type[16];
        if (get_setting(root, "type", type, 16))
        {
            if (!strcmp(type, "confirm"))
            {
                g_is_confirm = 1;
            }
            else if (!strcmp(type, "progress"))
            {
                g_is_progress = 1;
                if (get_setting(root, "percent", g_percent_format, 8))
                {
                    if (!strlen(g_percent_format))
                    {
                        copy_string(g_percent_format, "%3.02f％", 8); /* with a full-width percent sign */
                    }
                    g_show_percent = 1;
                }
            }
        }
    }
    g_defocus = CreateDefocus(Self());
    SetColor(g_defocus, 1.0, 1.0, 1.0, 0.0);
    SetOptionI(g_defocus, TAG("LEVL"), 3);
    SetOptionI(g_defocus, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionF(g_defocus, TAG("ZOFS"), -3.0);
    create_window(g_defocus, width, height);
    {
        new title[32];
        if (get_text(_, "title", title, -1, 32))
        {
            g_title = create_centered_label(g_defocus);
            SetPos(g_title, 0.0, 32.0 - 0.5 * height, 0.0, 0);
            SetOptionS(g_title, TAG("UTF8"), title);
            SetColor(g_title, 0.215, 0.708, 0.999, 0.0);
        }
    }
    {
        new value[128];
        if (get_setting(root, "mode", value, 128))
        {
            if (!strcmp(value, "center"))
            {
                if (get_text(_, "msg", value, -1, 128))
                {
                    new msg[128];
                    replace_all(msg, value, "\\n", "\n", 128);
                    add_line_labels(g_defocus, msg, value, 128);
                }
            }
            else
            {
                g_content = attach_content(g_defocus, value, width - 64.0, height - (g_title ? 128.0 : 112.0));
                g_content_is_script = 1;
            }
        }
        else
        {
            g_content = create_message_textbox(g_defocus, width - 64.0, height - 48.0);
            if (get_text(_, "msg", value, -1, 128))
            {
                new msg[128];
                replace_all(msg, value, "\\n", "\n", 128);
                SetOptionS(g_content, TAG("UTF8"), msg);
            }
            SetColor(g_content, 0.7843, 0.7843, 0.7843, 0.0);
        }
    }
    if (g_is_confirm)
    {
        new label[16];
        new tag;
        while (get_text(tag, "btn", label, g_button_count, 16))
        {
            g_buttons[g_button_count] = create_button_plane(g_defocus);
            SetTexture(g_buttons[g_button_count], imgGetResource(RES_BUTTON0), 0, -1);
            g_button_labels[g_button_count] = create_centered_label(g_defocus);
            SetOptionS(g_button_labels[g_button_count], TAG("UTF8"), label);
            SetColor(g_button_labels[g_button_count], 0.215, 0.708, 0.999, 0.0);
            g_button_count++;
        }
    }
    g_button_lit = create_button_plane(g_defocus);
    SetTexture(g_button_lit, imgGetResource(RES_BUTTON1), 0, -1);
    Hide(g_button_lit);
    {
        new Float:half_w;
        new Float:half_h;
        if (g_content_is_script)
        {
            half_w = 0.5 * floatround(width - 64.0, 0);
            half_h = 0.5 * floatround(height - (g_title ? 128.0 : 112.0), 0);
        }
        else if (g_content)
        {
            new rect[5];
            do
            {
                sleep;
                GetOptionP(g_content, TAG("RECT"), rect, 5);
            }
            while (!rect[0]);
            half_w = 0.5 * float(rect[3] >> 6);
            half_h = 0.5 * float(rect[4] >> 6);
        }
        else
        {
            half_w = 0.0;
            half_h = 0.5 * (20.0 * listSize());
        }
        if (g_content_is_script)
        {
            SetPos(g_content, 0.0, g_title ? -12.0 : -28.0, 0.0, 0);
        }
        else if (g_is_progress || g_button_count)
        {
            if (g_content)
            {
                SetPos(g_content, -half_w, float(15) - 2.0 - 24.0 - half_h, 0.0, 0); /* 15: the font size ("FSIZ") */
            }
            else
            {
                place_line_labels(-half_w, -8.0 - half_h);
            }
        }
        else if (g_content)
        {
            SetPos(g_content, -half_w, float(15) - 2.0 - half_h, 0.0, 0); /* 15: the font size ("FSIZ") */
        }
        else
        {
            place_line_labels(-half_w, -half_h);
        }
        if (g_is_progress)
        {
            create_progress_bar(g_defocus, 8.0 + half_h);
            if (g_show_percent)
            {
                g_percent_label = create_centered_label(g_defocus);
                SetPos(g_percent_label, 0.0, 28.0 + half_h, 0.0, 0);
                new text[8];
                sprintf(text, g_percent_format, 0);
                SetOptionS(g_percent_label, TAG("UTF8"), text);
                SetColor(g_percent_label, 0.7843, 0.7843, 0.7843, 0.0);
            }
        }
    }
    if (g_button_count)
    {
        new Float:button_w;
        new Float:button_h;
        for (new i; i < g_button_count; i++)
        {
            fit_buttons_to_label(i, button_w, button_h);
        }
        new Float:x = -0.5 * (24.0 + button_w) * float(g_button_count - 1);
        new Float:y = 0.5 * (height - button_h) - 32.0;
        for (new i; i < g_button_count; i++, x = x + (24.0 + button_w))
        {
            SetPos(g_buttons[i], x, y, 0.0, 0);
            SetPos(g_button_labels[i], x, y, 0.0, 0);
            set_button_size(g_buttons[i], button_w, button_h);
        }
        set_button_size(g_button_lit, button_w, button_h);
    }
    play_sound(se_open, SE_DIALOG_OPEN);
    fade_dialog(1.0, 300.0);
    AnimColor(g_defocus, bgcolor, bgcolor, bgcolor, 1.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(300);
    new saved_cancel[8];
    {
        new cancel_label[8];
        /* The setting "cancel" is the label of the cross in the pad help while the dialog is open. */
        if (get_text(_, "cancel", cancel_label, -1, 8))
        {
            GetSystemParamStr(SYS_STR_PADHELP_RD, saved_cancel, 8);
            SetSystemParamStr(SYS_STR_PADHELP_RD, cancel_label);
        }
    }
    select = clamp(select, 0, 1);
    new focus_zsort;
    new Float:focus_zoffset;
    if (g_button_count)
    {
        GetOptionI(focus, TAG("ZSRT"), focus_zsort);
        GetOptionF(focus, TAG("ZOFS"), focus_zoffset);
        SetOptionI(focus, TAG("ZSRT"), ZSORT_OVERLAY);
        SetOptionF(focus, TAG("ZOFS"), 3.0);
        send_focus(focus, g_buttons[select], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        padhelp = padhelp | PADHELP_CIRCLE;
    }
    else
    {
        send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    }
    new saved_padhelp = GetSystemParam(SYS_PADHELP);
    set_pad_help(padhelp);
    ready = 1;
    new content_active;
    for (; ; )
    {
        sleep;
        if (g_is_progress)
        {
            update_progress_bar(100.0);
        }
        if (canceled)
        {
            break;
        }
        new msg_id;
        new a;
        new b;
        /* "FOCS" (direction): the content script gives the focus back; with "DOWN" it goes onto the buttons. */
        if (RecvMessage(msg_id, a, b))
        {
            if (msg_id == TAG("FOCS"))
            {
                if (a == TAG("DOWN"))
                {
                    send_focus(focus, g_buttons[select], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    fade_dialog(1.0, 100.0);
                }
                content_active = 0;
            }
        }
        if (!content_active)
        {
            new pressed;
            new repeated;
            PadState(_, pressed, _, repeated, -1);
            if (g_button_count == 2)
            {
                if (pressed & PAD_RIGHT && !select)
                {
                    play_sound(se_cursor, SE_CURSOR);
                    select++;
                    send_focus(focus, g_buttons[select], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    fade_dialog(1.0, 100.0);
                }
                else if (pressed & PAD_LEFT && select)
                {
                    play_sound(se_cursor, SE_CURSOR);
                    select--;
                    send_focus(focus, g_buttons[select], 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
                    fade_dialog(1.0, 100.0);
                }
            }
            if (g_content_is_script)
            {
                if (repeated & PAD_DOWN)
                {
                    SendMessage(g_content, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                    content_active = 1;
                }
                else if (repeated & PAD_UP)
                {
                    SendMessage(g_content, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                    content_active = 1;
                }
            }
            if (pressed & PAD_CIRCLE && padhelp & PADHELP_CIRCLE && (!g_button_count || !is_animating2(focus)))
            {
                play_sound(se_ok, SE_DECIDE);
                if (g_button_count)
                {
                    new Float:x;
                    new Float:y;
                    GetPos(g_buttons[select], x, y, _, 0);
                    SetPos(g_button_lit, x, y, 0.0, 0);
                    Show(g_button_lit);
                    flash_button(1.0, 100.0);
                    flash_button(0.0, 100.0);
                    Hide(g_button_lit);
                }
                break;
            }
            else if (pressed & PAD_CROSS && padhelp & PADHELP_CROSS && (!g_button_count || !is_animating2(focus)))
            {
                play_sound(se_cancel, SE_CANCEL);
                canceled = 1;
                break;
            }
        }
        /* The glow on the window mirrors the position of the focus and follows its brightness. */
        if (focus)
        {
            new Float:x;
            new Float:y;
            GetPos(focus, x, y, _, g_window);
            SetPos(g_glow, -x, y, 0.0, 0);
            new Float:focus_alpha;
            GetColor(focus, _, _, _, focus_alpha);
            AnimColor(g_glow, 0.5 * focus_alpha, 0.5 * focus_alpha, 0.5 * focus_alpha, 1.0, 100.0, 0.0,
                    ANIM_FUNC_LINEAR);
        }
    }
    set_pad_help(saved_padhelp);
    if (g_button_count)
    {
        send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
        SetOptionI(focus, TAG("ZSRT"), focus_zsort);
        SetOptionF(focus, TAG("ZOFS"), focus_zoffset);
    }
    if (g_is_progress)
    {
        for (new i; i < ms_to_frames(100); i++)
        {
            sleep;
            update_progress_bar(100.0);
        }
    }
    play_sound(se_close, -1);
    fade_dialog(0.0, 300.0);
    if (g_is_progress)
    {
        AnimColor(g_bar_glow, 1.0, 1.0, 1.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
        AnimColor(g_bar_fill, 1.0, 1.0, 1.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    }
    AnimColor(g_defocus, 1.0, 1.0, 1.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
    wait_ms(300);
    if (strlen(saved_cancel))
    {
        SetSystemParamStr(SYS_STR_PADHELP_RD, saved_cancel);
    }
}

/* Fades the window and everything on it to alpha over duration ms; the selected button label is the brighter. */
fade_dialog(Float:alpha, Float:duration)
{
    AnimColor(g_window, red, green, blue, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    if (g_content_is_script)
    {
        SendMessage(g_content, TAG("FADE"), alpha, duration, 0, 0);
    }
    else if (g_content)
    {
        AnimColor(g_content, 0.7843, 0.7843, 0.7843, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
    else
    {
        for (new i; i < listSize(); i++)
        {
            AnimColor(listGetAt(i), 0.7843, 0.7843, 0.7843, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
        }
    }
    if (g_button_count)
    {
        for (new i; i < g_button_count; i++)
        {
            AnimColor(g_buttons[i], 0.0, 0.54, 0.78, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
        }
        if (select)
        {
            AnimColor(g_button_labels[0], 0.215, 0.708, 0.999, 0.5 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
            if (g_button_count == 2)
            {
                AnimColor(g_button_labels[1], 0.215, 0.708, 0.999, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
            }
        }
        else
        {
            AnimColor(g_button_labels[0], 0.215, 0.708, 0.999, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
            if (g_button_count == 2)
            {
                AnimColor(g_button_labels[1], 0.215, 0.708, 0.999, 0.5 * alpha, duration, 0.0, ANIM_FUNC_LINEAR);
            }
        }
    }
    if (g_is_progress)
    {
        AnimColor(g_bar_frame, 1.0, 1.0, 1.0, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
        if (g_show_percent)
        {
            AnimColor(g_percent_label, 0.7843, 0.7843, 0.7843, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
        }
    }
    if (g_title)
    {
        AnimColor(g_title, 0.7843, 0.7843, 0.7843, alpha, duration, 0.0, ANIM_FUNC_LINEAR);
    }
}

/* Sets the buttons the pad help shows (PADHELP_* bits). */
set_pad_help(buttons)
{
    SetSystemParam(SYS_PADHELP, buttons);
}
