#ifndef PLAYER_H
#define PLAYER_H

#include "deck.hpp"
#include "ring_node.hpp"
#include <string>
#include <vector>

class Player {
  public:
    Player (int id, std::string &ifname, std::string &next_id_ip,
            std::string &prev_id_ip);
    Deck deck;
    RingNode node;

    void receive_cards (const std::string &card);
    void promote_dealer ();
    bool is_dealer ();
    void display_cards ();
    void display_guesses ();
    void collect_guess ();
    int player_id ();
    void deal_hands (int round);
    int hand_size ();
    bool is_hand_empty ();
    std::string pick_card ();
    void collect_plays ();
    void compute_lap ();
    bool compute_hp ();
    void check_winner ();
    void announce_single_survivor (std::string &broadcast_msg);
    void evaluate_winner (std::string &broadcast_msg);
    void announce_draw (std::string &broadcast_msg, int highest_hp);

  private:
    int id;
    int dealer_id;
    int last_winner;
    bool dead;
    int dead_count;
    std::string ifname;

    int translate_value (char ch);
    int get_winner_from_buffer (std::string &buffer_data);
    std::string handle_guess ();
    bool is_between ();
    bool check_death (std::string &buffer_data);
    void update_hp_array (std::string &buffer_data);

    std::string generate_hand (int round);
    std::vector<std::string> hand;
    std::vector<int> guesses;
    std::vector<int> plays;
    std::vector<int> hp;
    std::unordered_map<std::string, std::string> suits_map = {
        {"p",      "♧"},
        {"o",      "♢"},
        {"c",      "♡"},
        {"e",      "♤"},
        {"paus",   "♧"},
        {"ouro",   "♢"},
        {"copas",  "♡"},
        {"espada", "♤"}
    };

    // Tabela hash para tratar as cartas A, 10, J, Q e K. Os demais valores são
    // apenas para simplificar a chamada;
    std::unordered_map<char, int> value_map = {
        {'A', 1 },
        {'1', 10},
        {'J', 11},
        {'Q', 12},
        {'K', 13},
        {'2', 2 },
        {'3', 3 },
        {'4', 4 },
        {'5', 5 },
        {'6', 6 },
        {'7', 7 },
        {'8', 8 },
        {'9', 9 },
    };
};

#endif // PLAYER_H
