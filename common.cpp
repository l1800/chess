// common.cpp - implementations of shared helpers
#include "common.h"

#include <algorithm>
#include <cctype>
#include <sstream>

using namespace std;

const static std::vector<std::string> VALID_TOKENS = {
    ".",
    "wK", "wQ", "wR", "wB", "wN", "wP",
    "bK", "bQ", "bR", "bB", "bN", "bP"
};

string trim(const string& text) {
    size_t start = 0;
    while (start < text.size() && isspace((unsigned char)text[start])) start++;
    size_t end = text.size();
    while (end > start && isspace((unsigned char)text[end-1])) end--;
    return text.substr(start, end-start);
}

vector<string> splitWords(const string& line) {
    vector<string> tokens;
    istringstream stream(line);
    string w;
    while (stream >> w) tokens.push_back(w);
    return tokens;
}

bool isValidToken(const string& token) {
    for (auto &t : VALID_TOKENS) if (t == token) return true;
    return false;
}

bool isPiece(const string& token) { return token != "."; }

bool sameColor(const string& a, const string& b) { return !a.empty() && !b.empty() && a[0] == b[0]; }

bool pathClear(const vector<vector<string>>& board, int fromRow, int fromCol, int toRow, int toCol) {
    int dRow = (toRow > fromRow) ? 1 : ((toRow < fromRow) ? -1 : 0);
    int dCol = (toCol > fromCol) ? 1 : ((toCol < fromCol) ? -1 : 0);
    int r = fromRow + dRow, c = fromCol + dCol;
    while (r != toRow || c != toCol) {
        if (board[r][c] != ".") return false;
        r += dRow; c += dCol;
    }
    return true;
}

bool canMovePiece(const string& piece, int fromRow, int fromCol, int toRow, int toCol, const vector<vector<string>>& board) {
    if (fromRow == toRow && fromCol == toCol) return false;
    int dRow = toRow - fromRow; int dCol = toCol - fromCol;
    int absRow = abs(dRow), absCol = abs(dCol);
    char type = piece.size() > 1 ? piece[1] : '\0';
    switch (type) {
        case 'K': return absRow <= 1 && absCol <= 1;
        case 'N': return (absRow==1 && absCol==2) || (absRow==2 && absCol==1);
        case 'R': if (fromRow==toRow || fromCol==toCol) return pathClear(board, fromRow, fromCol, toRow, toCol); return false;
        case 'B': if (absRow==absCol) return pathClear(board, fromRow, fromCol, toRow, toCol); return false;
        case 'Q': if (fromRow==toRow || fromCol==toCol || absRow==absCol) return pathClear(board, fromRow, fromCol, toRow, toCol); return false;
        case 'P': {
            if (piece.empty()) return false; char color = piece[0];
            if (color == 'w') {
                if (dRow==-1 && dCol==0 && board[toRow][toCol]==".") return true;
                if (dRow==-1 && absCol==1 && board[toRow][toCol]!="." && !sameColor(piece, board[toRow][toCol])) return true;
                return false;
            } else if (color=='b') {
                if (dRow==1 && dCol==0 && board[toRow][toCol]==".") return true;
                if (dRow==1 && absCol==1 && board[toRow][toCol]!="." && !sameColor(piece, board[toRow][toCol])) return true;
                return false;
            }
            return false;
        }
        default: return false;
    }
}
