
#include "SecOC.h"
#include "SecOC_Debug.h"

extern void SecOC_test();

int main(void)
{
    #ifdef DEBUG_ALL
        SecOC_test();
    #endif

    (void)printf("Program ran successfully\n");
    return 0;
}