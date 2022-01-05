#ifndef __SYSTEM_CFG_H
#define __SYSTEM_CFG_H


#define SERIAL_RECEIVE_USING_POLLING_MODE               1
#define   DBG_ENABLE                                    1
#if (DBG_ENABLE)
#define   DBG_PRINT                                     Serial           
#else
#define   DBG_PRINT                                     //Serial
#endif
#endif
