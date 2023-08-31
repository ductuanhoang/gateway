/**
 * @file user_console.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-06-28
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef USER_CONSOLE_H_
#define USER_CONSOLE_H_

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
#define arg_int_to_value(src_msg, dst_msg, message) do { \
    if (src_msg->count != 0) {\
        dst_msg = src_msg->ival[0];\
    } \
} while(0)

/****************************************************************************/
/***         Exported global functions                                     ***/
/****************************************************************************/
void user_console_init(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_CONSOLE_H_ */

