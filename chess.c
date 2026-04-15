#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*
 * You are implementing a simplified chess engine using bitboards.
 *
 * A chessboard is represented using a 64-bit unsigned integer (uint64_t),
 * where each bit corresponds to a square on the board:
 *
 *   bit 0  = a1
 *   bit 1  = b1
 *   ...
 *   bit 63 = h8
 * 
 *  0   1   2   3   4   5   6   7     
 *  a1  b1  c1  d1  e1  f1  g1  h1  
 *  a2  .   .   .   .   .   .   h2
 *  .   .   .   .   .   .   .   .
 *  .   .   .   .   .   .   .   .
 *  .   b8                      h8
 *
 * White pawns move "up" the board (towards higher bit indices).
 *
 * You are given:
 *   - a bitboard of white pawns
 *   - a bitboard of black pieces (targets)
 *
 * Your task is to generate pawn attack moves.
 *
 * A white pawn attacks diagonally forward:
 *   - forward-left  : shift by +7
 *   - forward-right : shift by +9
 *
 * IMPORTANT:
 * - Do NOT use loops over squares
 * - Do NOT use branching inside core logic
 * - You must use bitwise operations only
 */

/*
 * Generate all squares attacked by white pawns.
 *
 * @param white_pawns  bitboard of white pawns
 * @param black_pieces bitboard of enemy pieces
 *
 * @return bitboard of all valid attack targets
 */
uint64_t pawn_attacks_white(uint64_t white_pawns, uint64_t black_pieces){

    
}

/*

moving up one rank = +8 bits
moving right one file = +1 bit
moving left one file = -1 bit


*/


int main(void){

    return 0;
}