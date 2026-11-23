/*
 * String window variables and the image decoder.
 */

/* Reads a string window variable; returns 0 if it does not exist. */
stock get_wnd_var_str(name[], value[], maxlen)
{
    new vars = domFindTag(domRoot(), "__wndvar__", _, 0);
    if (!vars)
    {
        return 0;
    }
    new node = domFindTag(vars, "__wndvar_str__", name, 0);
    if (!node)
    {
        return 0;
    }
    domGetData(node, "value", value, maxlen);
    return 1;
}

/* Same as get_wnd_var_str. */
stock get_wnd_var_str2(name[], value[], maxlen)
{
    new vars = domFindTag(domRoot(), "__wndvar__", _, 0);
    if (!vars)
    {
        return 0;
    }
    new node = domFindTag(vars, "__wndvar_str__", name, 0);
    if (!node)
    {
        return 0;
    }
    domGetData(node, "value", value, maxlen);
    return 1;
}

/* Same as start_image_decode. */
stock start_image_decode2(url[])
{
    return imgDecStart(url, 0, 0, -1, 100, 0);
}

stock poll_image_decode()
{
    return imgDecSync();
}

stock finish_image_decode()
{
    return wait_image_decode();
}

