#include "player.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

constexpr int PLAYER_QTT      = 4;
constexpr int ONE_SURVIVOR    = 3;
constexpr bool KEEP_LISTENING = true;
constexpr bool KEEP_PLAYING   = true;

Player::Player (int id, std::string &if_name, std::string &next_id_ip,
                std::string &prev_id_ip)
    : node (id, if_name, next_id_ip, prev_id_ip), id (id), dead (false), guesses (4), plays (4), hp (4, 5) {
    dealer_id   = 0;
    last_winner = 0;
}

// Quebra a string recebida do carteador com as cartas do jogador e as armazena
// no vetor de cartas do jogador.
void
Player::receive_cards (const std::string &cards_string) {
    std::stringstream card_stream (cards_string);
    std::string card;

    while (std::getline (card_stream, card, ';')) {
        if (!card.empty ())
            hand.push_back (card);
    }
}

void
Player::display_cards () {
    std::string cards_str = "";
    for (const auto &card : hand) {
        cards_str += card + ", ";
    }
    if (!cards_str.empty () && cards_str.length () >= 2) {
        cards_str.erase (cards_str.length () - 2);
    }
    std::cout << "Suas cartas são: " << cards_str << std::endl;
}

void
Player::promote_dealer () {
    if (!is_dealer ()) {
        node.receive_frame (KEEP_LISTENING);
        std::string buffer_data = node.get_buffer_data ();
        std::cout << buffer_data << std::endl;
        dealer_id =
            std::stoi (buffer_data.substr (buffer_data.find_last_of (" ") + 1));

        node.receive_frame (!KEEP_LISTENING);
        if (is_dealer ()) {
            buffer_data = node.get_buffer_data ();
            update_hp_array (buffer_data);
        }

        return;
    }

    dealer_id = (dealer_id + 1) % PLAYER_QTT;
    std::string message =
        "O novo carteador é o jogador " + std::to_string (dealer_id) + ".\n";
    node.broadcast_message (message);

    std::string hp_msg = "";
    for (int i = 0; i < 4; i++) {
        hp_msg += std::to_string (hp[i]) + ";";
    }
    node.send_message (hp_msg, dealer_id);
}

bool
Player::is_dealer () {
    return id == dealer_id;
}

int
Player::player_id () {
    return id;
}

void
Player::collect_guess () {
    std::string message, guess, current_guesses;

    if (!is_dealer ()) {
        std::cout << "Esperando os demais jogadores...\n" << std::endl;
        node.receive_frame (KEEP_LISTENING);
        current_guesses = node.get_buffer_data ();
    }

    message = current_guesses;
    if (!dead) {
        guess = handle_guess ();
        message += std::to_string (id) + ":" + guess + ";";
    }

    node.send_coll_message (message);
}

std::string
Player::handle_guess () {
    std::string input;
    display_cards ();
    while (true) {
        std::cout << "Quantas jogadas você acha que vai fazer nessa rodada?"
                  << std::endl;

        std::getline (std::cin, input);
        if (input == "/cartas" || input == "/c") {
            display_cards ();
            continue;
        } else if (std::cin.fail () || !isdigit (input[0]) ||
                   static_cast<unsigned int> (stoi (input)) > hand.size ()) {
            std::cout << "Palpite inválido. O palpite precisa ser um número, e "
                         "o valor precisa ser menor que sua "
                         "quantidade de cartas ("
                      << hand.size ()
                      << ") e maior que zero. Tente novamente.\n"
                      << std::endl;
            continue;
        }

        std::cout << std::endl;
        return input;
    }
}


void
Player::display_guesses () {
    node.receive_frame (!is_dealer ());
    std::string buffer_data = node.get_buffer_data ();
    if (!is_dealer ()) {
        std::cout << buffer_data << std::endl;
        return;
    }

    std::stringstream buffer_stream (buffer_data);
    std::string entry;
    while (std::getline (buffer_stream, entry, ';')) {
        size_t colon_pos = entry.find (':');
        if (colon_pos != std::string::npos) {
            int index         = std::stoi (entry.substr (0, colon_pos));
            std::string value = entry.substr (colon_pos + 1);
            guesses[index]    = stoi (value);
        }
    }

    std::string broadcast_msg = "Nessa rodada, os palpites foram:\n";
    for (int i = 0; i < 4; i++) {
        if (hp[i] > 0)
            broadcast_msg += "Jogador " + std::to_string (i) + ": " +
                             std::to_string (guesses[i]) + "\n";
    }

    node.broadcast_message (broadcast_msg);
}

