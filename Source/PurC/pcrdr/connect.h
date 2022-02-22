/**
 * @file connect.h
 * @date 2022/02/22
 * @brief The internal interfaces to manage renderer connection.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * Authors:
 *  Vincent Wei (<https://github.com/VincentWei>), 2021, 2022
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PURC_PCRDR_CONN_H
#define PURC_PCRDR_CONN_H

#include <time.h>

#include "purc-pcrdr.h"
#include "private/kvlist.h"

struct pcrdr_conn {
    int prot;
    int type;
    int fd;
    int last_ret_code;

    char* srv_host_name;
    char* own_host_name;
    char* app_name;
    char* runner_name;

    struct kvlist call_list;

    void *user_data;

    pcrdr_request_handler request_handler;

    pcrdr_event_handler event_handler;

    /* the current pending request identifier */
    char *pending_request_id;
    pcrdr_response_handler response_handler;
    time_t time_expected;

    /* operations */
    int (*wait_message) (pcrdr_conn* conn, int timeout_ms);
    pcrdr_msg *(*read_message) (pcrdr_conn* conn);
    int (*send_message) (pcrdr_conn* conn, const pcrdr_msg *msg);
    int (*ping_peer) (pcrdr_conn* conn);
    int (*disconnect) (pcrdr_conn* conn);
};

#endif  /* PURC_PCRDR_CONN_H */
