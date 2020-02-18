/*
Cubesat Space Protocol - A small network-layer protocol designed for Cubesats
Copyright (C) 2012 GomSpace ApS (http://www.gomspace.com)
Copyright (C) 2012 AAUSAT3 Project (http://aausat3.space.aau.dk)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <csp/csp.h>

#include "csp/interfaces/csp_if_can.h"
/* Using un-exported header file.
 * This is allowed since we are still in libcsp */
#include <csp/arch/csp_thread.h>

/** Example defines */
#define SATB_ADDR  2			// Address of SATB CSP node
#define SATC_ADDR  3			// Address of SATC CSP node
#define SATA_ADDR  4                    // Address of SATA CSP node

#define MY_PORT		10			// Port to send test traffic to



CSP_DEFINE_TASK(task_server) {

	/* Create socket without any socket options */
	csp_socket_t *sock = csp_socket(CSP_SO_NONE);

	/* Bind all ports to socket */
	csp_bind(sock, CSP_ANY);

	/* Create 10 connections backlog queue */
	csp_listen(sock, 10);

	/* Pointer to current connection and packet */
	csp_conn_t *conn;
	csp_packet_t *packet;

	/* Process incoming connections */
	while (1) {

		/* Wait for connection, 10000 ms timeout */
		if ((conn = csp_accept(sock, 10000)) == NULL)
			continue;

		/* Read packets. Timout is 100 ms */
		while ((packet = csp_read(conn, 100)) != NULL) {
			switch (csp_conn_dport(conn)) {
			case MY_PORT:
				/* Process packet here */
				printf("Packet received on MY_PORT: %s\r\n", (char *) packet->data);
				csp_buffer_free(packet);
				break;

			default:
				/* Let the service handler reply pings, buffer use, etc. */
				csp_service_handler(conn, packet);
				break;
			}
		}

		/* Close current connection, and handle next */
		csp_close(conn);

	}

	return CSP_TASK_RETURN;

}

int main(int argc, char * argv[]) {

	/**
	 * Initialise CSP,
	 * No physical interfaces are initialised in this example,
	 * so only the loopback interface is registered.
	 */

	/* Init buffer system with 10 packets of maximum 300 bytes each */
	printf("Initialising CSP\r\n");

	/* CAN configuration struct for SocketCAN interface "can0" */
	struct csp_can_config can_conf = {.bitrate = 1000000, .ifc = "can0"};

	csp_buffer_init(10, 300);

	/* Init CSP with address SATB_ADDR */
	csp_init(SATB_ADDR);

	/* Init the CAN interface with hardware filtering */
	csp_can_init(CSP_CAN_PROMISC, &can_conf);

	/* Setup default route to CAN interface */
	csp_route_set(SATC_ADDR, &csp_if_can, CSP_NODE_MAC);
        csp_route_set(SATA_ADDR, &csp_if_can, CSP_NODE_MAC);
	csp_route_set(CSP_BROADCAST_ADDR, &csp_if_can, CSP_NODE_MAC);


	/* Start router task with 500 word stack, OS task priority 1 */
	csp_route_start_task(500, 1);

	/* Enable debug output from CSP */
	if ((argc > 1) && (strcmp(argv[1], "-v") == 0)) {
		printf("Debug enabed\r\n");
		csp_debug_toggle_level(3);
		csp_debug_toggle_level(4);

		printf("Conn table\r\n");
		csp_conn_print_table();

		printf("Route table\r\n");
		csp_route_print_table();

		printf("Interfaces\r\n");
		csp_route_print_interfaces();

	}

	/**
	 * Initialise example threads, using pthreads.
	 */

	/* Server */
	printf("Starting Server task\r\n");
	csp_thread_handle_t handle_server;
	csp_thread_create(task_server, "SERVER", 1000, NULL, 0, &handle_server);


	/* Wait for execution to end (ctrl+c) */
	while(1) {
		csp_sleep_ms(100000);
	}

	return 0;

}
