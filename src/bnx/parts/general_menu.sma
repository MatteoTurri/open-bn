/*
 * /opt0/bn/script/parts/general_menu.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A scrolling list or grid of items laid out on the nodes of a Hermite path. Only the rows near the selection
 * have item objects: the menu asks its manager for each one ("ITEM") and drives them with messages.
 */

#include <core>
#include <bn>

#pragma dynamic 512

forward Float:scroll_to(target);
forward Float:get_node_x(slot, columns);
forward Float:get_node_y(slot, columns);
forward Float:get_node_z(slot, columns);
forward Float:get_item_alpha(slot, focused);

#include "tag.inc"

#include <float>

#include "scroll.inc"

#include "genmenu.inc"

/* Values of g_scroll_dir. */
const SCROLL_DIR_NONE = 0;
const SCROLL_DIR_FORWARD = 1;  /* items were removed at the start or added at the end */
const SCROLL_DIR_BACKWARD = 2; /* items were removed at the end or added at the start */

/* Sleeps until a message msg arrives. */
wait_message(msg, &a=0, &b=0, &c=0, &d=0)
{
    while (!FindMessage(msg, a, b, c, d))
    {
        sleep;
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

public manager; /* the object that creates the items and is told of focus moves and presses */
public focus; /* the focus object */
public scroll_top; /* the index of the first item of the page */
new g_page_size; /* slots on the page, columns times rows */
new g_margin_before; /* slots before and after the page, for items scrolling in and out */
new g_margin_after;
new g_list_count;
new g_top_index; /* the index of the first item object, negative for blanks before the list */
new g_columns = 1;
new g_slot_count;
new g_scroll_dir; /* the item objects changed since the last scroll: SCROLL_DIR_* */
new g_focused;
new g_moves; /* unused, but part of the layout */
new Float:g_focus_pos; /* position on the path of the focused item */
new g_active;
new Float:g_slot_step;
new g_path;
new Float:g_move_time = 600.0;
new Float:g_move_delay;
new g_scroll_mode = SCROLL_LINE;
new g_move_curve = ANIM_FUNC_EXPONENT;
new g_blanks_before; /* blank item objects before the start and after the end of the list */
new g_blanks_after;
new Float:g_column_dist = 40.0;
new Float:g_row_dist = 30.0;
new Float:g_alpha = 1.0;
new Float:g_fade_time = 500.0;
new g_last_row;
/* Positions of rows on the path, 0.0 to 1.0, where the alpha of get_item_alpha changes. */
new Float:g_first_inner_row;
new Float:g_last_inner_row;
new Float:g_fade_in_end;
new Float:g_fade_out_start;
new Float:g_off_alpha = 0.15;  /* alpha of the rows at the ends of the path */
new Float:g_idle_alpha = 0.75; /* alpha of the rows of the page other than the focused item */

/* Creates the path, one node per slot laid out by get_node_x and get_node_y, and the fade limits of the rows. */
init_layout(parent, item_count, page_size, margin_before, margin_after, columns, column_dist, row_dist)
{
    g_focused = 0;
    g_list_count = item_count;
    g_page_size = page_size;
    g_margin_before = margin_before;
    g_margin_after = margin_after;
    g_columns = columns;
    g_slot_count = g_margin_before + g_margin_after + g_page_size;
    g_column_dist = column_dist;
    g_row_dist = row_dist;
    if (g_slot_count > 0)
    {
        g_slot_step = 1.0 / float(g_slot_count - 1);
        g_focus_pos = float(g_focused - g_top_index) * g_slot_step;
    }
    else
    {
        g_focus_pos = 0.0;
    }
    g_scroll_dir = SCROLL_DIR_NONE;
    g_path = CreateHermite(parent);
    SetPos(g_path, 0.0, 0.0, 0.0, 0);
    InitNodeArray(g_path, g_slot_count, 1);
    for (new slot = -g_margin_before; slot < g_margin_after + g_page_size; slot++)
    {
        SetNodePos(g_path, get_node_x(slot, g_columns), get_node_y(slot, g_columns), get_node_z(slot, g_columns),
                slot + g_margin_before);
    }
    g_last_row = g_slot_count / g_columns - 1;
    g_first_inner_row = 1.0 / float(g_last_row);
    g_last_inner_row = float(g_last_row - 1) / float(g_last_row);
    g_fade_in_end = float(g_margin_before / g_columns) / float(g_last_row);
    g_fade_out_start = float((g_slot_count - g_margin_after) / g_columns - 1) / float(g_last_row);
}

set_off_alpha(alpha)
{
    g_off_alpha = alpha;
}

set_idle_alpha(alpha)
{
    g_idle_alpha = alpha;
}

set_move_time(duration)
{
    g_move_time = duration;
}

set_move_anim(duration, delay, curve)
{
    g_move_time = duration;
    g_move_delay = delay;
    g_move_curve = curve;
}

set_scroll_mode(mode)
{
    g_scroll_mode = mode;
}

set_node_pos(node, Float:x, Float:y, Float:z)
{
    SetNodePos(g_path, x, y, z, node);
}

/* Fades every item object to its alpha times alpha ("FADE"). */
fade_items(alpha, duration, focused)
{
    g_alpha = alpha;
    g_fade_time = duration;
    for (new i; i < listSize(); i++)
    {
        new item = listGetAt(i);
        if (item)
        {
            SendMessage(item, TAG("FADE"), get_item_alpha(i, focused) * g_alpha, g_fade_time, 0, 0);
        }
    }
}

/* Tells every item object the focused index ("FOCS") with the menu active. */
focus_items(focused)
{
    for (new i; i < listSize(); i++)
    {
        new item = listGetAt(i);
        if (item)
        {
            SendMessage(item, TAG("FOCS"), focused, get_item_alpha(i, focused) * g_alpha, g_move_time, 1);
        }
    }
}

/* Moves the focus object off the menu and tells every item object that nothing is focused. */
unfocus_items(focused)
{
    send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
    for (new i; i < listSize(); i++)
    {
        new item = listGetAt(i);
        if (item)
        {
            SendMessage(item, TAG("FOCS"), -2, get_item_alpha(i, focused) * g_alpha, g_move_time, 0);
        }
        i = i + focused * 0; /* redundant as written */
    }
}

/* Returns the public pad_enable of the item object of index, or 0. */
get_item_pad_enable(index)
{
    new pad_bits;
    new slot = index - g_top_index;
    if (slot >= 0 && slot < listSize())
    {
        new item = listGetAt(slot);
        if (item)
        {
            pad_bits = GetPubVar(item, "pad_enable");
        }
    }
    return pad_bits;
}

/*
 * Moves the focus to the index target: creates and destroys item objects, one change per pass, until they cover
 * the rows around it, then tells them their new places. Returns the progress of the scroll, 0.0 to 1.0.
 */
Float:scroll_to(target)
{
    if (target < 0 || !g_path)
    {
        return -1.0;
    }
    new row_start = g_columns * (target / g_columns);
    for (; ; )
    {
        if (0 == g_list_count)
        {
            break;
        }
        if (0 == listSize())
        {
            printf("----------------- 1 -----------------\n");
            g_focused = -1;
            g_top_index = row_start + g_columns - (g_slot_count - g_margin_after);
            if (g_top_index < -g_margin_before)
            {
                g_top_index = 0 - g_margin_before;
            }
            printf("visible %d \n", g_top_index);
        }
        if (g_top_index + (g_margin_before + g_page_size) < row_start + g_columns)
        {
            printf("----- 2 ----- %d, %d\n", g_top_index, row_start);
            new index = g_top_index;
            new item = listGetAt(0);
            if (g_focused == index)
            {
                send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            }
            Destroy(item);
            listRemoveAt(0);
            g_scroll_dir = SCROLL_DIR_FORWARD;
            if (g_top_index < 0)
            {
                g_blanks_before--;
            }
            printf("remove item - %d\n", index);
            g_top_index++;
            continue;
        }
        if (g_top_index + listSize() > row_start + g_margin_after + g_page_size)
        {
            printf("---------- 3 ---------- %d\n", g_top_index);
            new index = g_top_index + listSize() - 1;
            new item = listGetAt(listSize() - 1);
            if (g_focused == index)
            {
                send_focus(focus, 0, 0, 0.5, 60.0, 60.0, 150.0, 1, 1);
            }
            Destroy(item);
            listRemoveAt(listSize() - 1);
            g_scroll_dir = SCROLL_DIR_BACKWARD;
            if (index >= g_list_count)
            {
                g_blanks_after--;
            }
            printf("remove item - %d\n", index);
            continue;
        }
        if (g_top_index + g_margin_before > row_start)
        {
            printf("---------- 4 --------- %d\n", g_top_index);
            new index = g_top_index - 1;
            new item;
            SendMessage(manager, TAG("ITEM"), Self(), g_path, index, 0);
            wait_message(TAG("ITEM"), item);
            if (index < 0)
            {
                g_blanks_before++;
                index = -1;
            }
            if (index >= g_list_count)
            {
                g_blanks_after++;
                index = -1;
            }
            new Float:pos;
            if (SCROLL_ALONG_CURVE == g_scroll_mode)
            {
                pos = 0.0;
            }
            else
            {
                pos = float(index - (row_start - g_margin_before)) / float(g_slot_count - 1);
            }
            SendMessage(item, TAG("INIT"), index, pos, g_slot_count, focus);
            SendMessage(item, TAG("MODE"), g_scroll_mode, 0, 0, 0);
            listInsert(0, item);
            g_scroll_dir = SCROLL_DIR_BACKWARD;
            printf("add item - %d\n", index);
            g_top_index--;
            continue;
        }
        if (g_top_index + listSize() < row_start + g_margin_after || listSize() < g_slot_count)
        {
            printf("---------- 5 ---------- %d\n", g_top_index);
            new index = g_top_index + listSize();
            new item;
            SendMessage(manager, TAG("ITEM"), Self(), g_path, index, 0);
            wait_message(TAG("ITEM"), item);
            if (index < 0)
            {
                g_blanks_before++;
                index = -1;
            }
            if (index >= g_list_count)
            {
                g_blanks_after++;
                index = -1;
            }
            new Float:pos;
            if (SCROLL_ALONG_CURVE == g_scroll_mode)
            {
                pos = 1.0;
            }
            else
            {
                pos = float(index - g_top_index) / float(g_slot_count - 1);
            }
            SendMessage(item, TAG("INIT"), index, pos, g_slot_count, focus);
            SendMessage(item, TAG("MODE"), g_scroll_mode, 0, 0, 0);
            listAddLast(item);
            g_scroll_dir = SCROLL_DIR_FORWARD;
            printf("add item - %d\n", index);
            continue;
        }
        break;
    }
    g_focus_pos = float(target - g_top_index) * g_slot_step;
    if (g_focused != target)
    {
        g_moves = g_moves + (target - g_focused);
    }
    if (g_scroll_dir == SCROLL_DIR_FORWARD)
    {
        for (new i; i < listSize(); i++)
        {
            new item = listGetAt(i);
            new delay_steps = i - g_blanks_before < 0 ? 0 : i - g_blanks_before;
            if (Float:AnimGetCurrTime(item, ANIM_MOVE) <= 0.8)
            {
                delay_steps = 0;
            }
            SendMessage(item, TAG("TIME"), g_move_time, g_move_delay * float(delay_steps), g_move_curve, 0);
            SendMessage(item, TAG("SCRL"), i * g_slot_step, g_focus_pos, g_scroll_mode, 0);
        }
        AnimTimerManager(g_path, g_move_time, 0.0, ANIM_FUNC_EXPONENT);
        AnimDoTimer(g_path, ANIM_TIMER, 1.0);
    }
    else if (g_scroll_dir == SCROLL_DIR_BACKWARD)
    {
        for (new i = listSize() - 1; i >= 0; i--)
        {
            new item = listGetAt(i);
            new delay_steps = listSize() - 1 - i - g_blanks_after < 0 ? 0 : listSize() - 1 - i - g_blanks_after;
            if (Float:AnimGetCurrTime(item, ANIM_MOVE) <= 0.8)
            {
                delay_steps = 0;
            }
            SendMessage(item, TAG("TIME"), g_move_time, g_move_delay * float(delay_steps), g_move_curve, 0);
            SendMessage(item, TAG("SCRL"), i * g_slot_step, g_focus_pos, g_scroll_mode, 0);
        }
        AnimTimerManager(g_path, g_move_time, 0.0, ANIM_FUNC_EXPONENT);
        AnimDoTimer(g_path, ANIM_TIMER, 1.0);
    }
    if (SCROLL_DIR_NONE == g_scroll_dir)
    {
        if (g_focused != target)
        {
            new slot = target - g_top_index;
            SendMessage(listGetAt(slot), TAG("FOCS"), target, get_item_alpha(slot, target) * g_alpha, g_move_time,
                    g_active);
            slot = g_focused - g_top_index;
            SendMessage(listGetAt(slot), TAG("FOCS"), target, get_item_alpha(slot, target) * g_alpha, g_move_time,
                    g_active);
        }
    }
    else
    {
        for (new i; i < listSize(); i++)
        {
            SendMessage(listGetAt(i), TAG("FOCS"), target, get_item_alpha(i, target) * g_alpha, g_move_time, g_active);
        }
        g_scroll_dir = SCROLL_DIR_NONE;
        g_moves = 0;
    }
    g_focused = target;
    scroll_top = g_top_index + g_margin_before;
    new Float:progress = AnimGetCurrTime(g_path, ANIM_TIMER);
    return progress;
}

public select; /* the selected index */
public idle; /* 1 once the scroll has settled */
public padhelp; /* the pad help buttons of pad_enable (PADHELP_* bits) */
public pad_enable; /* the pad buttons every item takes, besides the pad_enable of the item */
public se_focusin = SE_CURSOR; /* sound effect when the focus arrives */
new g_repeat_frames = 12;
new g_exit_reason; /* how the focus last left: a direction or "PUSH" */
new g_enter_reason;
new g_next_select;
new g_repeating; /* a held direction is repeating */
new g_repeat_wait; /* frames until a held direction repeats; one byte each for left, right, down and up */
new g_item_count;
new Float:g_first_move_time = 100.0;
new Float:g_repeat_move_time = 100.0;

/* The nodes of the path form a grid, column_dist apart across and row_dist apart down. */
Float:get_node_x(slot, columns)
{
    return float(slot % columns) * g_column_dist;
}

Float:get_node_y(slot, columns)
{
    return -float(slot / columns) * g_row_dist;
}

Float:get_node_z(slot, columns)
{
    return 0.0 * float(slot * columns);
}

/*
 * Returns the alpha of the item object in slot: 1.0 for the focused one, g_idle_alpha on the page, fading toward
 * g_off_alpha in the margins, and 0.0 out of the path.
 */
Float:get_item_alpha(slot, focused)
{
    if (g_slot_count <= 0 || slot >= g_slot_count || slot < 0)
    {
        return 0.0;
    }
    if (slot == focused - g_top_index)
    {
        return 1.0;
    }
    new Float:row_pos = float(slot / g_columns) / float(g_last_row);
    if (row_pos < g_first_inner_row || row_pos > g_last_inner_row)
    {
        new Float:alpha = g_off_alpha;
        return alpha < 0.0 ? 0.0 : alpha;
    }
    if (row_pos < g_fade_in_end)
    {
        new Float:alpha = (1.0 - g_off_alpha) / g_fade_in_end * row_pos + g_off_alpha;
        return alpha < 0.0 ? 0.0 : alpha * g_idle_alpha;
    }
    if (row_pos > g_fade_out_start)
    {
        new Float:alpha = (1.0 - g_off_alpha) / (1.0 - g_fade_out_start) * (1.0 - row_pos) + g_off_alpha;
        return alpha < 0.0 ? 0.0 : alpha * g_idle_alpha;
    }
    return 1.0 * g_idle_alpha;
}

/*
 * Set up by messages from the manager: "INUM" (item count), "CLRW" (columns, rows), "MGIN" (slots before, after),
 * and optionally "PADL" (steps of up, down, left, right), "DIST" (column distance, row distance), "MODE" (menu
 * type, scroll mode, exit mask), "ANIM" (move time, delay, curve), "OFFA" (alpha), "IDLA" (alpha), "REPT" (repeat
 * time in ms, move time), "SLCT" (index), "NPOS" (node, x, y, z) and "FOCS".
 */
public general_menu()
{
    new self = Self();
    new menu_type;
    new exit_mask;
    new columns = 1;
    new step_up;
    new step_down;
    new step_left;
    new step_right;
    {
        new a;
        new b;
        new c;
        new d;
        wait_message(TAG("INUM"), g_item_count);
        wait_message(TAG("CLRW"), a, b);
        columns = a;
        new rows = b;
        new page_size = columns * rows;
        new Float:margin_before = columns;
        new margin_after = columns;
        wait_message(TAG("MGIN"), a, b);
        new remainder = a % columns;
        if (remainder)
        {
            margin_before = a + columns - remainder;
        }
        else
        {
            margin_before = a;
        }
        remainder = b % columns;
        if (remainder)
        {
            margin_after = b + columns - remainder;
        }
        else
        {
            margin_after = b;
        }
        if (FindMessage(TAG("PADL"), a, b, c, d))
        {
            step_up = a;
            step_down = b;
            step_left = c;
            step_right = d;
        }
        else
        {
            step_up = -columns;
            step_down = columns;
            step_left = -1;
            step_right = 1;
        }
        new Float:column_dist = 40.0;
        new Float:row_dist = 30.0;
        if (FindMessage(TAG("DIST"), a, b))
        {
            column_dist = a;
            row_dist = b;
        }
        if (FindMessage(TAG("MODE"), a, b, c))
        {
            menu_type = a;
            set_scroll_mode(b);
            if (MENU_TYPE_PAGE == menu_type)
            {
                exit_mask = c;
            }
            else
            {
                exit_mask = 0;
            }
        }
        if (FindMessage(TAG("ANIM"), a, b, c))
        {
            g_first_move_time = a;
            g_repeat_move_time = a;
            set_move_anim(a, b, c);
        }
        if (FindMessage(TAG("OFFA"), a))
        {
            set_off_alpha(a);
        }
        if (FindMessage(TAG("IDLA"), a))
        {
            set_idle_alpha(a);
        }
        if (FindMessage(TAG("REPT"), a, b))
        {
            g_repeat_frames = floatround(0.001 * (float(FrameRate()) * Float:a), 0);
            if (b)
            {
                g_repeat_move_time = b;
            }
        }
        if (FindMessage(TAG("SLCT"), a))
        {
            if (a >= 0 && a < g_item_count)
            {
                select = a;
                g_next_select = select;
            }
        }
        init_layout(self, g_item_count, page_size, margin_before, margin_after, columns, column_dist, row_dist);
        while (FindMessage(TAG("NPOS"), a, b, c, d))
        {
            set_node_pos(a, b, c, d);
        }
        sleep;
        if (FindMessage(TAG("FOCS"), a))
        {
            receive_focus(a);
        }
        new Float:progress = scroll_to(select);
        if (progress > 0.9)
        {
            idle = 1;
        }
        else
        {
            idle = 0;
        }
        sleep;
    }
    for (; ; )
    {
        new msg;
        new a;
        new b;
        new c;
        new d;
        while (FindMessage(TAG("FOCS"), a))
        {
            receive_focus(a);
        }
        /*
         * "THRU" (index or -1 for all, message, a, b): passes a message to item objects; "SLCT" (index);
         * "NPOS" (node, x, y, z); "ANIM" (move time, delay, curve); "FADE" (alpha, duration).
         */
        while (RecvMessage(msg, a, b, c, d))
        {
            if (msg == TAG("THRU"))
            {
                if (a >= 0)
                {
                    if (a - g_top_index >= 0 && a - g_top_index < listSize())
                    {
                        new item = listGetAt(a - g_top_index);
                        SendMessage(item, b, c, d, 0, 0);
                    }
                }
                else
                {
                    for (new i; i < listSize(); i++)
                    {
                        new item = listGetAt(i);
                        SendMessage(item, b, c, d, 0, 0);
                    }
                }
            }
            else if (msg == TAG("SLCT"))
            {
                if (a >= 0 && a < g_item_count)
                {
                    g_next_select = a;
                }
            }
            else if (msg == TAG("NPOS"))
            {
                set_node_pos(a, b, c, d);
            }
            else if (msg == TAG("ANIM"))
            {
                set_move_anim(a, b, c);
            }
            else if (msg == TAG("FADE"))
            {
                fade_items(a, b, select);
            }
            else
            {
                printf("WARNING : listcontrol may lost a message!!\n");
            }
        }
        /*
         * A held direction moves the selection, first after g_first_move_time, then every g_repeat_frames; past
         * an exit the manager is asked to move the focus ("FOCS"). A face button the item takes is a press
         * ("PUSH" index, button, item), and the menu waits for the manager to echo it.
         */
        if (g_active)
        {
            new held;
            new pressed;
            new released;
            new repeated;
            PadState(held, pressed, released, repeated, -1);
            if (g_repeat_wait <= 0 && held & PAD_LEFT)
            {
                if (!g_repeating || pressed & PAD_LEFT)
                {
                    set_move_time(g_first_move_time);
                    g_repeat_wait = floatround(0.001 * (float(FrameRate()) * g_first_move_time), 0) << 0;
                    g_repeating = 1;
                }
                else
                {
                    set_move_time(g_repeat_move_time);
                    g_repeat_wait = g_repeat_frames << 0;
                }
                if (exit_mask & EXIT_LEFT && (select / columns != (select + step_left) / columns
                        || select + step_left < 0 || select + step_left >= g_item_count || 0 == step_left))
                {
                    SendMessage(manager, TAG("FOCS"), TAG("LEFT"), 0, 0, 0);
                    g_exit_reason = TAG("LEFT");
                    unfocus_items(select);
                    g_active = 0;
                }
                else if (select + step_left >= 0 && select + step_left < g_item_count)
                {
                    g_next_select = select + step_left;
                }
                goto count_repeat;
            }
            if (g_repeat_wait <= 0 && held & PAD_RIGHT)
            {
                if (!g_repeating || pressed & PAD_RIGHT)
                {
                    set_move_time(g_first_move_time);
                    g_repeat_wait = floatround(0.001 * (float(FrameRate()) * g_first_move_time), 0) << 8;
                    g_repeating = 1;
                }
                else
                {
                    set_move_time(g_repeat_move_time);
                    g_repeat_wait = g_repeat_frames << 8;
                }
                if (exit_mask & EXIT_RIGHT && (select / columns != (select + step_right) / columns
                        || select + step_right < 0 || select + step_right >= g_item_count || 0 == step_right))
                {
                    SendMessage(manager, TAG("FOCS"), TAG("RIGT"), 0, 0, 0);
                    g_exit_reason = TAG("RIGT");
                    unfocus_items(select);
                    g_active = 0;
                }
                else if (select + step_right >= 0 && select + step_right < g_item_count)
                {
                    g_next_select = select + step_right;
                }
                goto count_repeat;
            }
            if (g_repeat_wait <= 0 && held & PAD_DOWN)
            {
                if (!g_repeating || pressed & PAD_DOWN)
                {
                    set_move_time(g_first_move_time);
                    g_repeat_wait = floatround(0.001 * (float(FrameRate()) * g_first_move_time), 0) << 16;
                    g_repeating = 1;
                }
                else
                {
                    set_move_time(g_repeat_move_time);
                    g_repeat_wait = g_repeat_frames << 16;
                }
                if (select + step_down >= 0 && select + step_down < g_item_count)
                {
                    g_next_select = select + step_down;
                }
                else if (exit_mask & EXIT_DOWN)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("DOWN"), 0, 0, 0);
                    g_exit_reason = TAG("DOWN");
                    unfocus_items(select);
                    g_active = 0;
                }
                else if (select + step_down < 0)
                {
                    g_next_select = 0;
                }
                else
                {
                    g_next_select = g_item_count - 1;
                }
                goto count_repeat;
            }
            if (g_repeat_wait <= 0 && held & PAD_UP)
            {
                if (!g_repeating || pressed & PAD_UP)
                {
                    set_move_time(g_first_move_time);
                    g_repeat_wait = floatround(0.001 * (float(FrameRate()) * g_first_move_time), 0) << 24;
                    g_repeating = 1;
                }
                else
                {
                    set_move_time(g_repeat_move_time);
                    g_repeat_wait = g_repeat_frames << 24;
                }
                if (select + step_up >= 0 && select + step_up < g_item_count)
                {
                    g_next_select = select + step_up;
                }
                else if (exit_mask & EXIT_UP)
                {
                    SendMessage(manager, TAG("FOCS"), TAG("UP"), 0, 0, 0);
                    g_exit_reason = TAG("UP");
                    unfocus_items(select);
                    g_active = 0;
                }
                else if (select + step_up < 0)
                {
                    g_next_select = 0;
                }
                else
                {
                    g_next_select = g_item_count - 1;
                }
                goto count_repeat;
            }
            new item;
            if (g_next_select - g_top_index < listSize() && g_next_select - g_top_index >= 0)
            {
                item = listGetAt(g_next_select - g_top_index);
            }
            if (!item)
            {
                goto count_repeat;
            }
            if (menu_type == MENU_TYPE_POPUP)
            {
                if (pressed & PAD_CIRCLE && (get_item_pad_enable(g_next_select) | pad_enable) & PAD_CIRCLE)
                {
                    sndSePlay(SE_DECIDE);
                    SendMessage(manager, TAG("PUSH"), g_next_select, TAG("RRGT"), item, 0);
                    g_exit_reason = TAG("PUSH");
                    g_active = 0;
                    wait_message(TAG("PUSH"));
                }
                else if (pressed & PAD_CROSS && (get_item_pad_enable(g_next_select) | pad_enable) & PAD_CROSS)
                {
                    sndSePlay(SE_CANCEL);
                    SendMessage(manager, TAG("PUSH"), g_next_select, TAG("RDWN"), item, 0);
                    g_exit_reason = TAG("PUSH");
                    g_active = 0;
                    wait_message(TAG("PUSH"));
                }
                goto count_repeat;
            }
            if (pressed & PAD_CIRCLE && (get_item_pad_enable(g_next_select) | pad_enable) & PAD_CIRCLE)
            {
                SendMessage(manager, TAG("PUSH"), g_next_select, TAG("RRGT"), item, 0);
                g_exit_reason = TAG("PUSH");
                wait_message(TAG("PUSH"));
                goto count_repeat;
            }
            if (pressed & PAD_CROSS && (get_item_pad_enable(g_next_select) | pad_enable) & PAD_CROSS)
            {
                SendMessage(manager, TAG("PUSH"), g_next_select, TAG("RDWN"), item, 0);
                g_exit_reason = TAG("PUSH");
                wait_message(TAG("PUSH"));
                goto count_repeat;
            }
            if (pressed & PAD_TRIANGLE && (get_item_pad_enable(g_next_select) | pad_enable) & PAD_TRIANGLE)
            {
                SendMessage(manager, TAG("PUSH"), g_next_select, TAG("RUP"), item, 0);
                g_exit_reason = TAG("PUSH");
                wait_message(TAG("PUSH"));
                goto count_repeat;
            }
            if (pressed & PAD_SQUARE && (get_item_pad_enable(g_next_select) | pad_enable) & PAD_SQUARE)
            {
                SendMessage(manager, TAG("PUSH"), g_next_select, TAG("RLFT"), item, 0);
                g_exit_reason = TAG("PUSH");
                wait_message(TAG("PUSH"));
                goto count_repeat;
            }
            /* NOTE: bug: the select button is enabled by the bit of the start button */
            if (pressed & PAD_SELECT && (get_item_pad_enable(g_next_select) | pad_enable) & PAD_START)
            {
                SendMessage(manager, TAG("PUSH"), g_next_select, TAG("SLCT"), item, 0);
                g_exit_reason = TAG("PUSH");
                wait_message(TAG("PUSH"));
                goto count_repeat;
            }
            if (pressed & PAD_START && (get_item_pad_enable(g_next_select) | pad_enable) & PAD_START)
            {
                SendMessage(manager, TAG("PUSH"), g_next_select, TAG("STRT"), item, 0);
                g_exit_reason = TAG("PUSH");
                wait_message(TAG("PUSH"));
            }
            count_repeat:
            if (g_repeat_wait & 0xFF)
            {
                if (~held & PAD_LEFT)
                {
                    g_repeating = 0;
                }
                g_repeat_wait = g_repeat_wait - 1;
            }
            else if (g_repeat_wait & 0xFF00)
            {
                if (~held & PAD_RIGHT)
                {
                    g_repeating = 0;
                }
                g_repeat_wait = g_repeat_wait - 0x100;
            }
            else if (g_repeat_wait & 0xFF0000)
            {
                if (~held & PAD_DOWN)
                {
                    g_repeating = 0;
                }
                g_repeat_wait = g_repeat_wait - 0x10000;
            }
            else if (g_repeat_wait & 0xFF000000)
            {
                if (~held & PAD_UP)
                {
                    g_repeating = 0;
                }
                g_repeat_wait = g_repeat_wait - 0x1000000;
            }
        }
        new Float:progress = scroll_to(g_next_select);
        if (progress > 0.9)
        {
            idle = 1;
        }
        else
        {
            idle = 0;
        }
        if (select != g_next_select)
        {
            sndSePlay(SE_CURSOR);
            select = g_next_select;
        }
        sleep;
    }
    return;
}

