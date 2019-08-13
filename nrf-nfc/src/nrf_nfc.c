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

#define NDEF_BUF_SIZE (256)
static uint8_t ndef_buffer[NDEF_BUF_SIZE];

static const uint8_t url[] = "proxy.com";

static bool nfc_emulation_on = false;

static struct os_callout tag_present_complete_callout;

/* Globals for UID and UID size. */
static uint8_t uid_buf[NRF_NFC_UID_MAX_LEN] = {0};
static nrf_nfct_sensres_nfcid1_size_t uid_size = NRF_NFCT_SENSRES_NFCID1_SIZE_SINGLE;

static void
nfc_tag_present_complete_cb(struct os_event *ev)
{
  NRF_NFC_LOG(INFO, "nrf-nfc: nfc_tag_present_complete_cb()\n");
  nrf_nfc_emulation_stop();
}

void
nrf_nfc_set_tag_uri(uint8_t *uri_data, uint8_t uri_data_len)
{
  ret_code_t rc;
  uint32_t buf_len = sizeof(ndef_buffer);
  bool renable_emulation = false;

  assert(uri_data != NULL);

  if (nfc_emulation_on) {
    rc = nfc_t4t_emulation_stop();
    assert(rc == NRF_SUCCESS);
    renable_emulation = true;
  }

  rc = nfc_uri_msg_encode(NFC_URI_HTTPS, uri_data, uri_data_len, ndef_buffer, &buf_len);
  assert(rc == NRF_SUCCESS);

  rc = nfc_t4t_ndef_rwpayload_set(ndef_buffer, sizeof(ndef_buffer));
  assert(rc == NRF_SUCCESS);

  if (renable_emulation) {
    rc = nfc_t4t_emulation_start();
    assert(rc == NRF_SUCCESS);
  }
}

void
nrf_nfc_set_tag_uid(uint8_t *uid_data, uint8_t uid_data_len)
{
  int i, j;

  assert(uid_data != NULL);
  assert(uid_data_len <= NRF_NFC_UID_MAX_LEN);

  memset(uid_buf, 0, sizeof(uid_buf));

  /* UID byte-order is backwards. */
  j = NRF_NFC_UID_MAX_LEN - 1;
  for (i = 0; i < uid_data_len && i < NRF_NFC_UID_MAX_LEN; i++) {
    uid_buf[j] = uid_data[i];
    j--;
  }

  /* Set the NFC UID size class. */
  if (uid_data_len <= NRF_NFC_UID_SINGLE_4_BYTE) {
    uid_size = NRF_NFCT_SENSRES_NFCID1_SIZE_SINGLE;
  } else if (uid_data_len <= NRF_NFC_UID_DOUBLE_7_BYTE) {
    uid_size = NRF_NFCT_SENSRES_NFCID1_SIZE_DOUBLE;
  } else {
    uid_size = NRF_NFCT_SENSRES_NFCID1_SIZE_TRIPLE;
  }
}

void
nrf_nfc_emulation_start(void)
{
  ret_code_t rc;

  /* We have to set the UID each time that we turn the emulation on. It gets wiped out when it's
   * turned off. */
  nrf_nfct_nfcid1_set(uid_buf, uid_size);

  rc = nfc_t4t_emulation_start();
  assert(rc == NRF_SUCCESS);
  nfc_emulation_on = true;
}

void
nrf_nfc_emulation_stop(void)
{
  ret_code_t rc;
  rc = nfc_t4t_emulation_stop();
  assert(rc == NRF_SUCCESS);

  nfc_emulation_on = false;

  /* This makes explicit what happens in the hardware—the UID goes away when the tag emulation
   * stops. We clear it here to make that explicit, and to make it so that we don't inadvertently
   * deliver the same credential again. */
  memset(uid_buf, 0, sizeof(uid_buf));
}

/* We can respond to NFC events here. Currently just a place holder, as library requires a function
 * to be defined. */
static void
nfc_event_callback(void *p_context, nfc_t4t_event_t event, const uint8_t *p_data,
    size_t data_length, uint32_t flags)
{
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
  nrf_nfct_nfcid1_set(uid_buf, uid_size);

  /* Initialize tag with default URI. */
  nrf_nfc_set_tag_uri((uint8_t *)url, sizeof(url));
}
