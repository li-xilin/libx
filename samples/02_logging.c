#include <stdio.h>
#include "x/log.h"

int main()
{
	x_log_set_mode(X_LM_NOLOC | X_LM_NOTIME);

	x_log(X_LL_TRACE, "trace message");
	x_log(X_LL_DEBUG, "Debug message");
	x_log(X_LL_INFO, "Info message");
	x_log(X_LL_WARN, "Warn message");
	x_log(X_LL_ERROR, "Error message");
	x_log(X_LL_FATAL, "Fatal message");
	x_log(-42, "Fatal message with custom level -42");

	x_log_set_mode(X_LM_NOLOC);

	x_pinfo("Info message with location: %s", "[test string]");
	x_ptrace("Trace message with location");
	x_pdebug("Debug message with location");
	x_pwarn("Warn message with location");
	x_perror("Error message with location");
	x_pfatal("Fatal message with location");

	x_log_set_mode(X_LM_NOTIME);

	x_ptrace("Trace message with with");
	x_pdebug("Debug message with time");
	x_pinfo("Info message with time");
	x_pwarn("Warn message with time");
	x_perror("Error message with time");
	x_pfatal("Fatal message with time");

	x_log_set_mode(0);

	x_pdebug("Trace message with location and time");
	x_pdebug("Debug message with location and time");
	x_pinfo("Info message with location and time");
	x_pwarn("Warn message with location and time");
	x_perror("Error message with location and time");
	x_pfatal("Fatal message with location and time");
}
