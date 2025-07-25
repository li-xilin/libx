#include "x/file.h"
#include "x/log.h"

int main(void)
{
	x_setmode_utf8(stderr);
	x_log_set_mode(X_LM_NOLOC | X_LM_NOTIME);

	x_log(X_LL_TRACE, "trace message");
	x_log(X_LL_DEBUG, "Debug message");
	x_log(X_LL_INFO, "Info message");
	x_log(X_LL_WARN, "Warn message");
	x_log(X_LL_ERROR, "Error message");
	x_log(X_LL_FATAL, "Fatal message");
	x_log(-42, "Fatal message with custom level -42");

	x_log_set_mode(X_LM_NOLOC);

	x_pinfo("Info message without location: %S", x_u("[test string]"));
	x_ptrace("Trace message without location");
	x_pdebug("Debug message without location");
	x_pwarn("Warn message without location");
	x_perror("Error message without location");
	x_pfatal("Fatal message without location");

	x_log_set_mode(X_LM_NOTIME);

	x_ptrace("Trace message without time");
	x_pdebug("Debug message without time");
	x_pinfo("Info message without time");
	x_pwarn("Warn message without time");
	x_perror("Error message without time");
	x_pfatal("Fatal message without time");

	x_log_set_mode(0);

	x_pdebug("Trace message with location and time");
	x_pdebug("Debug message with location and time");
	x_pinfo("Info message with location and time");
	x_pwarn("Warn message with location and time");
	x_perror("Error message with location and time");
	x_pfatal("Fatal message with location and time");

	x_log(X_LL_INFO, "打印uchar类型 %S", x_u("中文消息"));
	x_log(X_LL_INFO, "打印utf-8类型 %s", "中文消息");
}