void
Player::collect_plays () {
    std::string message, buffer_data;
    bool is_last_winner = id == last_winner;
    if (!is_last_winner) {
        std::cout << "Esperando os demais jogadores...\n" << std::endl;
        node.receive_frame (KEEP_LISTENING);
        buffer_data = node.get_buffer_data ();
        std::cout << "\nCartas jogadas:" << std::endl;
        std::cout << buffer_data << std::endl;
    }

    message = buffer_data;
    if (!dead) {
        std::string card = pick_card ();

        message += "O jogador " + std::to_string (id) + " jogou a carta " +
                   card + ".\n";
    }

    node.send_coll_message (message);
    std::cout << "Esperando os demais jogadores...\n" << std::endl;

    if (is_last_winner && !is_dealer ()) {
        node.receive_frame (KEEP_LISTENING);
        buffer_data = node.get_buffer_data ();
        std::cout << "Passando as cartas para o carteador computar o "
                     "ganhador da jogada...\n"
                  << std::endl;
        node.send_message (buffer_data, dealer_id);
    }
}

void
Player::compute_lap () {
    std::string buffer_data, entry, broadcast_msg;

    node.receive_frame (!is_dealer ());
    buffer_data = node.get_buffer_data ();
    if (!is_dealer ()) {
        std::cout << buffer_data << std::endl;
        last_winner = get_winner_from_buffer (buffer_data);

        return;
    }

    std::pair<int, int> current_lap_winner = {-1, 0};
    bool draw{false};

    std::stringstream buffer_stream (buffer_data);
    while (std::getline (buffer_stream, entry)) {
        std::size_t card_pos = entry.find ("carta ") + 6;
        char value_char      = entry[card_pos];
        int value;
        value = translate_value (value_char);
        if (value > current_lap_winner.second) {
            std::size_t player_pos = entry.find ("jogador ") + 8;
            int player             = entry[player_pos] - '0';
            draw                   = false;
            current_lap_winner     = {player, value};
        } else if (value == current_lap_winner.second) {
            draw = true;
        }
    }

    if (!draw) {
        broadcast_msg = "O jogador " +
                        std::to_string (current_lap_winner.first) +
                        " fez a jogada!\n";
        plays[current_lap_winner.first] += 1;
        last_winner = current_lap_winner.first;
    } else {
        broadcast_msg = "Houve um empate. Ninguém fez a jogada.\n";
    }

    node.broadcast_message (broadcast_msg);
}

std::string
Player::pick_card () {
    display_cards ();
    while (true) {
        std::cout << "Qual carta você gostaria de jogar?" << std::endl;
        std::string input;
        std::getline (std::cin, input);
        if (input == "/cartas" || input == "/c") {
            display_cards ();
            continue;
        }

        if (input.size () < 2) {
            std::cerr << "Input menor que o tamanho mínimo. Tente novamente.\n"
                      << std::endl;
            continue;
        }

        // Se o valor da carta é 10 ou "as", ocupa um caractere a mais.
        int substr_divisor =
            (input[0] == '1' || input.substr (0, 2) == "as") ? 2 : 1;
        std::string card_value = input.substr (0, substr_divisor);
        if (std::isalpha (card_value[0])) {
            card_value[0] = std::toupper (card_value[0]);
            card_value    = card_value.substr (0, 1);
        }

        // Remove espaços para caso o jogador escreva o naipe por extenso
        std::string suit = input.substr (substr_divisor);
        if (suit.size () > 3 && suit.substr (0, 3) == " de") {
            suit = suit.substr (4);
        }

        auto it = suits_map.find (suit);
        if (it == suits_map.end ()) {
            std::cerr << "Naipe não reconhecido. Tente novamente.\n"
                      << std::endl;
            continue;
        }

        std::string card = card_value + it->second;

        auto card_it = std::find (hand.begin (), hand.end (), card);
        if (card_it == hand.end ()) {
            std::cerr
                << "Você não possui esta carta na sua mão. Tente novamente.\n"
                << std::endl;
            continue;
        }

        hand.erase (card_it);
        std::cout << "\nVocê jogou a carta " << card << "." << std::endl;

        return card;
    }
}

int
Player::get_winner_from_buffer (std::string &buffer_data) {
    std::size_t player_pos = buffer_data.find ("jogador ");
    if (player_pos != std::string::npos) {
        player_pos += 8;
        return buffer_data[player_pos] - '0';
    }

    // Se não encontrou "jogador" no buffer, o último vencedor continua
    // começando a jogada.
    return last_winner;
}

std::string
Player::generate_hand (int round) {
    std::string hand = "";
    for (int j = 0; j < round; j++)
        hand += deck.deal_card ();

    if (!hand.empty () && hand.back () == ';') {
        hand.pop_back ();
    }

    return hand;
}

