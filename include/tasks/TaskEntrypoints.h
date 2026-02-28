#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

   void inputTask(void *pvParameters);
   void controlTask(void *pvParameters);
   void commandTask(void *pvParameters);
   void networkTask(void *pvParameters);

#ifdef __cplusplus
}
#endif