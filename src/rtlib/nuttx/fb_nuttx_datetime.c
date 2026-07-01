/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_datetime.c

    Purpose:

        Provide the first DATE/TIME calculation support for the NuttX
        generated-C smoke target.

    Responsibilities:

        - parse the simple English date/time forms used by fbctests
        - encode and decode FreeBASIC/VB-style serial date values
        - implement DATEADD, DATEDIFF, and DATEPART interval math

    This file intentionally does NOT contain:

        - locale-aware date parsing
        - date formatting
        - system clock mutation
        - timezone handling
*/

#define FB_NUTTX_TIME_INTERVAL_INVALID      0
#define FB_NUTTX_TIME_INTERVAL_YEAR         1
#define FB_NUTTX_TIME_INTERVAL_QUARTER      2
#define FB_NUTTX_TIME_INTERVAL_MONTH        3
#define FB_NUTTX_TIME_INTERVAL_DAY_OF_YEAR  4
#define FB_NUTTX_TIME_INTERVAL_DAY          5
#define FB_NUTTX_TIME_INTERVAL_WEEKDAY      6
#define FB_NUTTX_TIME_INTERVAL_WEEK_OF_YEAR 7
#define FB_NUTTX_TIME_INTERVAL_HOUR         8
#define FB_NUTTX_TIME_INTERVAL_MINUTE       9
#define FB_NUTTX_TIME_INTERVAL_SECOND       10

#define FB_NUTTX_WEEK_DAY_SYSTEM            0
#define FB_NUTTX_WEEK_DAY_DEFAULT           1
#define FB_NUTTX_WEEK_FIRST_SYSTEM          0
#define FB_NUTTX_WEEK_FIRST_JAN_1           1
#define FB_NUTTX_WEEK_FIRST_FOUR_DAYS       2
#define FB_NUTTX_WEEK_FIRST_FULL_WEEK       3
#define FB_NUTTX_RTERROR_OK                 0
#define FB_NUTTX_RTERROR_ILLEGALFUNCTIONCALL 1

static int fb_nuttx_i18n_on = 1;

double fb_FIXDouble(double value);

static double fb_nuttx_floor(double value)
{
    int64_t whole;

    whole = (int64_t)value;

    if ((value < 0.0) && ((double)whole != value))
        whole--;

    return (double)whole;
}

static int fb_nuttx_leap_year(int year)
{
    if ((year % 400) == 0)
        return 1;

    if ((year % 100) == 0)
        return 0;

    return ((year & 3) == 0) ? 1 : 0;
}

