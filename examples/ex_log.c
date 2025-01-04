#include "myc/assert.h"
#include "myc/log.h"

int main(void)
{
    int digit = 3;
    
    MYC_LOG_TRACE("I tell you were the error below comes from :)");
    MYC_LOG_ERROR("I am an error!");
    MYC_LOG_WARN("I am a warning.");
    MYC_LOG_INFO("I am log message number %d", digit);
    MYC_LOG_DEBUG("I have a lot of arguments: "MYC_FMT_BLUE("%s")", %d, 0x%08x and %p", "BLUE", 6, 1234567, main);
    MYC_LOG_TODO("Make a todo list");
    MYC_LOG("Add an extra line to the todo list.");
    MYC_LOG("End the todo list.");

    /* We only care about logging, so skip the actual abort step in the assertions. */
    #define abort() ;
    MYC_ASSERT(digit == 3, "Digit must be three, but is %d instead", digit);
    MYC_ASSERT(digit == 6, "Digit must be six, but is %d instead", digit);
    MYC_UNREACHABLE("For example we had a return right above this line.");
    #undef abort

    return 0;
}