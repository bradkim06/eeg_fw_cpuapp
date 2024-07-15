#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MAIN, CONFIG_APP_LOG_LEVEL);

int main(void)
{
	LOG_INF("Hello Main");
	return 0;
}
