/*
 * The reduced mode of the background scene and the device of a menu item.
 */

/*
 * Switches the background scene to its reduced mode: it fades out over fade_out_time ms and back in over
 * fade_in_time ms.
 */
stock set_background_poor(Float:fade_in_time, fade_out_time)
{
    new scene = wndGetScene(WND_BG);
    if (scene)
    {
        SendMessage(scene, TAG("POOR"), fade_in_time, fade_out_time, 0, 0);
    }
}

/* Returns the device attribute of the item with id index in menu. */
stock get_item_device(menu, index)
{
    new device;
    new text[64];
    new node;
    sprintf(text, "%d", index);
    node = domFindTag(menu, "item", text, 0);
    domGetData(node, "device", text, 64);
    device = atoi(text);
    return device;
}

