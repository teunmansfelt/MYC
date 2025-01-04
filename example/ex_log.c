#include "myc/log.h"

int main(void)
{
    MYC_LOG_TRACE("I tell you were the error below comes from :)");
    MYC_LOG_ERROR("I am an error!");
    MYC_LOG_WARN("I am a warning.");
    MYC_LOG_INFO("I am log message number %d", 3);
    MYC_LOG_DEBUG("I have a lot of arguments: "MYC_FMT_BLUE("%s")", %d, 0x%08x and %p", "BLUE", 6, 1234567, main);
    MYC_LOG_TODO("Make a todo list");
    MYC_LOG("Add an extra line to the todo list.");
    MYC_LOG("End the todo list.");

    return 0;
}