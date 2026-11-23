/*
 * /opt0/bn/script/parts/context_menu.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * A menu of the "TEXT" tags of an XML group (argument "xml_group") on a turning drum: an item object runs
 * "item_main" of the argument "item_script" for the rows near the selection only. Up and down select, the circle
 * chooses and the cross cancels; the choice is left in select.
 */

#include <core>
#include <bn>

#pragma dynamic 1280

forward Float:get_item_x(Float:offset);
forward Float:get_item_y(Float:offset);
forward Float:get_item_z(Float:offset);
forward Float:get_item_alpha(Float:offset);

#include "wait.inc"

#include <float>

#include "text.inc"

public select; /* the selected row; -1 when cancelled */
new g_rows_above = 4; /* rows kept above and below the selected one */
new g_rows_below = 4;

/* Frames after a move during which a repeat in the other direction is ignored. */
get_repeat_pause()
{
    return FrameRate() / 6;
}

public context_menu_main()
{
    new self = Self();
    new Float:scroll;
    new top_index;
    new up_repeat_wait;
    new down_repeat_wait;
    new group[256];
    GetArg("xml_group", group, 256);
    new count = count_group_tags("TEXT", group);
    for (; ; )
    {
        new moved;
        {
            new held;
            new pressed;
            new released;
            new repeated;
            PadState(held, pressed, released, repeated, -1);
            if (select + 1 < count && ((down_repeat_wait <= 0 && repeated & PAD_DOWN) || pressed & PAD_DOWN))
            {
                select++;
                moved = 1;
                up_repeat_wait = get_repeat_pause();
            }
            if (select - 1 >= 0 && ((up_repeat_wait <= 0 && repeated & PAD_UP) || pressed & PAD_UP))
            {
                select--;
                moved = 1;
                down_repeat_wait = get_repeat_pause();
            }
            if (pressed & PAD_CIRCLE)
            {
                for (new i; i < listSize(); i++)
                {
                    AnimColor(listGetAt(i), 1.0, 1.0, 1.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
                }
                AnimColor(listGetAt(select - top_index), 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(200);
                AnimColor(listGetAt(select - top_index), 1.0, 1.0, 1.0, 0.5, 200.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(200);
                AnimColor(listGetAt(select - top_index), 1.0, 1.0, 1.0, 1.0, 200.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(200);
                AnimColor(listGetAt(select - top_index), 1.0, 1.0, 1.0, 0.0, 400.0, 0.0, ANIM_FUNC_LINEAR);
                wait_ms(400);
                return;
            }
            if (pressed & PAD_CROSS)
            {
                for (new i; i < listSize(); i++)
                {
                    AnimColor(listGetAt(i), 1.0, 1.0, 1.0, 0.0, 300.0, 0.0, ANIM_FUNC_LINEAR);
                }
                wait_ms(300);
                select = -1;
                return;
            }
            new Float:target = 1.0 * select;
            scroll = scroll + (target - scroll) / 10;
            if (moved)
            {
                printf("select = %d\n", select);
                printf("visible_top_index = %d\n", top_index);
                printf("listSize() = %d\n", listSize());
            }
        }
        /* Creates and destroys item objects, one change per pass, until they cover the rows near select. */
        for (; ; )
        {
            if (0 == count)
            {
                break;
            }
            if (0 == listSize())
            {
                printf("----------------- 1 -----------------\n");
                new index;
                top_index = 0;
                select = 0;
                new item = CreateNull(self);
                SetPos(item, get_item_x(0.0), get_item_y(0.0), get_item_z(0.0), 0);
                SetColor(item, 1.0, 1.0, 1.0, 0.0);
                {
                    new script[256];
                    GetArg("item_script", script, 256);
                    new args[256];
                    /* NOTE: bug: the script name as the group */
                    sprintf(args, "xml_group = %s ; index = %d", script, index);
                    AttachScript(item, "item_main", script, args);
                }
                listAddLast(item);
                printf("add item - %d\n", 0);
                continue;
            }
            if (top_index < select - g_rows_above)
            {
                printf("----------------- 2 -----------------\n");
                new index = top_index;
                new item = listGetAt(0);
                Destroy(item);
                listRemoveAt(0);
                top_index++;
                printf("remove item - %d\n", index);
                continue;
            }
            if (top_index + listSize() - 1 > select + g_rows_below - 1)
            {
                printf("----------------- 3 -----------------\n");
                new index = top_index + listSize() - 1;
                new item = listGetAt(listSize() - 1);
                Destroy(item);
                listRemoveAt(listSize() - 1);
                printf("remove item - %d\n", index);
                continue;
            }
            if (top_index - 1 >= 0 && top_index - 1 >= select - g_rows_above)
            {
                printf("----------------- 4 -----------------\n");
                new index = top_index - 1;
                new item = CreateNull(self);
                new Float:offset = 1.0 * (index - select);
                SetPos(item, get_item_x(offset), get_item_y(offset), get_item_z(offset), 0);
                SetColor(item, 1.0, 1.0, 1.0, 0.0);
                {
                    new script[256];
                    GetArg("item_script", script, 256);
                    new args[256];
                    /* NOTE: bug: the script name as the group */
                    sprintf(args, "xml_group = %s ; index = %d", script, index);
                    AttachScript(item, "item_main", script, args);
                }
                listInsert(0, item);
                top_index--;
                printf("add item - %d\n", index);
                continue;
            }
            if (top_index + listSize() < count && top_index + listSize() < select + g_rows_below)
            {
                printf("----------------- 5 -----------------\n");
                new index = top_index + listSize();
                new item = CreateNull(self);
                new Float:offset = 1.0 * (index - select);
                SetPos(item, get_item_x(offset), get_item_y(offset), get_item_z(offset), 0);
                SetColor(item, 1.0, 1.0, 1.0, 0.0);
                {
                    new script[256];
                    GetArg("item_script", script, 256);
                    new args[256];
                    /* NOTE: bug: the script name as the group */
                    sprintf(args, "xml_group = %s ; index = %d", script, index);
                    AttachScript(item, "item_main", script, args);
                }
                listAddLast(item);
                printf("add item - %d\n", index);
                continue;
            }
            break;
        }
        for (new i; i < listSize(); i++)
        {
            new item = listGetAt(i);
            new Float:offset = i + top_index - scroll;
            SetPos(item, get_item_x(offset), get_item_y(offset), get_item_z(offset), 0);
            SetColor(item, 1.0, 1.0, 1.0, get_item_alpha(offset));
        }
        if (up_repeat_wait > 0)
        {
            up_repeat_wait--;
        }
        if (down_repeat_wait > 0)
        {
            down_repeat_wait--;
        }
        sleep;
    }
    return;
}

/* The position on the drum of a row offset rows from the selected one. */
Float:get_item_x(Float:offset)
{
    return 0.0 * offset;
}

Float:get_item_y(Float:offset)
{
    return -50.0 * sin(0.4 * offset);
}

Float:get_item_z(Float:offset)
{
    return -100.0 + 100.0 * cos(0.4 * offset);
}

/* Fades rows out toward the ends of the window kept around select. */
Float:get_item_alpha(Float:offset)
{
    new Float:alpha;
    if (offset < 0.0)
    {
        alpha = 1.0 - -offset / g_rows_above;
    }
    else
    {
        alpha = 1.0 - offset / (g_rows_below - 1);
    }
    if (alpha < 0.0)
    {
        alpha = 0.0;
    }
    if (alpha > 1.0)
    {
        alpha = 1.0;
    }
    return alpha;
}
