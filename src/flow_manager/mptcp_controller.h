/* File: mptcp_control.h */

#ifndef __MPTCP_CONTROLLER__
#define __MPTCP_CONTROLLER__

#include <stdint.h>

#include "mptcp_state.h"

typedef enum {
	MPTCP_NEW_FLOW = 0x01,
	MPTCP_REM_FLOW = 0x02,
	MPTCP_NEW_ADDR = 0x03,
	MPTCP_REM_ADDR = 0x04,
} mptcp_event_codes;

int create_subflow(
	struct mptcp_state *mp_state,
	uint32_t src_ip,
	uint16_t src_locid,
	struct connection *conn);

int remove_subflow(
	struct mptcp_state *mp_state,
	uint32_t src_ip,
	uint16_t src_locid,
	struct connection *conn);

void * mptcp_control_start(void *data);

#endif

/* end file: mptcp_control.h */
