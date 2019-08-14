/*
 * nrf_nfc_log.h
 *
 *  Created on: July 24, 2019
 *  Author: Jeremy Wood (jeremy@proxy.com)
 */

#ifndef __NRF_NFC_LOG_H__
#define __NRF_NFC_LOG_H__

#include <modlog/modlog.h>

#define NRF_NFC_LOG(lvl_, ...) MODLOG_ ## lvl_(MYNEWT_VAL(NRF_NFC_LOG_MODULE), __VA_ARGS__)

/* Aliases for compatibility with included Nordic SDK files. */
#define NRF_LOG_ERROR(...)                     NRF_NFC_LOG(ERROR, __VA_ARGS__)
#define NRF_LOG_WARNING(...)                   NRF_NFC_LOG(WARN, __VA_ARGS__)
#define NRF_LOG_INFO(...)                      NRF_NFC_LOG(INFO, __VA_ARGS__)
#define NRF_LOG_DEBUG(...)                     NRF_NFC_LOG(DEBUG, __VA_ARGS__)

#define NRF_LOG_HEXDUMP_INFO(...)              // Not Implemented
#define NRF_LOG_HEXDUMP_DEBUG(...)             // Not Implemented

#endif /* __NRF_NFC_LOG_H__ */
