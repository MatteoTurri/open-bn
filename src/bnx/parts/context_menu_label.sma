/*
 * /opt0/bn/script/parts/context_menu_label.bnx: Small 2.1.0, sc -d0 -t0 -\ -C+ -P+ -;+ -iinclude
 * An item of context_menu: the value of the "TEXT" tag number "index" of the XML group "xml_group", on a dot, in
 * the color and alpha the menu gives its object.
 */

#include <core>
#include <bn>
#include <float>

#pragma dynamic 1280

#include "tag.inc"

#include "text.inc"

new Float:g_dot_scale = 1.0;

public item_main()
{
    new self = Self();
    new group[256];
    GetArg("xml_group", group, 256);
    new index;
    {
        new value[256];
        GetArg("index", value, 256);
        index = atoi(value);
    }
    new dot;
    new label;
    dot = CreatePlane(self);
    SetLocalScale(dot, 10.0, 10.0, 10.0);
    SetPos(dot, 0.0, 0.0, 0.0, 0);
    SetScale(dot, g_dot_scale, g_dot_scale, g_dot_scale);
    SetTexture(dot, imgGetResource(RES_DOT), 0, -1);
    label = CreateLabel(self);
    SetPos(label, 0.0, 0.0, 0.0, 0);
    SetLocalScale(label, 0.16, 0.16, 0.16);
    {
        new text[256];
        get_group_tag_attr2("TEXT", group, index, "value", text, 256);
        SetOptionI(label, TAG("FONT"), FontID(0));
        SetOptionS(label, TAG("UTF8"), text);
    }
    for (; ; )
    {
        new Float:red;
        new Float:green;
        new Float:blue;
        new Float:alpha;
        GetColor(self, red, green, blue, alpha);
        SetColor(dot, red, green, blue, alpha);
        {
            new colors[4];
            new bytes[16] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
            new alpha_byte = floatround(255.0 * alpha, 0);
            bytes[3] = alpha_byte;
            bytes[7] = alpha_byte;
            bytes[11] = alpha_byte;
            bytes[15] = alpha_byte;
            pack_color_bytes(colors, bytes);
            SetOptionP(label, TAG("COL4"), colors, 4);
        }
        sleep;
    }
    return;
}
