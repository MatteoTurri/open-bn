/*
 * The format of a save device through the edclient server, behind a dialog.
 */

/* Formats the memory card device behind the dialog dialog_id; returns 1 when done, with the result in status. */
stock run_eds_format(focus_obj, device, dialog_id[], &status)
{
    new ok;
    status = edsClientFormat(device);
    if (0 != status)
    {
        printf("@@@ Error edsClientFormat() : [%d]\n", status);
        return 0;
    }
    status = wait_eds_with_dialog(focus_obj, dialog_id);
    switch (status)
    {
        case 0:
        {
            ok = 1;
        }
        case EDS_NO_CARD:
        {
        }
        case EDS_CARD_CHANGED:
        {
        }
        default:
        {
            printf("@@@@ Error : Unknown format status. : [%d]\n", status);
        }
    }
    return ok;
}

