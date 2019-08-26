/*
 * nrf-nfc-t4t.h
 *
 *  Created on: July 24, 2019
 *      Author: Jeremy Wood (jeremy@proxy.com)
 */
#ifndef __NRF_NFC_H__
#define __NRF_NFC_H__

#include <stdint.h>

/* The NFC UID can be one of three different sizes: Either 4, 7 or 10 bytes. */
enum {
  NRF_NFC_UID_SINGLE_4_BYTE = 4,
  NRF_NFC_UID_DOUBLE_7_BYTE = 7,
  NRF_NFC_UID_TRIPLE_10_BYTE = 10
};

/* The maximum NFC UID length is NRF_NFC_UID_TRIPLE_10_BYTE (10) bytes. */
#define NRF_NFC_UID_MAX_LEN (NRF_NFC_UID_TRIPLE_10_BYTE)

/**
 * Set NFC Tag URI.
 *
 * @param uri_data        Pointer to URI data
 * @param uri_data_len    Length of URI data in bytes
 *
 * @returns 0 on success, non-zero on failure.
 */
int nrf_nfc_set_tag_uri(uint8_t *uri_data, uint8_t uri_data_len);

/**
 * Set NFC Tag UID.
 *
 * @param uid_data        Pointer to UID data
 * @param uid_data_len    Length of UID data in bytes
 *
 * @returns 0 on success, non-zero on failure.
 */
int nrf_nfc_set_tag_uid(uint8_t *uid_data, uint8_t uid_data_len);

/**
 * Start NFC Tag Emulation
 *
 * @returns 0 on success, non-zero on failure.
 */
int nrf_nfc_emulation_start(void);

/**
 * Stop NFC Tag Emulation
 *
 * @returns 0 on success, non-zero on failure.
 */
int nrf_nfc_emulation_stop(void);

/**
 * Present NFC Tag for duration_ms.
 *
 * @param duration_ms    Time in milliseconds to present tag for reading.
 *
 * @returns 0 on success, non-zero on failure.
 */
int nrf_nfc_present_tag(uint16_t duration_ms);

#endif /* __NRF_NFC_H__ */