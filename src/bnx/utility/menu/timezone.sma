/*
 * /opt0/bn/script/utility/menu/timezone.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * The list of the cities of the time zones (the items of menu sysconf_value_1_4) for the system settings: only
 * the visible rows have a label, made as the list scrolls; it tells its manager the id of the city pushed.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

#include "tag.inc"

#include <float>

#include "imgsync.inc"

#include "text.inc"

const CITY_COUNT = 128;

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

public manager; /* gets "FOCS" when the cursor leaves the list and "PUSH" (city id, button) */
public focus; /* the focus plane */
public padhelp; /* the pad help buttons shown with the circle */
public active; /* the list has the cursor */
public cursor; /* the city under the cursor; set to its id before the script starts */
public Float:meta_alpha; /* alpha of the whole list, set by "FADE" */
new g_visible; /* the number of rows shown at a time */
public scroll_pos; /* the first visible city */
new g_first; /* the city of the first label of the list */
new Float:g_interval = 18.0;
new g_hiding; /* faded out: the list hides itself g_hide_delay ms after g_hide_start */
new g_hide_start;
new g_hide_delay;
new g_menu;
new g_node; /* an item of the menu, the g_node_index-th, where the last lookup stopped */
new g_node_index;

main()
{
    g_visible = 10;
    {
        new value[5];
        if (GetArg("visible", value, 5))
        {
            g_visible = atoi(value);
        }
    }
    g_menu = domFindTag(domRoot(), "menu", "sysconf_value_1_4", 0);
    g_node = domFirstChild(g_menu, "");
    g_node_index = 0;
    {
        new id[4];
        new index;
        for (new node = g_node; node; node = domNext(node), index++)
        {
            domGetData(node, "id", id, 4);
            if (atoi(id) == cursor)
            {
                cursor = index;
                break;
            }
        }
    }
    scroll_pos = clamp(cursor - 4, 0, CITY_COUNT - g_visible);
    g_first = scroll_pos;
    for (new i; i < g_visible; i++)
    {
        new index = i + g_first;
        new name[64];
        get_city_name(index, name);
        new label = create_label(g_interval * i, name);
        listAddLast(label);
        sleep;
    }
    update_colors(100.0, 0.0);
    new pushed;
    for (; ; )
    {
        sleep;
        new msg;
        new a;
        new Float:b;
        /* "FOCS" (from): takes the cursor; "FADE" (alpha, time); "PUSH": the manager is done with the last push. */
        if (RecvMessage(msg, a, b))
        {
            if (msg == TAG("FOCS"))
            {
                enter_focus(a);
            }
            else if (msg == TAG("FADE"))
            {
                meta_alpha = a;
                update_colors(b, 0.0);
                if (0.0 == meta_alpha)
                {
                    g_hiding = 1;
                    g_hide_delay = floatround(b, 0);
                    g_hide_start = tickcount();
                }
                else
                {
                    g_hiding = 0;
                    Show(Self());
                }
            }
            else if (msg == TAG("PUSH"))
            {
                pushed = 0;
            }
        }
        if (active && !pushed)
        {
            cursor = clamp(cursor, 0, CITY_COUNT - 1);
            {
                new repeated;
                new pressed;
                PadState(_, pressed, _, repeated, -1);
                if (pressed & PAD_CIRCLE && !is_animating(focus) && ANIM_DONE == AnimGetState(listGetAt(0), ANIM_MOVE))
                {
                    SendMessage(manager, TAG("PUSH"), get_city_id(cursor), TAG("RRGT"), 0, 0);
                    pushed = 1;
                    continue;
                }
                else if (pressed & PAD_CROSS)
                {
                    SendMessage(manager, TAG("PUSH"), get_city_id(cursor), TAG("RDWN"), 0, 0);
                    pushed = 1;
                    continue;
                }
                else if (pressed & PAD_TRIANGLE)
                {
                    SendMessage(manager, TAG("PUSH"), get_city_id(cursor), TAG("RUP"), 0, 0);
                    pushed = 1;
                    continue;
                }
                else if (pressed & PAD_SQUARE)
                {
                    SendMessage(manager, TAG("PUSH"), get_city_id(cursor), TAG("RLFT"), 0, 0);
                    pushed = 1;
                    continue;
                }
                else if (pressed & PAD_SELECT)
                {
                    SendMessage(manager, TAG("PUSH"), get_city_id(cursor), TAG("SLCT"), 0, 0);
                    pushed = 1;
                    continue;
                }
                else if (pressed & PAD_START)
                {
                    SendMessage(manager, TAG("PUSH"), get_city_id(cursor), TAG("STRT"), 0, 0);
                    pushed = 1;
                    continue;
                }
                if (repeated & PAD_UP)
                {
                    if (cursor > 0)
                    {
                        cursor--;
                        sndSePlay(SE_CURSOR);
                        update_colors(100.0, 0.0);
                        scroll_to_cursor();
                        move_focus();
                    }
                    else
                    {
                        SendMessage(manager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                        leave_focus();
                        continue;
                    }
                }
                else if (repeated & PAD_DOWN)
                {
                    if (cursor < CITY_COUNT - 1)
                    {
                        cursor++;
                        sndSePlay(SE_CURSOR);
                        update_colors(100.0, 0.0);
                        scroll_to_cursor();
                        move_focus();
                    }
                    else
                    {
                        SendMessage(manager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                        leave_focus();
                        continue;
                    }
                }
                else if (repeated & PAD_LEFT)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
                    leave_focus();
                    continue;
                }
                else if (repeated & PAD_RIGHT)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                    leave_focus();
                    continue;
                }
            }
            drop_hidden_labels();
        }
        if (g_hiding)
        {
            new elapsed = tickcount() - g_hide_start;
            if (elapsed >= g_hide_delay)
            {
                Hide(Self());
                printf("list hiding...\n");
                g_hiding = 0;
            }
        }
    }
    return;
}

