int strlen(char const* s)
{
    int len = 0;
    while (s[len])
        len++;
    return len;
}
