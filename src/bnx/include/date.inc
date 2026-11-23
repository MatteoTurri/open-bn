/*
 * Splitting of dates written as YYYYMMDD.
 */

/* Splits YYYYMMDD into year, month and day; all empty if date_text is shorter. */
stock split_date(date_text[], year_text[], month_text[], day_text[], year_max, month_max, day_max)
{
    if (strlen(date_text) < 8)
    {
        year_text{0} = 0;
        month_text{0} = 0;
        day_text{0} = 0;
        return;
    }
    copy_substring(year_text, 0, date_text, 0, 4, year_max);
    copy_substring(month_text, 0, date_text, 4, 2, month_max);
    copy_substring(day_text, 0, date_text, 6, 2, day_max);
}

/* Splits YYYYMM into year and month; both empty if date_text is shorter. */
stock split_year_month(date_text[], year_text[], month_text[], year_max, month_max)
{
    if (strlen(date_text) < 6)
    {
        year_text{0} = 0;
        month_text{0} = 0;
        return;
    }
    copy_substring(year_text, 0, date_text, 0, 4, year_max);
    copy_substring(month_text, 0, date_text, 4, 2, month_max);
}

/* Splits YYYYMM into the last two digits of the year and the month. */
stock split_short_year_month(date_text[], year_text[], month_text[], year_max, month_max)
{
    if (strlen(date_text) < 6)
    {
        year_text{0} = 0;
        month_text{0} = 0;
        return;
    }
    copy_substring(year_text, 0, date_text, 2, 2, year_max);
    copy_substring(month_text, 0, date_text, 4, 2, month_max);
}

