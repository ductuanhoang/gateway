#ifndef SRC_SHADOW_AWS_IOT_SHADOW_ACTIONS_H_
#define SRC_SHADOW_AWS_IOT_SHADOW_ACTIONS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "aws_iot_shadow_interface.h"

IoT_Error_t aws_iot_shadow_internal_action(const char *pThingName, ShadowActions_t action,
										   const char *pJsonDocumentToBeSent, size_t jsonSize, fpActionCallback_t callback,
										   void *pCallbackContext, uint32_t timeout_seconds, bool isSticky);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SHADOW_AWS_IOT_SHADOW_ACTIONS_H_ */
