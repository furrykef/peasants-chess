# Peasants' Chess

This is a work-in-progress program to attempt to solve the game of Peasants' Chess, and probably also let you play it against an AI. Don't bother googling it; I believe it was invented by a local named Bob Trager.


## The rules of Peasants' Chess

Basic rules:
  * The game uses a standard 8x8 chessboard.
  * Both players begin with pawns on their second and third ranks (rows). E.g., black has pawns on a7 through h7 and a6 through h6.
  * Pawns move as in chess (explained below).
  * White moves first.
  * The first player to advance a pawn to his eighth rank wins.

How pawns move:
  * A pawn can advance one square forward if that square is unoccupied.
  * A pawn can capture an enemy pawn by moving one square diagonally and occupying its space.
  * A pawn on the second rank can optionally move two spaces forward instead of one. It can still capture only one square diagonally.
  * If a pawn moves forward two spaces, neighboring enemy pawns can capture it as if it had moved only one space; that is, the capturing pawn moves into the empty square behind the enemy pawn, and the enemy pawn is removed from the board. The pawn is said to be captured "en passant" (French for "in passing").

These next rules are currently unconfirmed; I will have to consult with Bob:
  * If a player takes his opponent's last pawn, he wins the game.
  * Otherwise, if the player cannot move because he has no moves, the game is a draw. This is called stalemate.
