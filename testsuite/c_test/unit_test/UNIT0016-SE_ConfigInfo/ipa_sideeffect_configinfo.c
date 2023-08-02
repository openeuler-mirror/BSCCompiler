#include <stdio.h>

int main()
{
    char buffer[64];
    buffer[0] = 0;
    // {PI::kWriteMemoryOnly, PI::kReadMemoryOnly, PI::kReadMemoryOnly, PI::kReadMemoryOnly}
    // Let opnd0 == opnd2, make readMemOnly opnd can be written.
    sprintf(buffer, "%p", buffer);


    if ( buffer[0] != 48) {  // charater '0' ascii value is 48
        abort();
        return 1;
    }

    return 0;
}
