#ifndef RINGNODE_H
#define RINGNODE_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>

#include "rn_macros.hpp"

class RingNode {
  public:
    RingNode (int id, std::string &if_name, std::string &next_id_ip,
              std::string &prev_id_ip);
    int connect ();
    void operation_decoder (char opcode);
    void send_message (std::string message, char dest_node,
                       bool is_broadcast = false);
    void send_coll_message (std::string message);
    void forward_frame (bool log_message = false);
    void receive_frame (bool keep_listening = true);
    std::string get_buffer_data ();
    void debug ();
    int get_next_id ();
    void broadcast_message (std::string message);
    void pass_token (char dest_node);

  private:
    std::string get_ipv4_addr (const std::string &if_name);

    unsigned int calculate_neighbour_addr (int neighbour_id);
    void initialize_neighbour_addresses ();

    void forward_receipt (bool success = true);

    void call_func ();

    void set_buffer_data (std::string data);

    void debug_message (char target);
    void receive_message ();
    void receive_coll_message ();

    bool check_integrity ();
    bool check_dest ();
    bool compute_crc (bool is_dest = false);

    void check_framestate ();

    int handle_error (const int err_code);

    const int id, next_id, prev_id;
    std::string next_id_ip, prev_id_ip;
    int socket_fd;
    struct sockaddr_in node_addr, next_addr, prev_addr;
    char buffer[BUFFER_SIZE];
    char buffer_data[DATA_SIZE];
    char last_sent_buffer[BUFFER_SIZE];
    char next_addr_str[INET_ADDRSTRLEN];
    char prev_addr_str[INET_ADDRSTRLEN];
    socklen_t addr_len;

    std::unordered_map<int, void (RingNode::*) ()> op_code_map = {
        {OPC_MSG,       &RingNode::receive_message     },
        {OPC_BROADCAST, &RingNode::receive_message     },
        {OPC_COLL_MSG,  &RingNode::receive_coll_message},
        {OPC_RECEIPT,   &RingNode::check_framestate    },
        {OPC_RESEND,    &RingNode::check_framestate    },
    };

    std::unordered_map<int, std::string> error_map = {
        {ERR_INVALID_ADDRESS, "Endereço inválido."               },
        {ERR_SCKFD_BIND,      "Falha ao vincular o socket."      },
        {ERR_SCKFD_DEF,       "Falha ao abrir o socket."         },
        {ERR_GET_IFADDR,      "Nenhuma interface foi encontrada."},
        {ERR_GET_NAMEINFO,    "Falha ao obter o nome do host."   },
    };
};

#endif // RINGNODE_H
