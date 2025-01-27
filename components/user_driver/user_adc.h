/*
 * template.h
 *
 *  Created on:
 *      Author:
 */

#ifndef TEMPLATE_H_
#define TEMPLATE_H_

#ifdef __cplusplus
extern "C"
{
#endif

    /****************************************************************************/
    /***        Include files                                                 ***/
    /****************************************************************************/
    #include <stdint.h>
    /****************************************************************************/
    /***        Macro Definitions                                             ***/
    /****************************************************************************/

    /****************************************************************************/
    /***        Type Definitions                                              ***/
    /****************************************************************************/

    /****************************************************************************/
    /***         Exported global functions                                     ***/
    /****************************************************************************/
    void adc_init(void);

    uint32_t adc_get_voltage_value(void);

#ifdef __cplusplus
}
#endif
#endif /* TEMPLATE_H_ */