static int fb_nuttx_days_in_month(int month, int year)
{
    static const int days[] =
        { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if ((month < 1) || (month > 12))
        return 31;

    if (month == 2)
        return days[month - 1] + fb_nuttx_leap_year(year);

    return days[month - 1];
}

static int fb_nuttx_days_in_year(int year)
{
    return fb_nuttx_leap_year(year) ? 366 : 365;
}

#if !defined(FB_NUTTX_USE_GENERIC_DATETIME_MATH) || \
    (FB_NUTTX_USE_GENERIC_DATETIME_MATH == 0)
static int fb_nuttx_sign(double value)
{
    if (value > 0.0)
        return 1;

    if (value < 0.0)
        return -1;

    return 0;
}

static void fb_nuttx_normalize_date(int *day_ptr, int *month_ptr,
    int *year_ptr)
{
    int day;
    int month;
    int year;

    day = *day_ptr;
    month = *month_ptr;
    year = *year_ptr;

    if (month < 1) {
        int sub_months;
        int sub_years;

        sub_months = -month + 1;
        sub_years = (sub_months + 11) / 12;
        year -= sub_years;
        month = sub_years * 12 - sub_months + 1;
    } else {
        month--;
        year += month / 12;
        month %= 12;
        month++;
    }

    if (day < 1) {
        int sub_days;

        sub_days = -day + 1;

        while (sub_days > 0) {
            int days_in_month;

            month--;

            if (month == 0) {
                month = 12;
                year--;
            }

            days_in_month = fb_nuttx_days_in_month(month, year);

            if (sub_days > days_in_month) {
                sub_days -= days_in_month;
            } else {
                day = days_in_month - sub_days + 1;
                sub_days = 0;
            }
        }
    } else {
        int days_in_month;

        days_in_month = fb_nuttx_days_in_month(month, year);

        while (day > days_in_month) {
            day -= days_in_month;
            month++;

            if (month == 13) {
                month = 1;
                year++;
            }

            days_in_month = fb_nuttx_days_in_month(month, year);
        }
    }

    *day_ptr = day;
    *month_ptr = month;
    *year_ptr = year;
}

static int fb_nuttx_interval_type(FBSTRING *interval)
{
    const char *text;

    if ((interval == NULL) || (interval->data == NULL))
        return FB_NUTTX_TIME_INTERVAL_INVALID;

    text = interval->data;

    if (strcmp(text, "yyyy") == 0)
        return FB_NUTTX_TIME_INTERVAL_YEAR;
    if (strcmp(text, "q") == 0)
        return FB_NUTTX_TIME_INTERVAL_QUARTER;
    if (strcmp(text, "m") == 0)
        return FB_NUTTX_TIME_INTERVAL_MONTH;
    if (strcmp(text, "y") == 0)
        return FB_NUTTX_TIME_INTERVAL_DAY_OF_YEAR;
    if (strcmp(text, "d") == 0)
        return FB_NUTTX_TIME_INTERVAL_DAY;
    if (strcmp(text, "w") == 0)
        return FB_NUTTX_TIME_INTERVAL_WEEKDAY;
    if (strcmp(text, "ww") == 0)
        return FB_NUTTX_TIME_INTERVAL_WEEK_OF_YEAR;
    if (strcmp(text, "h") == 0)
        return FB_NUTTX_TIME_INTERVAL_HOUR;
    if (strcmp(text, "n") == 0)
        return FB_NUTTX_TIME_INTERVAL_MINUTE;
    if (strcmp(text, "s") == 0)
        return FB_NUTTX_TIME_INTERVAL_SECOND;

    return FB_NUTTX_TIME_INTERVAL_INVALID;
}
#endif

static int fb_nuttx_month_number(const char *name)
{
    static const char *months[] = {
        "jan", "feb", "mar", "apr", "may", "jun",
        "jul", "aug", "sep", "oct", "nov", "dec"
    };
    char lower[4];
    int i;

    if (name == NULL)
        return 0;

    for (i = 0; i < 3; i++) {
        if (name[i] == '\0')
            return 0;

        lower[i] = (char)tolower((unsigned char)name[i]);
    }

    lower[3] = '\0';

    for (i = 0; i < 12; i++) {
        if (strcmp(lower, months[i]) == 0)
            return i + 1;
    }

    return 0;
}

static int fb_nuttx_parse_datetime(FBSTRING *s, int *year, int *month,
    int *day, int *hour, int *minute, int *second)
{
    char month_name[16];
    int parsed_day;
    int parsed_year;
    int parsed_hour;
    int parsed_minute;
    int parsed_second;
    int parsed_month;
    const char *text;

    if ((s == NULL) || (s->data == NULL))
        return 0;

    text = s->data;
    parsed_hour = 0;
    parsed_minute = 0;
    parsed_second = 0;

    if (sscanf(text, " %15s %d , %d %d:%d:%d", month_name, &parsed_day,
        &parsed_year, &parsed_hour, &parsed_minute, &parsed_second) < 3)
        return 0;

    parsed_month = fb_nuttx_month_number(month_name);

    if (parsed_month == 0)
        return 0;

    if ((parsed_day < 1) ||
        (parsed_day > fb_nuttx_days_in_month(parsed_month, parsed_year)))
        return 0;

    if ((parsed_hour < 0) || (parsed_hour > 23) ||
        (parsed_minute < 0) || (parsed_minute > 59) ||
        (parsed_second < 0) || (parsed_second > 59))
        return 0;

    if (year != NULL)
        *year = parsed_year;
    if (month != NULL)
        *month = parsed_month;
    if (day != NULL)
        *day = parsed_day;
    if (hour != NULL)
        *hour = parsed_hour;
    if (minute != NULL)
        *minute = parsed_minute;
    if (second != NULL)
        *second = parsed_second;

    return 1;
}

#if !defined(FB_NUTTX_USE_GENERIC_DATETIME_MATH) || \
    (FB_NUTTX_USE_GENERIC_DATETIME_MATH == 0)
int fb_DateSerial(int year, int month, int day)
{
    int result;
    int current_year;
    int current_month;

    result = 2;
    current_year = 1900;
    current_month = 1;

    fb_nuttx_normalize_date(&day, &month, &year);

    if (current_year < year) {
        while (current_year != year)
            result += fb_nuttx_days_in_year(current_year++);
    } else {
        while (current_year != year)
            result -= fb_nuttx_days_in_year(--current_year);
    }

    while (current_month != month)
        result += fb_nuttx_days_in_month(current_month++, year);

    result += day - 1;

    return result;
}

double fb_TimeSerial(int hour, int minute, int second)
{
    return ((double)hour / 24.0) +
        ((double)minute / (24.0 * 60.0)) +
        ((double)second / (24.0 * 60.0 * 60.0));
}
#endif

static void fb_nuttx_decode_date(double serial, int *year_ptr,
    int *month_ptr, int *day_ptr)
{
    int tmp_days;
    int current_year;
    int current_month;
    int current_day;

    current_year = 1900;
    current_month = 1;
    current_day = 1;
    serial = fb_nuttx_floor(serial) - 2.0;

    while (serial < 0.0) {
        current_year--;
        serial += fb_nuttx_days_in_year(current_year);
    }

    while (serial >= (double)(tmp_days = fb_nuttx_days_in_year(current_year))) {
        serial -= tmp_days;
        current_year++;
    }

    if ((month_ptr != NULL) || (day_ptr != NULL)) {
        while (serial >=
            (double)(tmp_days = fb_nuttx_days_in_month(current_month, current_year))) {
            serial -= tmp_days;
            current_month++;
        }
    }

    current_day += (int)serial;

    if (year_ptr != NULL)
        *year_ptr = current_year;
    if (month_ptr != NULL)
        *month_ptr = current_month;
    if (day_ptr != NULL)
        *day_ptr = current_day;
}

static void fb_nuttx_decode_time(double serial, int *hour_ptr,
    int *minute_ptr, int *second_ptr)
{
    int hour;
    int minute;
    int second;
    double whole;

    whole = fb_nuttx_floor(serial);
    serial -= whole;

    if (serial < 0.0)
        serial += 1.0;

    serial += 0.000000001;
    serial *= 24.0;
    hour = (int)serial;
    serial -= hour;
    serial *= 60.0;
    minute = (int)serial;
    serial -= minute;
    serial *= 60.0;
    second = (int)serial;

    if (hour_ptr != NULL)
        *hour_ptr = hour;
    if (minute_ptr != NULL)
        *minute_ptr = minute;
    if (second_ptr != NULL)
        *second_ptr = second;
}

double fb_Now(void)
{
    time_t now;
    struct tm *local_now;

    now = time(NULL);
    local_now = localtime(&now);

    if (local_now == NULL)
        return 0.0;

    return (double)fb_DateSerial(local_now->tm_year + 1900,
            local_now->tm_mon + 1, local_now->tm_mday) +
        fb_TimeSerial(local_now->tm_hour, local_now->tm_min,
            local_now->tm_sec);
}

static void fb_nuttx_format_append(char *buffer, size_t buffer_size,
    size_t *pos, const char *text)
{
    while ((text != NULL) && (*text != '\0') && ((*pos + 1) < buffer_size)) {
        buffer[*pos] = *text;
        (*pos)++;
        text++;
    }
}

static void fb_nuttx_format_append_2_digits(char *buffer, size_t buffer_size,
    size_t *pos, int value)
{
    char text[3];

    if (value < 0)
        value = 0;
    if (value > 99)
        value %= 100;

    text[0] = (char)('0' + (value / 10));
    text[1] = (char)('0' + (value % 10));
    text[2] = '\0';

    fb_nuttx_format_append(buffer, buffer_size, pos, text);
}

static void fb_nuttx_format_append_int(char *buffer, size_t buffer_size,
    size_t *pos, int value)
{
    char text[32];

    snprintf(text, sizeof(text), "%d", value);
    fb_nuttx_format_append(buffer, buffer_size, pos, text);
}

FBSTRING *fb_StrFormat(double value, const FBSTRING *mask)
{
    char buffer[128];
    const char *format;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    size_t i;
    size_t pos;

    if ((mask == NULL) || (mask->data == NULL) || (mask->len <= 0)) {
        snprintf(buffer, sizeof(buffer), "%.15g", value);
        return fb_StrAllocTempDescZEx(buffer, (int32)strlen(buffer));
    }

    format = mask->data;

    fb_nuttx_decode_date(value, &year, &month, &day);
    fb_nuttx_decode_time(value, &hour, &minute, &second);

    pos = 0;

    for (i = 0; (i < (size_t)mask->len) && ((pos + 1) < sizeof(buffer)); ) {
        if (strncmp(&format[i], "ddddd", 5) == 0) {
            fb_nuttx_format_append_2_digits(buffer, sizeof(buffer), &pos,
                month);
            fb_nuttx_format_append(buffer, sizeof(buffer), &pos, "-");
            fb_nuttx_format_append_2_digits(buffer, sizeof(buffer), &pos, day);
            fb_nuttx_format_append(buffer, sizeof(buffer), &pos, "-");
            fb_nuttx_format_append_int(buffer, sizeof(buffer), &pos, year);
            i += 5;
        } else if (strncmp(&format[i], "ttttt", 5) == 0) {
            fb_nuttx_format_append_2_digits(buffer, sizeof(buffer), &pos,
                hour);
            fb_nuttx_format_append(buffer, sizeof(buffer), &pos, ":");
            fb_nuttx_format_append_2_digits(buffer, sizeof(buffer), &pos,
                minute);
            fb_nuttx_format_append(buffer, sizeof(buffer), &pos, ":");
            fb_nuttx_format_append_2_digits(buffer, sizeof(buffer), &pos,
                second);
            i += 5;
        } else if (strncmp(&format[i], "yyyy", 4) == 0) {
            fb_nuttx_format_append_int(buffer, sizeof(buffer), &pos, year);
            i += 4;
        } else if (strncmp(&format[i], "yy", 2) == 0) {
            fb_nuttx_format_append_2_digits(buffer, sizeof(buffer), &pos,
                year % 100);
            i += 2;
        } else if (strncmp(&format[i], "hh", 2) == 0) {
            fb_nuttx_format_append_2_digits(buffer, sizeof(buffer), &pos,
                hour);
            i += 2;
        } else if (strncmp(&format[i], "nn", 2) == 0) {
            fb_nuttx_format_append_2_digits(buffer, sizeof(buffer), &pos,
                minute);
            i += 2;
        } else if (strncmp(&format[i], "ss", 2) == 0) {
            fb_nuttx_format_append_2_digits(buffer, sizeof(buffer), &pos,
                second);
            i += 2;
        } else if (strncmp(&format[i], "mm", 2) == 0) {
            fb_nuttx_format_append_2_digits(buffer, sizeof(buffer), &pos,
                month);
            i += 2;
        } else if (strncmp(&format[i], "dd", 2) == 0) {
            fb_nuttx_format_append_2_digits(buffer, sizeof(buffer), &pos, day);
            i += 2;
        } else if (format[i] == 'm') {
            fb_nuttx_format_append_int(buffer, sizeof(buffer), &pos, month);
            i++;
        } else if (format[i] == 'd') {
            fb_nuttx_format_append_int(buffer, sizeof(buffer), &pos, day);
            i++;
        } else if (format[i] == 'h') {
            fb_nuttx_format_append_int(buffer, sizeof(buffer), &pos, hour);
            i++;
        } else if (format[i] == 'n') {
            fb_nuttx_format_append_int(buffer, sizeof(buffer), &pos, minute);
            i++;
        } else if (format[i] == 's') {
            fb_nuttx_format_append_int(buffer, sizeof(buffer), &pos, second);
            i++;
        } else {
            buffer[pos] = format[i];
            pos++;
            i++;
        }
    }

    buffer[pos] = '\0';

    return fb_StrAllocTempDescZEx(buffer, (int32)pos);
}

#if !defined(FB_NUTTX_USE_GENERIC_DATETIME_MATH) || \
    (FB_NUTTX_USE_GENERIC_DATETIME_MATH == 0)
int fb_Year(double serial)
{
    int year;

    fb_nuttx_decode_date(serial, &year, NULL, NULL);

    return year;
}

int fb_Month(double serial)
{
    int month;

    fb_nuttx_decode_date(serial, NULL, &month, NULL);

    return month;
}

int fb_Day(double serial)
{
    int day;

    fb_nuttx_decode_date(serial, NULL, NULL, &day);

    return day;
}

int fb_Hour(double serial)
{
    int hour;

    fb_nuttx_decode_time(serial, &hour, NULL, NULL);

    return hour;
}

int fb_Minute(double serial)
{
    int minute;

    fb_nuttx_decode_time(serial, NULL, &minute, NULL);

    return minute;
}

int fb_Second(double serial)
{
    int second;

    fb_nuttx_decode_time(serial, NULL, NULL, &second);

    return second;
}
#endif

FBSTRING *fb_MonthName(int month, int abbreviation)
{
    static const char *long_names[] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    static const char *short_names[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    const char *name;

    if ((month < 1) || (month > 12))
        return fb_StrAllocTempDescZEx("", 0);

    name = (abbreviation != 0) ? short_names[month - 1] :
        long_names[month - 1];

    return fb_StrAllocTempDescZEx(name, (int32)strlen(name));
}

FBSTRING *fb_WeekdayName(int weekday, int abbreviation,
    int first_day_of_week)
{
    static const char *long_names[] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
        "Saturday"
    };
    static const char *short_names[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    int index;
    const char *name;

    if (first_day_of_week == FB_NUTTX_WEEK_DAY_SYSTEM)
        first_day_of_week = FB_NUTTX_WEEK_DAY_DEFAULT;

    if ((weekday < 1) || (weekday > 7) ||
        (first_day_of_week < 1) || (first_day_of_week > 7))
        return fb_StrAllocTempDescZEx("", 0);

    index = weekday + first_day_of_week - 2;

    while (index >= 7)
        index -= 7;

    name = (abbreviation != 0) ? short_names[index] : long_names[index];

    return fb_StrAllocTempDescZEx(name, (int32)strlen(name));
}

#if !defined(FB_NUTTX_USE_GENERIC_DATETIME_MATH) || \
    (FB_NUTTX_USE_GENERIC_DATETIME_MATH == 0)
static int fb_nuttx_day_of_year(int year, int month, int day)
{
    int result;
    int current_month;

    result = 0;

    for (current_month = 1; current_month < month; current_month++)
        result += fb_nuttx_days_in_month(current_month, year);

    return result + day;
}

static int fb_nuttx_weekday(double serial, int first_day_of_week)
{
    int dow;

    dow = ((int)(fb_nuttx_floor(serial) - 1.0) % 7) + 1;

    if (first_day_of_week == FB_NUTTX_WEEK_DAY_SYSTEM)
        first_day_of_week = FB_NUTTX_WEEK_DAY_DEFAULT;

    dow -= first_day_of_week - 1;

    if (dow < 1)
        dow += 7;
    else if (dow > 7)
        dow -= 7;

    return dow;
}

int fb_Weekday(double serial, int first_day_of_week)
{
    return fb_nuttx_weekday(serial, first_day_of_week);
}

static void fb_nuttx_begin_of_week(int *year, int *month, int *day,
    int first_day_of_week)
{
    double serial;
    int weekday;

    serial = (double)fb_DateSerial(*year, *month, *day);
    weekday = fb_nuttx_weekday(serial, first_day_of_week);
    serial -= weekday - 1;
    fb_nuttx_decode_date(serial, year, month, day);
}

static double fb_nuttx_first_week_of_year(int year, int first_day_of_year,
    int first_day_of_week)
{
    int first_week_year;
    int first_week_month;
    int first_week_day;
    int remaining_weekdays;
    double serial_week_begin;
    double serial_year_begin;

    if (first_day_of_year == FB_NUTTX_WEEK_FIRST_SYSTEM)
        first_day_of_year = FB_NUTTX_WEEK_FIRST_JAN_1;

    serial_year_begin = (double)fb_DateSerial(year, 1, 1);
    first_week_year = year;
    first_week_month = 1;
    first_week_day = 1;
    fb_nuttx_begin_of_week(&first_week_year, &first_week_month,
        &first_week_day, first_day_of_week);

    serial_week_begin = (double)fb_DateSerial(first_week_year,
        first_week_month, first_week_day);
    remaining_weekdays = (int)((serial_week_begin + 7.0) - serial_year_begin);

    if ((first_day_of_year == FB_NUTTX_WEEK_FIRST_FOUR_DAYS) &&
        (remaining_weekdays < 4))
        serial_week_begin += 7.0;

    if ((first_day_of_year == FB_NUTTX_WEEK_FIRST_FULL_WEEK) &&
        (remaining_weekdays < 7))
        serial_week_begin += 7.0;

    return serial_week_begin;
}

static int fb_nuttx_week_of_year(int ref_year, double serial,
    int first_day_of_year, int first_day_of_week)
{
    int sign;
    int week;
    double serial_first_week;

    serial_first_week = fb_nuttx_first_week_of_year(ref_year,
        first_day_of_year, first_day_of_week);
    serial = fb_nuttx_floor(serial - serial_first_week);
    sign = fb_nuttx_sign(serial);
    serial /= 7.0;
    week = (int)(serial + sign);

    return week;
}
#endif

void fb_I18nSet(int on_off)
{
    fb_nuttx_i18n_on = on_off != 0;
}

int fb_I18nGet(void)
{
    return fb_nuttx_i18n_on;
}

int fb_DateValue(FBSTRING *s)
{
    int year;
    int month;
    int day;

    if (fb_nuttx_parse_datetime(s, &year, &month, &day, NULL, NULL, NULL) == 0) {
        fb_nuttx_error_num = FB_NUTTX_RTERROR_ILLEGALFUNCTIONCALL;
        return 0;
    }

    fb_nuttx_error_num = FB_NUTTX_RTERROR_OK;

    return fb_DateSerial(year, month, day);
}

double fb_TimeValue(FBSTRING *s)
{
    int hour;
    int minute;
    int second;

    if (fb_nuttx_parse_datetime(s, NULL, NULL, NULL,
        &hour, &minute, &second) == 0) {
        fb_nuttx_error_num = FB_NUTTX_RTERROR_ILLEGALFUNCTIONCALL;
        return 0.0;
    }

    fb_nuttx_error_num = FB_NUTTX_RTERROR_OK;

    return fb_TimeSerial(hour, minute, second);
}

#if !defined(FB_NUTTX_USE_GENERIC_DATETIME_MATH) || \
    (FB_NUTTX_USE_GENERIC_DATETIME_MATH == 0)
int fb_DatePart(FBSTRING *interval, double serial, int first_day_of_week,
    int first_day_of_year)
{
    int result;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;

    result = 0;
    fb_nuttx_error_num = FB_NUTTX_RTERROR_OK;

    switch (fb_nuttx_interval_type(interval)) {
    case FB_NUTTX_TIME_INTERVAL_YEAR:
        fb_nuttx_decode_date(serial, &year, NULL, NULL);
        result = year;
        break;
    case FB_NUTTX_TIME_INTERVAL_QUARTER:
        fb_nuttx_decode_date(serial, NULL, &month, NULL);
        result = ((month - 1) / 3) + 1;
        break;
    case FB_NUTTX_TIME_INTERVAL_MONTH:
        fb_nuttx_decode_date(serial, NULL, &month, NULL);
        result = month;
        break;
    case FB_NUTTX_TIME_INTERVAL_DAY_OF_YEAR:
        fb_nuttx_decode_date(serial, &year, &month, &day);
        result = fb_nuttx_day_of_year(year, month, day);
        break;
    case FB_NUTTX_TIME_INTERVAL_DAY:
        fb_nuttx_decode_date(serial, NULL, NULL, &day);
        result = day;
        break;
    case FB_NUTTX_TIME_INTERVAL_WEEKDAY:
        result = fb_nuttx_weekday(serial, first_day_of_week);
        break;
    case FB_NUTTX_TIME_INTERVAL_WEEK_OF_YEAR:
        fb_nuttx_decode_date(serial, &year, NULL, NULL);
        result = fb_nuttx_week_of_year(year, serial, first_day_of_year,
            first_day_of_week);

        if (result < 0)
            result = fb_nuttx_week_of_year(year - 1, serial,
                first_day_of_year, first_day_of_week);
        break;
    case FB_NUTTX_TIME_INTERVAL_HOUR:
        fb_nuttx_decode_time(serial, &hour, NULL, NULL);
        result = hour;
        break;
    case FB_NUTTX_TIME_INTERVAL_MINUTE:
        fb_nuttx_decode_time(serial, NULL, &minute, NULL);
        result = minute;
        break;
    case FB_NUTTX_TIME_INTERVAL_SECOND:
        fb_nuttx_decode_time(serial, NULL, NULL, &second);
        result = second;
        break;
    default:
        fb_nuttx_error_num = FB_NUTTX_RTERROR_ILLEGALFUNCTIONCALL;
        break;
    }

    return result;
}

double fb_DateAdd(FBSTRING *interval, double interval_value_arg, double serial)
{
    int interval_value;
    int interval_type;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;

    interval_value = (int)fb_FIXDouble(interval_value_arg);
    interval_type = fb_nuttx_interval_type(interval);
    fb_nuttx_error_num = FB_NUTTX_RTERROR_OK;

    fb_nuttx_decode_time(serial, &hour, &minute, &second);
    fb_nuttx_decode_date(serial, &year, &month, &day);

    switch (interval_type) {
    case FB_NUTTX_TIME_INTERVAL_YEAR:
        year += interval_value;
        break;
    case FB_NUTTX_TIME_INTERVAL_QUARTER:
        month += interval_value * 3;
        break;
    case FB_NUTTX_TIME_INTERVAL_MONTH:
        month += interval_value;
        break;
    case FB_NUTTX_TIME_INTERVAL_DAY_OF_YEAR:
    case FB_NUTTX_TIME_INTERVAL_DAY:
    case FB_NUTTX_TIME_INTERVAL_WEEKDAY:
        day += interval_value;
        break;
    case FB_NUTTX_TIME_INTERVAL_WEEK_OF_YEAR:
        day += interval_value * 7;
        break;
    case FB_NUTTX_TIME_INTERVAL_HOUR:
        hour += interval_value;
        break;
    case FB_NUTTX_TIME_INTERVAL_MINUTE:
        minute += interval_value;
        break;
    case FB_NUTTX_TIME_INTERVAL_SECOND:
        second += interval_value;
        break;
    default:
        fb_nuttx_error_num = FB_NUTTX_RTERROR_ILLEGALFUNCTIONCALL;
        break;
    }

    if ((interval_type == FB_NUTTX_TIME_INTERVAL_YEAR) ||
        (interval_type == FB_NUTTX_TIME_INTERVAL_QUARTER) ||
        (interval_type == FB_NUTTX_TIME_INTERVAL_MONTH)) {
        int carry_value;
        int days_in_month;

        if (month < 1)
            carry_value = (month - 12) / 12;
        else
            carry_value = (month - 1) / 12;

        year += carry_value;
        month -= carry_value * 12;
        days_in_month = fb_nuttx_days_in_month(month, year);

        if (day > days_in_month)
            day = days_in_month;
    }

    return (double)fb_DateSerial(year, month, day) +
        fb_TimeSerial(hour, minute, second);
}

int64_t fb_DateDiff(FBSTRING *interval, double serial1, double serial2,
    int first_day_of_week, int first_day_of_year)
{
    int interval_type;
    int year1;
    int year2;
    int month1;
    int month2;
    int hour;
    int minute;
    int second;
    int week;
    int64_t result;
    double serial;

    interval_type = fb_nuttx_interval_type(interval);
    result = 0;
    fb_nuttx_error_num = FB_NUTTX_RTERROR_OK;

    switch (interval_type) {
    case FB_NUTTX_TIME_INTERVAL_YEAR:
        fb_nuttx_decode_date(serial1, &year1, NULL, NULL);
        fb_nuttx_decode_date(serial2, &year2, NULL, NULL);
        result = year2 - year1;
        break;
    case FB_NUTTX_TIME_INTERVAL_QUARTER:
    case FB_NUTTX_TIME_INTERVAL_MONTH:
        fb_nuttx_decode_date(serial1, &year1, &month1, NULL);
        fb_nuttx_decode_date(serial2, &year2, &month2, NULL);
        result = (month2 - month1) + ((year2 - year1) * 12);

        if (interval_type == FB_NUTTX_TIME_INTERVAL_QUARTER)
            result /= 3;
        break;
    case FB_NUTTX_TIME_INTERVAL_DAY_OF_YEAR:
    case FB_NUTTX_TIME_INTERVAL_DAY:
        result = (int64_t)(fb_nuttx_floor(serial2) -
            fb_nuttx_floor(serial1));
        break;
    case FB_NUTTX_TIME_INTERVAL_WEEKDAY:
    case FB_NUTTX_TIME_INTERVAL_WEEK_OF_YEAR:
        fb_nuttx_decode_date(serial1, &year1, NULL, NULL);
        week = fb_nuttx_week_of_year(year1, serial1, first_day_of_year,
            first_day_of_week);
        result = fb_nuttx_week_of_year(year1, serial2, first_day_of_year,
            first_day_of_week);

        if (week > 0)
            week--;
        if (result > 0)
            result--;

        result -= week;

        if (interval_type == FB_NUTTX_TIME_INTERVAL_WEEKDAY) {
            int add_value;

            if (serial1 > serial2) {
                double serial_tmp;

                serial_tmp = serial1;
                serial1 = serial2;
                serial2 = serial_tmp;
                add_value = 1;
            } else {
                add_value = -1;
            }

            if (fb_nuttx_weekday(serial1, first_day_of_week) >
                fb_nuttx_weekday(serial2, first_day_of_week))
                result += add_value;
        }
        break;
    case FB_NUTTX_TIME_INTERVAL_HOUR:
        serial = serial2 - serial1;
        fb_nuttx_decode_time(serial, &hour, NULL, NULL);
        result = (int64_t)(hour + (fb_nuttx_floor(serial) * 24.0));
        break;
    case FB_NUTTX_TIME_INTERVAL_MINUTE:
        serial = serial2 - serial1;
        fb_nuttx_decode_time(serial, &hour, &minute, NULL);
        result = (int64_t)(minute +
            ((hour + (fb_nuttx_floor(serial) * 24.0)) * 60.0));
        break;
    case FB_NUTTX_TIME_INTERVAL_SECOND:
        serial = serial2 - serial1;
        fb_nuttx_decode_time(serial, &hour, &minute, &second);
        result = (int64_t)(second +
            ((minute + ((hour + (fb_nuttx_floor(serial) * 24.0)) * 60.0)) *
                60.0));
        break;
    default:
        fb_nuttx_error_num = FB_NUTTX_RTERROR_ILLEGALFUNCTIONCALL;
        break;
    }

    return result;
}
#endif

int32 fb_SetDate(const FBSTRING *date_text)
{
    /*
        Setting the board clock is configuration and privilege dependent on
        NuttX.  Keep the runtime symbol available, but report the operation as
        unsupported until the target has a board-aware clock layer.
    */
    (void)date_text;

    fb_nuttx_error_num = FB_NUTTX_RTERROR_ILLEGALFUNCTIONCALL;

    return -1;
}

int32 fb_SetTime(const FBSTRING *time_text)
{
    (void)time_text;

    fb_nuttx_error_num = FB_NUTTX_RTERROR_ILLEGALFUNCTIONCALL;

    return -1;
}

/* end of fb_nuttx_datetime.c */