void
Player::deal_hands (int round) {
    if (!is_dealer ()) {
        node.receive_frame (KEEP_LISTENING);
        receive_cards (node.get_buffer_data ());
        return;
    }

    deck.generate_deck ();
    deck.shuffle ();

    for (int i = (id + 1) % NODE_QTT; i != id; i = (i + 1) % NODE_QTT) {
        if (hp[i] <= 0) {
            node.send_message ("", i, true);
            continue;
        }

        std::string hand = generate_hand (round);

        std::cout << "Entregando cartas ao jogador " << i << "..." << std::endl;
        node.send_message (hand, i, true);
    }

    std::string hand = generate_hand (round);
    std::cout << "Entregando cartas ao jogador " << id << ".\n" << std::endl;
    receive_cards (hand);

    deck.destroy_deck ();
}

int
Player::translate_value (char ch) {
    return value_map[ch];
}

bool
Player::compute_hp () {
    std::string buffer_data;
    if (!is_dealer ()) {
        node.receive_frame (KEEP_LISTENING);
        buffer_data = node.get_buffer_data ();
        bool game_ended = buffer_data.find ("end_game") != std::string::npos;
        std::cout << buffer_data.substr (0, buffer_data.find ("end_game"))
                  << std::endl;
        if (game_ended) {
            return !KEEP_PLAYING;
        }
        if (!dead)
            dead = check_death (buffer_data);

        promote_dealer ();

        return KEEP_PLAYING;
    }

    dead_count = 0;
    std::string broadcast_msg =
        "Ao fim desta rodada, as vidas dos jogadores eram:\n";
    for (int i = 0; i < 4; i++) {
        int damage    = std::abs (guesses[i] - plays[i]);
        bool is_alive = hp[i] > 0;

        hp[i] = std::max (0, hp[i] - damage);

        broadcast_msg += "Jogador " + std::to_string (i) + ": ";
        if (is_alive) {
            broadcast_msg += std::to_string (hp[i]) + " (Disse que ganharia " +
                             std::to_string (guesses[i]) + ", e ganhou " +
                             std::to_string (plays[i]) + ", perdendo ";
            if (hp[i] > 0) {
                broadcast_msg += std::to_string (damage) + " vidas.)\n";
            } else {
                broadcast_msg += "suas últimas vidas.)\n";
            }
        } else {
            broadcast_msg += "0 (O jogador não participou desta rodada.)\n";
        }

        if (hp[i] <= 0) {
            dead_count++;
        }
    }

    if (dead_count >= 3) {
        broadcast_msg += "end_game";
    }
    dead = hp[id] <= 0;
    node.broadcast_message (broadcast_msg);
    if (dead_count >= 3) {
        return !KEEP_PLAYING;
    }
    promote_dealer ();
    return KEEP_PLAYING;
}

bool
Player::check_death (std::string &buffer_data) {
    int my_hp = std::stoi (buffer_data.substr (
        buffer_data.find ("Jogador " + std::to_string (id)) + 11, 2));
    return my_hp <= 0;
}

void
Player::update_hp_array (std::string &buffer_data) {
    std::stringstream buffer_stream (buffer_data);
    std::string hp_str;
    int count = 0;
    while (std::getline (buffer_stream, hp_str, ';')) {
        hp[count] = std::stoi (hp_str);
        ++count;
    }
}

void
Player::check_winner () {
    if (!is_dealer ()) {
        node.receive_frame (KEEP_LISTENING);
        std::string buffer_data = node.get_buffer_data ();
        std::cout << buffer_data << std::endl;

        return;
    }

    std::string broadcast_msg = "A partida acabou! ";

    if (dead_count == PLAYER_QTT) {
        broadcast_msg += "Todos os jogadores morreram, não houve vencedor.";
    } else if (dead_count == ONE_SURVIVOR) {
        for (int i = 0; i < 4; ++i) {
            if (hp[i] > 0) {
                broadcast_msg +=
                    "O jogador " + std::to_string (i) + " é o vencedor!";
                break;
            }
        }
    } else {
        evaluate_winner (broadcast_msg);
    }

    node.broadcast_message (broadcast_msg);
}

void
Player::evaluate_winner (std::string &broadcast_msg) {
    int highest_hp     = 0;
    int current_winner = -1;
    bool draw          = false;

    for (int i = 0; i < 4; ++i) {
        if (hp[i] > 0) {
            if (hp[i] > highest_hp) {
                highest_hp     = hp[i];
                current_winner = i;
                draw           = false;
            } else if (hp[i] == highest_hp) {
                draw = true;
            }
        }
    }

    if (draw) {
        announce_draw (broadcast_msg, highest_hp);
    } else {
        broadcast_msg +=
            "O jogador " + std::to_string (current_winner) + " é o vencedor!";
    }
}

void
Player::announce_draw (std::string &broadcast_msg, int highest_hp) {
    broadcast_msg += "Houve um empate entre os jogadores ";
    for (int i = 0; i < 4; ++i) {
        if (hp[i] == highest_hp) {
            broadcast_msg += std::to_string (i) + ", ";
        }
    }
    // Remove the last ", " and add "!"
    broadcast_msg.erase (broadcast_msg.size () - 2);
    broadcast_msg += "!";
}
