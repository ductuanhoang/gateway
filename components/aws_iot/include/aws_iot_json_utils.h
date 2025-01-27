#ifndef AWS_IOT_SDK_SRC_JSON_UTILS_H_
#define AWS_IOT_SDK_SRC_JSON_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "aws_iot_error.h"
#include "jsmn.h"

// utility functions
/**
 * @brief          JSON Equality Check
 *
 * Given a token pointing to a particular JSON node and an
 * input string, check to see if the key is equal to the string.
 *
 * @param json      json string
 * @param tok     	json token - pointer to key to test for equality
 * @param s			input string for key to test equality
 *
 * @return         	0 if equal, 1 otherwise
 */
int8_t jsoneq(const char *json, jsmntok_t *tok, const char *s);

/**
 * @brief          Parse a signed 32-bit integer value from a JSON node.
 *
 * Given a JSON node parse the integer value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param i				address of int32_t to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseInteger32Value(int32_t *i, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse a signed 16-bit integer value from a JSON node.
 *
 * Given a JSON node parse the integer value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param i				address of int16_t to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseInteger16Value(int16_t *i, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse a signed 8-bit integer value from a JSON node.
 *
 * Given a JSON node parse the integer value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param i				address of int8_t to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseInteger8Value(int8_t *i, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse an unsigned 32-bit integer value from a JSON node.
 *
 * Given a JSON node parse the integer value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param i				address of uint32_t to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseUnsignedInteger32Value(uint32_t *i, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse an unsigned 16-bit integer value from a JSON node.
 *
 * Given a JSON node parse the integer value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param i				address of uint16_t to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseUnsignedInteger16Value(uint16_t *i, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse an unsigned 8-bit integer value from a JSON node.
 *
 * Given a JSON node parse the integer value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param i				address of uint8_t to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseUnsignedInteger8Value(uint8_t *i, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse a float value from a JSON node.
 *
 * Given a JSON node parse the float value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param f				address of float to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseFloatValue(float *f, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse a double value from a JSON node.
 *
 * Given a JSON node parse the double value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param d				address of double to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseDoubleValue(double *d, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse a boolean value from a JSON node.
 *
 * Given a JSON node parse the boolean value from the value.
 *
 * @param jsonString	json string
 * @param tok     		json token - pointer to JSON node
 * @param b				address of boolean to be updated
 *
 * @return         		SUCCESS - success
 * @return				JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseBooleanValue(bool *b, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Parse a string value from a JSON node.
 *
 * Given a JSON node parse the string value from the value.
 *
 * @param buf           address of string to be updated
 * @param bufLen        length of buf in bytes
 * @param jsonString    json string
 * @param token         json token - pointer to JSON node
 *
 * @return              SUCCESS - success
 * @return              JSON_PARSE_ERROR - error parsing value
 */
IoT_Error_t parseStringValue(char *buf, size_t bufLen, const char *jsonString, jsmntok_t *token);

/**
 * @brief          Find the JSON node associated with the given key in the given object.
 *
 * Given a JSON node parse the string value from the value.
 *
 * @param key			json key
 * @param token 		json token - pointer to JSON node
 * @param jsonString 	json string
 *
 * @return 				pointer to found property value
 * @return 				NULL - not found
 */
jsmntok_t *findToken(const char *key, const char *jsonString, jsmntok_t *token);

#ifdef __cplusplus
}
#endif

#endif /* AWS_IOT_SDK_SRC_JSON_UTILS_H_ */
