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

#endif /* __NRF_NFC_LOG_H__ */
