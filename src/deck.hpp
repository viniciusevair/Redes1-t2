#ifndef DECK_H
#define DECK_H

#include <string>
#include <vector>

class Deck {
  public:
    Deck ();

    void shuffle ();
    std::string deal_card ();

    void generate_deck ();
    void destroy_deck ();

  private:
    std::vector<std::string> suits  = {"♡", "♢", "♧", "♤"};
    std::vector<std::string> values = {"A", "2", "3",  "4", "5", "6", "7",
                                       "8", "9", "10", "J", "Q", "K"};
    std::vector<std::string> cards;
};

#endif // DECK_H
