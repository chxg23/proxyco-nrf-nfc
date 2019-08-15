/*
 * nrf_nfc.c
 *
 *  Created on: July 26, 2019
 *      Author: Jeremy Wood <jeremy@proxy.com>
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <os/os.h>
#include <os/os_callout.h>

#include <util/util.h>

#include "nrf_nfct.h"

#include "nrf-nfc/nrf_nfc.h"
#include "nrf-nfc/nrf_nfc_log.h"
#include "nfc_t4t_lib.h"
#include "nfc_uri_msg.h"

#define NRF_NFC_ERROR_MAX (10)

#define NDEF_BUF_SIZE (256)
static uint8_t ndef_buffer[NDEF_BUF_SIZE];

static const uint8_t url[] = MYNEWT_VAL(NRF_NFC_DEFAULT_TAG_URI);

/* Local variable to track whether NFC tag emulation is active. */
static bool nfc_emulation_on = false;

static struct os_callout tag_present_complete_callout;

/* Raw UID data buffer. Up to NRF_NFC_UID_MAX_LEN (10) bytes, this field holds the UID credential
 * while it's being presented. Data is zero-padded, right-aligned, little-endian in the current
 * UID field-size (4, 7, or 10 bytes).
 *
 * For example, 0xbaadbeef would look like this:
 *
 * ef be ad ba 00 00 00 00 00 00
 */
static uint8_t uid_buf[NRF_NFC_UID_MAX_LEN] = {0};
static nrf_nfct_sensres_nfcid1_size_t uid_size_class = NRF_NFCT_SENSRES_NFCID1_SIZE_SINGLE;

static void
nfc_tag_present_complete_cb(struct os_event *ev)
{
  NRF_NFC_LOG(INFO, "nrf-nfc: nfc_tag_present_complete_cb()\n");
  nrf_nfc_emulation_stop();
}

int
nrf_nfc_set_tag_uri(uint8_t *uri_data, uint8_t uri_data_len)
{
  ret_code_t rc;
  uint32_t buf_len = sizeof(ndef_buffer);
  bool renable_emulation = false;

  assert(uri_data != NULL);

  if (nfc_emulation_on) {
    rc = nfc_t4t_emulation_stop();
    if (rc != NRF_SUCCESS) {
      NRF_NFC_LOG(INFO, "nrf-nfc: nfc_t4t_emulation_stop() failed, rc=%d\n");
      return rc;
    }
    renable_emulation = true;
  }

  rc = nfc_uri_msg_encode(NFC_URI_HTTPS, uri_data, uri_data_len, ndef_buffer, &buf_len);
  if (rc != 0) {
    NRF_NFC_LOG(INFO, "nrf-nfc: nfc_uri_msg_encode() failed, rc=%d\n");
    return rc;
  }

  rc = nfc_t4t_ndef_rwpayload_set(ndef_buffer, sizeof(ndef_buffer));
  if (rc != NRF_SUCCESS) {
    NRF_NFC_LOG(INFO, "nrf-nfc: nfc_t4t_ndef_rwpayload_set() failed, rc=%d\n");
    return rc;
  }

  if (renable_emulation) {
    rc = nfc_t4t_emulation_start();
    if (rc != NRF_SUCCESS) {
      NRF_NFC_LOG(INFO, "nrf-nfc: nfc_t4t_emulation_start() failed, rc=%d\n");
      return rc;
    }
  }

  return rc;
}

int
nrf_nfc_set_tag_uid(uint8_t *uid_data, uint8_t uid_data_len)
{
  int i, j;
  uint8_t uid_field_len;

  assert(uid_data != NULL);

  NRF_NFC_LOG(INFO, "nrf-nfc: nrf_nfc_set_tag_uid(), uid_data_len=%d, uid_data=");
  for (i = 0; i < uid_data_len; i++) {
    printf("%02x", uid_data[i]);
  }
  printf(".\n");

  if (uid_data_len > NRF_NFC_UID_MAX_LEN) {
    NRF_NFC_LOG(INFO, "nrf-nfc: nrf_nfc_set_tag_uid(), uid_data_len=%d > NRF_NFC_UID_MAX_LEN!, uid_data_len\n");
    return -1;
  }

  memset(uid_buf, 0, sizeof(uid_buf));

  /* Set the NFC UID size class and field length based on uid_data_len. */
  if (uid_data_len <= NRF_NFC_UID_SINGLE_4_BYTE) {
    uid_size_class = NRF_NFCT_SENSRES_NFCID1_SIZE_SINGLE;
    uid_field_len = NRF_NFC_UID_SINGLE_4_BYTE;
  } else if (uid_data_len <= NRF_NFC_UID_DOUBLE_7_BYTE) {
    uid_size_class = NRF_NFCT_SENSRES_NFCID1_SIZE_DOUBLE;
    uid_field_len = NRF_NFC_UID_DOUBLE_7_BYTE;
  } else {
    uid_size_class = NRF_NFCT_SENSRES_NFCID1_SIZE_TRIPLE;
    uid_field_len = NRF_NFC_UID_TRIPLE_10_BYTE;
  }

  /* UID byte-order is little-endian, credentials are sent big-endian. */
  j = uid_field_len - 1;
  for (i = 0; i < uid_data_len && i < uid_field_len; i++) {
    uid_buf[j] = uid_data[i];
    j--;
  }

  return 0;
}

