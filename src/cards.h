#pragma once

#include <stdio.h>

#define PLAYERS_COUNT 4
#define CARDS_COUNT 52
#define PLAYER_HAND_COUNT (CARDS_COUNT / PLAYERS_COUNT)

#define CARDS_IN_COLOR 13
#define CLUBS_IDX 0
#define DIAMONDS_IDX 1
#define HEARTS_IDX 2
#define SPADES_IDX 3

#define ACE_IDX 12
#define KING_IDX 11
#define QUEEN_IDX 10
#define JACK_IDX 9

#define INVALID_CARD 0xff
#define CARD_TAKEN 1

#define PREV_PLAYER_IDX(IDX) ((IDX + PLAYERS_COUNT - 1) % PLAYERS_COUNT)
#define CARD_COLOR(CARD) (CARD / CARDS_IN_COLOR)
#define CARD_POWER(CARD) (CARD % CARDS_IN_COLOR)

/**
 * Type representing card. Card can be from 0 to 51.
 * Cards have 4 colors (Clubs, Diamonds, Hearts and Spades).
 * Dividing number by 4 it is color of the card.
 * Color have 13 cards. Modulo of 13 is power of the card.
 */
typedef unsigned char card_t;

/**
 * This functions shuffles array once.
 * Use multiple times for proper card shuffle.
 * It uses randomisation. Call srand() before usage.
 *
 * @param array
 * Address of array to shuffle
 * @param n
 * Number of array elements.
 */
void shuffle(card_t *array, size_t n);

/**
 * Helper fucntion for printing card a card.
 * It should present card by its power (2,3,...,10,J,Q,K,A) and color (C,D,D,S).
 * For e.g. 10C.
 *
 * @param card_number
 * Card number to print.
 */
void print_card(int card_number);

/**
 *  Function tests if card is stronger than reference_card.
 * @param reference_card
 * @param card
 * @return
 */
int is_card_stronger_than(card_t reference_card, card_t card);