move_focus()
{
    send_focus(focus, listGetAt(cursor - g_first), 0, 0.5, 60.0, 60.0, 150.0, 0, 1);
}

/* Fades every label, transparent when scrolled out of view. */
update_colors(Float:duration, Float:delay)
{
    for (new i; i < listSize(); i++)
    {
        new index = i + g_first;
        {
            new Float:alpha = 1.0;
            if (active && cursor == index)
            {
                alpha = 1.0;
            }
            if (index < scroll_pos)
            {
                alpha = 0.0;
            }
            if (index >= scroll_pos + g_visible)
            {
                alpha = 0.0;
            }
            AnimColor(listGetAt(i), 0.215, 0.708, 0.999, alpha * meta_alpha, duration, delay, ANIM_FUNC_LINEAR);
        }
    }
}

/* Takes the cursor coming from the direction from ("FRST", UP, "DOWN", "LEFT", "RIGT"), or gives it up ("FOUT"). */
enter_focus(from)
{
    active = 1;
    if (from == TAG("FOUT"))
    {
        leave_focus();
        return;
    }
    if (from == TAG("LEFT"))
    {
        sndSePlay(SE_CURSOR);
    }
    else if (from == TAG("RIGT"))
    {
        sndSePlay(SE_CURSOR);
    }
    else if (from == TAG("UP"))
    {
        sndSePlay(SE_CURSOR);
        cursor = 0;
    }
    else if (from == TAG("DOWN"))
    {
        sndSePlay(SE_CURSOR);
        cursor = CITY_COUNT - 1;
    }
    else if (from == TAG("FRST"))
    {
        printf("got initial focus.\n");
    }
    move_focus();
    update_colors(100.0, 0.0);
    update_pad_help();
}

/* Leaves the focus and dims the colors. */
leave_focus()
{
    active = 0;
    update_colors(100.0, 40.0);
}

update_pad_help()
{
    SetSystemParam(SYS_PADHELP, padhelp | PADHELP_CIRCLE);
}

