#include "mytask.h"
#include "cmsis_os.h"
#include "oled0.96.h"

void StartDefaultTask(void const * argument)
{
	OledInit();
  for(;;)
  {
		OledShowInformation();
		OledRefreshGram();
    osDelay(1);
  }
}
