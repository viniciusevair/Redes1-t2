#include <algorithm>
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <bits/types/struct_timeval.h>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ifaddrs.h>
#include <iostream>
#include <netinet/in.h>
#include <ostream>
#include <string>
#include <sys/socket.h>
#include <unordered_map>

#include "ring_node.hpp"
#include "rn_macros.hpp"

RingNode::RingNode (int id, std::string &if_name, std::string &next_id_ip,
                    std::string &prev_id_ip)
    : id (id), next_id ((id + 1) % NODE_QTT), prev_id ((id + 3) % NODE_QTT),
      next_id_ip (next_id_ip), prev_id_ip (prev_id_ip) {

    std::string inet_addr_str = get_ipv4_addr (if_name);
    addr_len                  = sizeof (struct sockaddr_in);

    socket_fd = socket (AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
        handle_error (ERR_SCKFD_DEF);

    memset (&node_addr, 0, sizeof (node_addr));
    memset (buffer, 0, sizeof (BUFFER_SIZE));
    memset (buffer_data, 0, sizeof (DATA_SIZE));

    node_addr.sin_family = AF_INET;
    node_addr.sin_port   = htons (BASE_PORT + id);
    if (inet_pton (AF_INET, inet_addr_str.c_str (), &node_addr.sin_addr) <= 0)
        handle_error (ERR_INVALID_ADDRESS);

    if (bind (socket_fd, (const struct sockaddr *)&node_addr,
              sizeof (node_addr)) < 0)
        handle_error (ERR_SCKFD_BIND);

    initialize_neighbour_addresses ();
}

std::string
RingNode::get_ipv4_addr (const std::string &if_name) {
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];
    const char *interface_name = if_name.c_str ();

    if (getifaddrs (&ifaddr) == -1) {
        handle_error (ERR_GET_IFADDR);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET && strcmp (ifa->ifa_name, interface_name) == 0) {
            s = getnameinfo (ifa->ifa_addr, sizeof (struct sockaddr_in), host,
                             NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                handle_error (ERR_GET_NAMEINFO);
            }

            freeifaddrs (ifaddr);
            return host;
        }
    }

    freeifaddrs (ifaddr);
    handle_error (ERR_GET_IFADDR);
    return "";
}

void
RingNode::initialize_neighbour_addresses () {
    memset (&next_addr, 0, sizeof (next_addr));
    memset (&prev_addr, 0, sizeof (prev_addr));

    next_addr.sin_family      = AF_INET;
    next_addr.sin_port        = htons (BASE_PORT + next_id);
    next_addr.sin_addr.s_addr = inet_addr (next_id_ip.c_str ());

    prev_addr.sin_family      = AF_INET;
    prev_addr.sin_port        = htons (BASE_PORT + prev_id);
    prev_addr.sin_addr.s_addr = inet_addr (prev_id_ip.c_str ());
}

void
RingNode::forward_receipt (bool success) {
    char aux = buffer[DEST_OFFSET];
    buffer[DEST_OFFSET]        = buffer[SRC_OFFSET];
    buffer[SRC_OFFSET]         = aux;
    buffer[OPCODE_OFFSET]      = OPC_RECEIPT;
    buffer[FRAME_STATE_OFFSET] = success ? FRAME_RECEIVED : FRAME_RESET;
    set_buffer_data ("");
    forward_frame ();
}

void
RingNode::forward_frame (bool log_message) {
    compute_crc ();
    sendto (socket_fd, buffer, BUFFER_SIZE, 0,
            (const struct sockaddr *)&next_addr, sizeof (next_addr));

    if (log_message) {
        std::cout << "Forwarded frame: '" << buffer + DATA_OFFSET << "' to "
                  << inet_ntop (AF_INET, &(next_addr.sin_addr), next_addr_str,
                                INET_ADDRSTRLEN)
                  << ":" << BASE_PORT + next_id << std::endl;
    }
}

bool
RingNode::check_integrity () {
    return compute_crc (true);
}

bool
RingNode::compute_crc (bool is_dest) {
    int len  = BUFFER_SIZE - 1; // Remove o byte de crc da conta
    char crc = 0xff;
    for (int i = 0; i < len; i++) {
        crc ^= buffer[i];
        for (int j = 0; j < 8; j++) {
            if ((crc & 0x80) != 0)
                crc = (crc << 1) ^ 0x49;
            else
                crc <<= 1;
        }
    }

    if (is_dest)
        return crc == buffer[CRC_OFFSET];

    buffer[CRC_OFFSET] = crc;
    return true;
}

void
RingNode::receive_frame (bool keep_listening) {
    do {
        int msg_len     = recvfrom (socket_fd, buffer, BUFFER_SIZE, 0,
                                    (struct sockaddr *)&prev_addr, &addr_len);
        buffer[msg_len] = '\0';

        if (!check_integrity ()) {
            std::cout << "CRC error" << std::endl;
            handle_error (ERR_CRC_FAIL);
        } else if (check_dest ()) {
            operation_decoder (buffer[OPCODE_OFFSET]);
            keep_listening = false;
        } else {
            forward_frame ();
        }
    } while (keep_listening);
}

