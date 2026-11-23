/*
 * The focus plane, a button sized to its label, password masking and the "KILL" message.
 */

/* Same as create_focus_plane of text.inc, with the default func of AttachScript (_) instead of "". */
stock create_focus_plane2(script[])
{
    new plane = CreatePlane3x3(Scene(0));
    SetTexture(plane, imgGetResource(RES_FOCUS), 0, -1);
    SetOptionI(plane, TAG("ALPH"), BLEND_ADD);
    AttachScript(plane, _, script);
    return plane;
}

#if !defined g_button_label
new g_button_label;
#endif
/* Waits until g_button_label has laid out its text, then enlarges button_w and button_h to hold it. */
stock fit_button_to_label(&Float:button_w, &Float:button_h)
{
    for (new rect[5]; ; )
    {
        GetOptionP(g_button_label, TAG("RECT"), rect, 5);
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

/* Copies src into dest with every character replaced by an asterisk. */
stock mask_password(dest[], src[])
{
    sprintf(dest, src);
    for (new i; src{i}; i++)
    {
        dest{i} = '*';
    }
}

stock kill_object(obj)
{
    SendMessage(obj, TAG("KILL"), 0, 0, 0, 0);
}

