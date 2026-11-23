/*
 * Window variables: values kept as __wndvar_int__ and __wndvar_str__ elements under a __wndvar__ element of the
 * document.
 */

/* Reads an integer window variable; returns 0 if it does not exist. */
stock get_wnd_var_int(name[], &value)
{
    new vars = domFindTag(domRoot(), "__wndvar__", _, 0);
    if (!vars)
    {
        return 0;
    }
    new node = domFindTag(vars, "__wndvar_int__", name, 0);
    if (!node)
    {
        return 0;
    }
    new text[8];
    domGetData(node, "value", text, 8);
    value = atoi(text);
    return 1;
}

/* Same as get_wnd_var_int. */
stock get_wnd_var_int2(name[], &value)
{
    new vars = domFindTag(domRoot(), "__wndvar__", _, 0);
    if (!vars)
    {
        return 0;
    }
    new node = domFindTag(vars, "__wndvar_int__", name, 0);
    if (!node)
    {
        return 0;
    }
    new text[8];
    domGetData(node, "value", text, 8);
    value = atoi(text);
    return 1;
}

stock set_wnd_var_str(name[], value[])
{
    new vars = domFindTag(domRoot(), "__wndvar__", _, 0);
    if (!vars)
    {
        vars = domInsertTag(domFirstChild(domRoot()), "__wndvar__");
    }
    new node = domFindTag(vars, "__wndvar_str__", name, 0);
    if (!node)
    {
        node = domAddTag(vars, "__wndvar_str__");
        domSetData(node, "id", name);
    }
    domSetData(node, "value", value);
}