bool
RingNode::check_dest () {
    bool is_dest = id == buffer[DEST_OFFSET];
    return is_dest;
}

int
RingNode::connect () {
    bool last_node      = id == 3;
    buffer[DEST_OFFSET] = (id + 1) % NODE_QTT;
    buffer[SRC_OFFSET]  = id;

    if (!strcmp (buffer + DATA_OFFSET, RING_LOCK)) {
        std::cout << "A rede em anel foi completamente estabelecida.\n"
                  << std::endl;
        if (!last_node)
            forward_frame ();
        return CONN_SUCCESS;
    }

    buffer[OPCODE_OFFSET] = OPC_NOP;

    if (id == 0)
        set_buffer_data ("0");
    sprintf (buffer + DATA_OFFSET + id, "%d", id);

    compute_crc ();

    forward_frame ();

    return CONN_FAIL;
}

int
RingNode::handle_error (const int err_code) {
    if (err_code == ERR_CRC_FAIL) {
        forward_receipt (FRAME_RESET);
        receive_frame ();
        return 1;
    }

    auto it = error_map.find (err_code);
    if (it != error_map.end ()) {
        std::cerr << "ERROR: " << it->second << std::endl;
        return it->first;
    }

    std::cerr << "ERROR: Unknown error happened." << std::endl;
    exit (EXIT_FAILURE);
}

void
RingNode::set_buffer_data (std::string data) {
    size_t data_size = std::min (data.size (), static_cast<size_t> (DATA_SIZE));
    memset (buffer + DATA_OFFSET, 0,
            DATA_SIZE); // Limpa o lixo da parte de dados
    memcpy (buffer + DATA_OFFSET, data.c_str (), data_size);
}

void
RingNode::operation_decoder (char opcode) {
    if (opcode == OPC_NOP)
        return;

    auto it = op_code_map.find (opcode);
    if (it == op_code_map.end ()) {
        handle_error (ERR_INVALID_OPCODE);
    }

    (this->*(it->second)) ();
}

void
RingNode::receive_message () {
    int src_id           = buffer[SRC_OFFSET];
    int distance         = (id - src_id + NODE_QTT) % NODE_QTT;
    int forwarding_count = NODE_QTT - distance - 1;

    bool is_broadcast = buffer[OPCODE_OFFSET] == OPC_BROADCAST;

    if (is_broadcast) {
        char saved_buffer_data[DATA_SIZE];
        memcpy (saved_buffer_data, buffer + DATA_OFFSET, DATA_SIZE);

        forward_receipt ();

        // Repassa o bastão a quantidade necessária para que os quatro nodos
        // recebam sua respectiva mensagem.
        for (int i = 0; i < forwarding_count; i++)
            receive_frame (false);

        memcpy (buffer_data, saved_buffer_data, DATA_SIZE);
    } else {
        memcpy (buffer_data, buffer + DATA_OFFSET, DATA_SIZE);
        forward_receipt ();
    }
}

void
RingNode::receive_coll_message () {
    memcpy (buffer_data, buffer + DATA_OFFSET, DATA_SIZE);
}

std::string
RingNode::get_buffer_data () {
    return std::string (buffer_data);
}

void
RingNode::send_message (std::string message, char dest_node,
                        bool is_broadcast) {
    set_buffer_data (message);
    if (dest_node == id) {
        memcpy (buffer_data, buffer, DATA_SIZE);
        return;
    }
    buffer[OPCODE_OFFSET] = is_broadcast ? OPC_BROADCAST : OPC_MSG;
    buffer[DEST_OFFSET]   = dest_node;
    buffer[SRC_OFFSET]    = id;

    memcpy (last_sent_buffer, buffer, BUFFER_SIZE);
    forward_frame ();
    receive_frame ();
}

void
RingNode::send_coll_message (std::string message) {
    buffer[OPCODE_OFFSET] = OPC_COLL_MSG;
    buffer[DEST_OFFSET]   = next_id;
    buffer[SRC_OFFSET]    = id;

    set_buffer_data (message);
    memcpy (last_sent_buffer, buffer, BUFFER_SIZE);
    forward_frame ();
}

void
RingNode::check_framestate () {
    auto state = buffer[FRAME_STATE_OFFSET];
    if (state != FRAME_RECEIVED) {
        memcpy (buffer, last_sent_buffer, BUFFER_SIZE);
        forward_frame ();
    } else {
        buffer[FRAME_STATE_OFFSET] = FRAME_RESET;
    }
}

void
RingNode::broadcast_message (std::string message) {
    for (int i = next_id; i != id; i = (i + 1) % NODE_QTT) {
        send_message (message, i, true);
    }

    std::cout << message.substr (0, message.find ("end_game")) << std::endl;
}
