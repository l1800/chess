// common.h - shared declarations for chess iterations
#ifndef CHESS_COMMON_H
#define CHESS_COMMON_H

#include <string>
#include <vector>

std::string trim(const std::string& text);
std::vector<std::string> splitWords(const std::string& line);
bool isValidToken(const std::string& token);
bool isPiece(const std::string& token);
bool sameColor(const std::string& a, const std::string& b);
bool pathClear(const std::vector<std::vector<std::string>>& board, int fromRow, int fromCol, int toRow, int toCol);
bool canMovePiece(const std::string& piece, int fromRow, int fromCol, int toRow, int toCol, const std::vector<std::vector<std::string>>& board);

#endif // CHESS_COMMON_H
