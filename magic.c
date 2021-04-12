#include <assert.h> /* man assert(3) */
#include <stdlib.h> /* malloc lives here */
#include <string.h> /* strtok, strdup lives here */
#include <stdio.h> /* printf lives here */

char **split(const char *str, const char *delim)
{
    char *aux;
    char *p;
    char **res;
    char *argv[200]; /* place for 200 words. */
    int n = 0, i;

    assert((aux = strdup(str)));
    for (p = strtok(aux, delim); p; p = strtok(NULL, delim))
        argv[n++] = p;
    argv[n++] = NULL;
    /* i'll put de strdup()ed string one place past the NULL,
     * so you can free(3), once finished */
    argv[n++] = aux;
    /* now, we need to copy the array, so we can use it outside
     * this function. */
    assert((res = calloc(n, sizeof (char *))));
    for (i = 0; i < n; i++)
        res[i] = argv[i];
    return res;
}

int main()
{
    char* buffer = "It's okay guys";
    char **argv =
        split(buffer, " ");
    int i;

    for (i = 0; argv[i]; i++)
        printf("[%s]", argv[i]);
    puts("");  /* to end with a newline */

    free(argv[i+1]);
    free(argv);
}