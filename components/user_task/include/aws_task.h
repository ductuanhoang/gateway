

#ifndef H_USER_AWS_TASK_
#define H_USER_AWS_TASK_
#include <stdint.h>
#include <stdbool.h>
#include <cstring>


#ifdef __cplusplus
extern "C"
{
#endif
    typedef void (*pSubCallBackHandler_t)(char *topicName, int payloadLen, char *payLoad);
    int aws_iot_init(void);
    int aws_publish(const char *pubTopic, const char *pubPayLoad, int payLoadLen);
    int aws_subscribe(const char *subTopic, pSubCallBackHandler_t pSubCallBackHandler);
    bool aws_isConnected(void);
#ifdef __cplusplus
}
#endif

#endif // H_USER_AWS_TASK_