/* Handles "FOCS": takes the focus, or gives it up if direction is "FOUT"; "RTRN" after a sideways exit steps on. */
receive_focus(direction)
{
    if (direction == TAG("FOUT"))
    {
        g_active = 0;
        unfocus_items(select);
    }
    else if (!g_active)
    {
        g_enter_reason = direction;
        if (g_enter_reason != TAG("FRST") && g_enter_reason != TAG("RTRN"))
        {
            sndSePlay(se_focusin);
        }
        if (g_enter_reason == TAG("FRST"))
        {
        }
        else if (g_exit_reason == TAG("LEFT") && g_enter_reason == TAG("RTRN"))
        {
            if (select > 0)
            {
                g_next_select = select - 1;
            }
        }
        else if (g_exit_reason == TAG("RIGT") && g_enter_reason == TAG("RTRN"))
        {
            if (select + 1 < g_item_count)
            {
                g_next_select = select + 1;
            }
        }
        else if (g_exit_reason == TAG("PUSH") && g_enter_reason == TAG("RTRN"))
        {
        }
        g_active = 1;
        focus_items(select);
        show_pad_help(select);
        g_repeat_wait = g_repeat_frames;
    }
}

/* Shows the pad help for the face buttons the item of index or the menu takes. */
show_pad_help(index)
{
    new item_pad_enable = get_item_pad_enable(index);
    if ((pad_enable | item_pad_enable) & PAD_CIRCLE)
    {
        padhelp = PADHELP_CIRCLE;
    }
    if ((pad_enable | item_pad_enable) & PAD_SQUARE)
    {
        padhelp = padhelp | PADHELP_SQUARE;
    }
    if ((pad_enable | item_pad_enable) & PAD_TRIANGLE)
    {
        padhelp = padhelp | PADHELP_TRIANGLE;
    }
    if ((pad_enable | item_pad_enable) & PAD_CROSS)
    {
        padhelp = padhelp | PADHELP_CROSS;
    }
    SetSystemParam(SYS_PADHELP, padhelp);
}
