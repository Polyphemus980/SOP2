#pragma once

#include "cards.h"

/**
 * Structure representing players hand.
 * This structure is local and doesn't require synchronisation.
 */
typedef struct player_hand
{
    /**
     * Index of player at this game. It is acquired when joining a table.
     */
    int idx;
    /**
     * It is index of leading player on current trick. PLayer has to track it on its side.
     */
    int leading_player_idx;
    /**
     * Counter of tricks played at this game.
     */
    int trick_counter;
    /**
     * Cards holding at hand. At the beginning there is 13 cards.
     * Played cards are marked by INVALID_CARD number.
     */
    card_t hand_cards[PLAYER_HAND_COUNT];
    /**
     * Cards played at consecutive tricks. useful for showing history of the game.
     * It contains ONLY cards played by player.
     */
    card_t played_cards[PLAYER_HAND_COUNT];
    /**
     * Result of every played card. If player or its partner won a trick is should contain CARD_TAKEN.
     */
    int trick_results[PLAYER_HAND_COUNT];
} player_hand_t;

/**
 * Initialisation of player hand.
 * Calling this function makes hand to be in valid state.
 *
 * @param hand Address of newly created hand.
 * @param player_idx Player index received during joining the table.
 */
void player_hand_init(player_hand_t* hand, int player_idx);

/**
 * Prints hand to STDOUT. It shows currently held cards at hand and all played cards.
 * @param hand Address of player hand.
 */
void player_hand_print_hand(player_hand_t* hand);

/**
 * Function finds the index of the strongest card at given color.
 * If there is no card in hand at this color it should return -1.
 *
 * @param hand Address of player hand.
 * @param color Color index to search a strongest card
 * @return
 * If there is any card at given color in hand, functions returns index of the strongest card.
 * Otherwise functions returns -1.
 */
int player_hand_find_strongest(player_hand_t* hand, int color);

/**
 * Functions determines if there is stronger card in hand than given card
 * @param hand
 * @param other
 * @return
 */
int player_hand_has_stronger(player_hand_t* hand, card_t other);

/**
 *
 * @param hand
 * @param color
 * @return
 */
int player_hand_find_weakest(player_hand_t* hand, int color);

/**
 *
 * @param hand
 * @return
 */
int player_hand_select_leading_card(player_hand_t* hand);

/**
 *
 * @param hand
 * @param played_idx
 */
void player_hand_print_played_card(player_hand_t *hand, int played_idx);
