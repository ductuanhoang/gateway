#ifndef AWS_IOT_SDK_SRC_IOT_SHADOW_JSON_H_
#define AWS_IOT_SDK_SRC_IOT_SHADOW_JSON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "aws_iot_error.h"
#include "aws_iot_shadow_json_data.h"

bool isJsonValidAndParse(const char *pJsonDocument, size_t jsonSize, void *pJsonHandler, int32_t *pTokenCount);

bool isJsonKeyMatchingAndUpdateValue(const char *pJsonDocument, void *pJsonHandler, int32_t tokenCount,
									 jsonStruct_t *pDataStruct, uint32_t *pDataLength, int32_t *pDataPosition);

IoT_Error_t aws_iot_shadow_internal_get_request_json(char *pBuffer, size_t bufferSize);

IoT_Error_t aws_iot_shadow_internal_delete_request_json(char *pBuffer, size_t bufferSize);

void resetClientTokenSequenceNum(void);


bool isReceivedJsonValid(const char *pJsonDocument, size_t jsonSize);

bool extractClientToken(const char *pJsonDocument, size_t jsonSize, char *pExtractedClientToken, size_t clientTokenSize);

bool extractVersionNumber(const char *pJsonDocument, void *pJsonHandler, int32_t tokenCount, uint32_t *pVersionNumber);

#ifdef __cplusplus
}
#endif

#endif // AWS_IOT_SDK_SRC_IOT_SHADOW_JSON_H_
