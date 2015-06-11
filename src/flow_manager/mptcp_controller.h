/* File: mptcp_control.h */

#ifndef __MPTCP_CONTROLLER__
#define __MPTCP_CONTROLLER__

#include <stdint.h>

#include "mptcp_state.h"

int mptcp_create_subflow(
	struct mptcp_state *mp_state,
	uint32_t src_ip,
	uint16_t src_locid,
	struct connection *conn);

int mptcp_remove_subflow(
	struct mptcp_state *mp_state,
	uint32_t src_ip,
	uint16_t src_locid,
	struct connection *conn);

void * mptcp_control_start(void *data);

#endif

/* end file: mptcp_control.h */
