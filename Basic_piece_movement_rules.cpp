#include <algorithm>
#include <cctype>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

const set<string> validTokens = {
    ".",
    "wK", "wQ", "wR", "wB", "wN", "wP",
    "bK", "bQ", "bR", "bB", "bN", "bP"
};

string trim(const string& text) {
    size_t start = 0;
    while (start < text.size() && isspace((unsigned char)text[start])) {
        start++;
    }
    size_t end = text.size();
    while (end > start && isspace((unsigned char)text[end - 1])) {
        end--;
    }
    return text.substr(start, end - start);
}

vector<string> splitWords(const string& line) {
    vector<string> tokens;
    istringstream stream(line);
    string word;
    while (stream >> word) {
        tokens.push_back(word);
    }
    return tokens;
}

bool isValidToken(const string& token) {
    return validTokens.count(token) > 0;
}

bool isPiece(const string& token) {
    return token != ".";
}

bool sameColor(const string& a, const string& b) {
    return !a.empty() && !b.empty() && a[0] == b[0];
}

bool pathClear(const vector<vector<string>>& board, int fromRow, int fromCol, int toRow, int toCol) {
    int dRow = (toRow > fromRow) ? 1 : ((toRow < fromRow) ? -1 : 0);
    int dCol = (toCol > fromCol) ? 1 : ((toCol < fromCol) ? -1 : 0);
    int row = fromRow + dRow;
    int col = fromCol + dCol;
    while (row != toRow || col != toCol) {
        if (board[row][col] != ".") {
            return false;
        }
        row += dRow;
        col += dCol;
    }
    return true;
}

bool canMovePiece(const string& piece, int fromRow, int fromCol, int toRow, int toCol, const vector<vector<string>>& board) {
    if (fromRow == toRow && fromCol == toCol) {
        return false;
    }
    int dRow = toRow - fromRow;
    int dCol = toCol - fromCol;
    int absRow = abs(dRow);
    int absCol = abs(dCol);
    char type = piece.size() > 1 ? piece[1] : '\0';

    switch (type) {
        case 'K':
            return absRow <= 1 && absCol <= 1;
        case 'N':
            return (absRow == 1 && absCol == 2) || (absRow == 2 && absCol == 1);
        case 'R':
            if (fromRow == toRow || fromCol == toCol) {
                return pathClear(board, fromRow, fromCol, toRow, toCol);
            }
            return false;
        case 'B':
            if (absRow == absCol) {
                return pathClear(board, fromRow, fromCol, toRow, toCol);
            }
            return false;
        case 'Q':
            if (fromRow == toRow || fromCol == toCol || absRow == absCol) {
                return pathClear(board, fromRow, fromCol, toRow, toCol);
            }
            return false;
        default:
            return false;
    }
}

int runBoardCommands() {
    vector<vector<string>> board;
    vector<string> commands;
    string line;
    bool readingBoard = false;
    bool readingCommands = false;

    while (getline(cin, line)) {
        string trimmed = trim(line);
        if (trimmed == "Board:") {
            readingBoard = true;
            readingCommands = false;
            continue;
        }
        if (trimmed == "Commands:" || trimmed.rfind("Commands", 0) == 0) {
            readingBoard = false;
            readingCommands = true;
            continue;
        }

        if (readingBoard) {
            if (trimmed.empty()) {
                continue;
            }
            auto rowTokens = splitWords(trimmed);
            if (!rowTokens.empty()) {
                board.push_back(rowTokens);
            }
        } else if (readingCommands) {
            if (!trimmed.empty()) {
                commands.push_back(trimmed);
            }
        }
    }

    if (board.empty()) {
        return 0;
    }

    int rows = board.size();
    int cols = board[0].size();
    for (const auto& row : board) {
        if ((int)row.size() != cols) {
            cout << "ERROR ROW_WIDTH_MISMATCH" << endl;
            return 0;
        }
        for (const auto& token : row) {
            if (!isValidToken(token)) {
                cout << "ERROR UNKNOWN_TOKEN" << endl;
                return 0;
            }
        }
    }

    struct Position { int row; int col; };
    optional<Position> selected;
    long long clockMs = 0;

    auto isInside = [&](int col, int row) {
        return col >= 0 && col < cols && row >= 0 && row < rows;
    };

    auto printBoard = [&]() {
        for (const auto& row : board) {
            for (int i = 0; i < (int)row.size(); ++i) {
                if (i > 0) cout << " ";
                cout << row[i];
            }
            cout << '\n';
        }
    };

    for (const auto& commandLine : commands) {
        auto tokens = splitWords(commandLine);
        if (tokens.empty()) {
            continue;
        }

        if (tokens[0] == "click" && tokens.size() == 3) {
            int x = stoi(tokens[1]);
            int y = stoi(tokens[2]);
            int col = x / 100;
            int row = y / 100;
            if (!isInside(col, row)) {
                continue;
            }

            const string& clicked = board[row][col];
            if (isPiece(clicked)) {
                if (!selected.has_value()) {
                    selected = Position{row, col};
                } else {
                    const string& selectedToken = board[selected->row][selected->col];
                    if (sameColor(selectedToken, clicked)) {
                        selected = Position{row, col};
                    } else {
                        if (canMovePiece(selectedToken, selected->row, selected->col, row, col, board)) {
                            board[row][col] = selectedToken;
                            board[selected->row][selected->col] = ".";
                            selected.reset();
                        }
                    }
                }
            } else {
                if (selected.has_value()) {
                    const string selectedToken = board[selected->row][selected->col];
                    if (canMovePiece(selectedToken, selected->row, selected->col, row, col, board)) {
                        board[row][col] = selectedToken;
                        board[selected->row][selected->col] = ".";
                        selected.reset();
                    }
                }
            }
        } else if (tokens[0] == "wait" && tokens.size() == 2) {
            int ms = stoi(tokens[1]);
            if (ms > 0) {
                clockMs += ms;
            }
        } else if (tokens[0] == "print" && tokens.size() == 2 && tokens[1] == "board") {
            printBoard();
        }
    }

    return 0;
}

int main() {
    return runBoardCommands();
}
