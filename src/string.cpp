#include <compiz-core.h>

CompString compPrintf (const char *format, ...)
{
    va_list    ap;
    CompString rv;

    va_start (ap, format);
    rv = compPrintf(format, ap);
    va_end (ap);

    return rv;
}

CompString compPrintf (const char *format, va_list ap)
{
    va_list      aq;
    unsigned int size = strlen (format) + 1;
    int          n;
    char         *str;

    if (!format)
	return CompString ("");

    str = new char[size];

    if (!str)
	return CompString ("");

    while (1)
    {
	/* Try to print in the allocated space. */
	va_copy (aq, ap);
	n = vsnprintf (str, size, format, aq);
	va_end (aq);

	/* If that worked, leave the loop. */
	if (n > -1 && n < (int) size)
	    break;

	/* Else try again with more space. */
	if (n > -1)       /* glibc 2.1 */
	    size = n + 1; /* precisely what is needed */
	else              /* glibc 2.0 */
	    size++;       /* one more than the old size */

	delete [] str;
	str = new char[size];
	
	if (!str)
	{
	    return CompString ("");
	}
    }
    CompString rv(str);
    delete [] str;
    return rv;
}
