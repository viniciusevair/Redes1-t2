#include <algorithm>
#include <random>

#include "deck.hpp"

Deck::Deck () {
}

void
Deck::shuffle () {
    std::random_device rd;
    std::mt19937 g (rd ());
    std::shuffle (cards.begin (), cards.end (), g);
}

void
Deck::generate_deck () {
    for (const auto &suit : suits)
        for (const auto &value : values)
            cards.push_back (value + suit + ";");
}

void
Deck::destroy_deck () {
    cards.clear ();
}

std::string
Deck::deal_card () {
    if (cards.empty ()) {
        return "";
    }
    std::string card = cards.back ();
    cards.pop_back ();
    return card;
}
