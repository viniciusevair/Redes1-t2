#include "player.hpp"
#include "ring_node.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <ostream>

int
main (int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Uso: " << argv[0] << " <id> <if_name> <next_id_ip> <prev_id_ip>" << std::endl;
        return EXIT_FAILURE;
    }

    int id = atoi (argv[1]);
    std::string if_name = argv[2];
    std::string next_id_ip = argv[3];
    std::string prev_id_ip = argv[4];

    Player player (id, if_name, next_id_ip, prev_id_ip);

    std::cout << "Tentando conectar..." << std::endl;
    while (!player.node.connect ())
        player.node.receive_frame ();

    // A quantidade de rodadas é também a quantidade de cartas iniciais do jogo.
    int round = 5;
    while (round) {
        int lap_count = round;
        player.deal_hands (round);
        player.collect_guess ();
        player.display_guesses ();

        while (lap_count > 0) {
            player.collect_plays ();
            player.compute_lap ();
            lap_count--;
        }

        bool keep_playing = player.compute_hp ();
        if (!keep_playing)
            break;

        round -= 1;
    }
    player.check_winner ();

    return 0;
}
