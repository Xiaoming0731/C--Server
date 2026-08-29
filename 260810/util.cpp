// #include "util.h"
// #include <stdio.h>
// #include <stdlib.h>

// void errif(bool condition,const char *error_message)
// {
//     if(condition)
//     {
//         perror(error_message);
//         exit(EXIT_FAILURE);
//     }
// }



#include "util.h"
#include <stdio.h>
#include <stdlib.h>

void errif(bool condition,const char * error_message)
{
    if(condition)
    {
        perror(error_message);
        exit(EXIT_FAILURE);
    }
}
    