/* Creates the label of a city at y: the text in front of a black copy of it, 2 pixels off. */
create_label(Float:y, text[])
{
    new Float:tex_w;
    new Float:tex_h;
    new label;
    imgDecStringStart(text, 15, 64, 64, 0);
    if (IMG_DONE != wait_image_decode())
    {
        SetLocalScale(label, 100.0, 30.0, 1.0); /* NOTE: bug: label is not created yet, still 0 */
    }
    tex_w = float(imgTexWidth());
    tex_h = float(imgTexHeight());
    label = CreatePlane(Self());
    SetPos(label, -130.0 + tex_w / 2.0, y, 0.0, 0);
    SetTexture(label, imgTexID(), 0, -1);
    SetColor(label, 0.215, 0.708, 0.999, 0.0);
    SetOptionI(label, TAG("ZSRT"), ZSORT_OVERLAY);
    SetOptionI(label, TAG("NUM"), 2);
    SetLocalScale(label, tex_w, tex_h, 1.0);
    new Float:color[4] = {1.0, 1.0, 1.0, 1.0};
    SetItemOptionP(label, 1, TAG("COLR"), color, 4);
    SetItemOptionI(label, 1, TAG("ALPH"), BLEND_ADD);
    color[0] = 0.0;
    color[1] = 0.0;
    color[2] = 0.0;
    color[3] = 1.0;
    new Float:clip[4] = {2.0, 2.0, 1.0, 1.0};
    clip[0] /= tex_w;
    clip[1] /= tex_h;
    SetItemOptionP(label, 0, TAG("CLIP"), clip, 4);
    SetItemOptionP(label, 0, TAG("COLR"), color, 4);
    SetItemOptionI(label, 0, TAG("ALPH"), BLEND_ALPHA);
    return label;
}

new g_scroll_time;
new g_last_scroll_time;

/*
 * Scrolls the list by one row if the cursor has left the visible rows, adding the label that comes into view; the
 * faster the scrolls follow each other, the shorter the move.
 */
scroll_to_cursor()
{
    new scrolled;
    if (cursor < scroll_pos)
    {
        scroll_pos--;
        if (g_first - 1 >= 0)
        {
            new name[64];
            get_city_name(g_first - 1, name);
            new label = create_label(-1.0 * g_interval, name);
            listInsert(0, label);
            g_first--;
            sleep;
        }
        scrolled = 1;
    }
    else if (cursor >= scroll_pos + g_visible)
    {
        scroll_pos++;
        if (g_first + listSize() < CITY_COUNT)
        {
            new name[64];
            get_city_name(g_first + listSize(), name);
            new label = create_label(g_interval * g_visible, name);
            listAddLast(label);
            sleep;
        }
        scrolled = 1;
    }
    if (scrolled)
    {
        g_last_scroll_time = g_scroll_time;
        g_scroll_time = tickcount();
        new duration = g_scroll_time - g_last_scroll_time;
        duration = clamp(duration, 60, 300);
        update_colors(float(duration), 0.0);
        for (new i; i < listSize(); i++)
        {
            new index = i + g_first;
            new Float:y = g_interval * (index - scroll_pos);
            new Float:x;
            GetPos(listGetAt(i), x, _, _, 0);
            AnimMoveLine(listGetAt(i), x, y, 0.0, float(duration), 0.0, ANIM_FUNC_ANGULER);
        }
    }
}

/* Destroys the first or the last label once it has moved a row out of view. */
drop_hidden_labels()
{
    new Float:y;
    GetPos(listGetAt(0), _, y, _, 0);
    if (y < -1.0 * g_interval)
    {
        Destroy(listGetAt(0));
        listRemoveAt(0);
        g_first++;
    }
    GetPos(listGetLast(), _, y, _, 0);
    if (y > g_interval * g_visible)
    {
        Destroy(listGetLast());
        listRemoveLast();
    }
}

/* Gets the name of the index-th city, walking g_node there. */
get_city_name(index, name[])
{
    for (; ; )
    {
        if (index == g_node_index)
        {
            domGetData(g_node, "value", name, 64);
            return;
        }
        if (index > g_node_index)
        {
            g_node = domNext(g_node);
            g_node_index++;
        }
        else
        {
            g_node = domPrev(g_node);
            g_node_index--;
        }
    }
}

/* Returns the id of the index-th city, walking g_node there. */
get_city_id(index)
{
    new id[4];
    for (; ; )
    {
        if (index == g_node_index)
        {
            domGetData(g_node, "id", id, 4);
            break;
        }
        else if (index > g_node_index)
        {
            g_node = domNext(g_node);
            g_node_index++;
        }
        else
        {
            g_node = domPrev(g_node);
            g_node_index--;
        }
    }
    return atoi(id);
}