int
nrf_nfc_emulation_start(void)
{
  ret_code_t rc;
  static uint8_t err_count = 0;

  NRF_NFC_LOG(INFO, "nrf-nfc: nrf_nfc_emulation_start(), uid_size_class=%d, uid_buf=");
  for (int i = 0; i < NRF_NFC_UID_MAX_LEN; i++) {
    printf("%02x", uid_buf[i]);
  }
  printf(".\n");

  /* We have to set the UID each time that we turn the emulation on. It gets wiped out when it's
   * turned off. */
  nrf_nfct_nfcid1_set(uid_buf, uid_size_class);

  rc = nfc_t4t_emulation_start();

  if (rc != NRF_SUCCESS) {
    NRF_NFC_LOG(INFO, "nrf-nfc: nrf_nfc_emulation_start(), failed, rc=%d!\n", rc);
    err_count++;
    assert(err_count < NRF_NFC_ERROR_MAX);
  } else {
    nfc_emulation_on = true;
  }

  return rc;
}

int
nrf_nfc_emulation_stop(void)
{
  ret_code_t rc;
  static uint8_t err_count = 0;

  rc = nfc_t4t_emulation_stop();

  if (rc != NRF_SUCCESS) {
    NRF_NFC_LOG(INFO, "nrf-nfc: nfc_t4t_emulation_stop(), failed, rc=%d!\n", rc);
    err_count++;
    assert(err_count < NRF_NFC_ERROR_MAX);
  } else {
    nfc_emulation_on = false;
    /* This makes explicit what happens in the hardware—the UID goes away when the tag emulation
    * stops. We clear it here to make that explicit, and to make it so that we don't inadvertently
    * deliver the same credential again. */
    memset(uid_buf, 0, sizeof(uid_buf));
  }

  return rc;
}

/* We can respond to NFC events here. Currently just a place holder, as library requires a function
 * to be defined. */
static void
nfc_event_callback(void *p_context, nfc_t4t_event_t event, const uint8_t *p_data,
    size_t data_length, uint32_t flags)
{
#if MYNEWT_VAL(DEBUG_BUILD)
  NRF_NFC_LOG(DEBUG, "nrf-nfc: nfc_event_callback() event=%d, data_length=%d, flags=%d, p_data=",
      event, data_length, flags);
  if (data_length > 0 && p_data != NULL) {
    for (int i = 0; i < data_length; i++) {
      printf("%02x", p_data[i]);
    }
  }
  printf(".\n");
#endif
}

void
nrf_nfc_present_tag(uint16_t duration_ms)
{
  NRF_NFC_LOG(INFO, "nrf-nfc: nrf_nfc_present_tag()\n");

  nrf_nfc_emulation_start();

  os_callout_reset(&tag_present_complete_callout, ms_to_os_ticks(duration_ms));
}

void
nrf_nfc_pkg_init(void)
{
  ret_code_t rc;

  NRF_NFC_LOG(INFO, "nrf-nfc: nrf_nfc_pkg_init()\n");

  os_callout_init(&tag_present_complete_callout, os_eventq_dflt_get(),
      nfc_tag_present_complete_cb, NULL);

  rc = nfc_t4t_setup(nfc_event_callback, NULL);
  assert(rc == NRF_SUCCESS);

  /* Set empty initial UID, default size. */
  nrf_nfct_nfcid1_set(uid_buf, uid_size_class);

  /* Initialize tag with default URI. */
  rc = nrf_nfc_set_tag_uri((uint8_t *)url, sizeof(url));
  if (rc != 0) {
    NRF_NFC_LOG(INFO, "nrf-nfc: nrf_nfc_set_tag_uri() failed, rc=%d\n");
  }
}
