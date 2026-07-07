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
    while (start < text.size() && isspace((unsigned char)text[start])) start++;
    size_t end = text.size();
    while (end > start && isspace((unsigned char)text[end - 1])) end--;
    return text.substr(start, end - start);
}

vector<string> splitWords(const string& line) {
    vector<string> tokens;
    istringstream stream(line);
    string word;
    while (stream >> word) tokens.push_back(word);
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
        if (board[row][col] != ".") return false;
        row += dRow;
        col += dCol;
    }
    return true;
}

bool canMovePiece(const string& piece, int fromRow, int fromCol, int toRow, int toCol, const vector<vector<string>>& board) {
    if (fromRow == toRow && fromCol == toCol) return false;
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
            if (fromRow == toRow || fromCol == toCol) return pathClear(board, fromRow, fromCol, toRow, toCol);
            return false;
        case 'B':
            if (absRow == absCol) return pathClear(board, fromRow, fromCol, toRow, toCol);
            return false;
        case 'Q':
            if (fromRow == toRow || fromCol == toCol || absRow == absCol) return pathClear(board, fromRow, fromCol, toRow, toCol);
            return false;
        case 'P': {
            char color = piece.empty() ? '\0' : piece[0];
            if (color == 'w') {
                if (dRow == -1 && dCol == 0 && board[toRow][toCol] == ".") return true;
                if (dRow == -1 && absCol == 1 && board[toRow][toCol] != "." && !sameColor(piece, board[toRow][toCol])) return true;
                return false;
            } else if (color == 'b') {
                if (dRow == 1 && dCol == 0 && board[toRow][toCol] == ".") return true;
                if (dRow == 1 && absCol == 1 && board[toRow][toCol] != "." && !sameColor(piece, board[toRow][toCol])) return true;
                return false;
            }
            return false;
        }
        default:
            return false;
    }
}

struct PendingMove {
    int fromRow;
    int fromCol;
    int toRow;
    int toCol;
    string piece;
    long long arrival;
};

bool sourceIsMoving(int row, int col, const vector<PendingMove>& pending) {
    for (const auto& pm : pending) {
        if (pm.fromRow == row && pm.fromCol == col) return true;
    }
    return false;
}

bool destIsReserved(int row, int col, long long arrival, const vector<PendingMove>& pending) {
    for (const auto& pm : pending) {
        if ((pm.fromRow == row && pm.fromCol == col && arrival >= pm.arrival) ||
            (pm.toRow == row && pm.toCol == col && arrival >= pm.arrival)) {
            return true;
        }
    }
    return false;
}

void applyArrivedMoves(vector<vector<string>>& board, vector<PendingMove>& pending, long long clockMs) {
    if (pending.empty()) return;
    sort(pending.begin(), pending.end(), [](const PendingMove& a, const PendingMove& b) {
        return a.arrival < b.arrival;
    });

    vector<PendingMove> remaining;
    long long currentTime = -1;
    vector<PendingMove> arriving;
    auto flushArriving = [&](const vector<PendingMove>& group) {
        if (group.empty()) return;
        map<pair<int,int>, vector<PendingMove>> byTarget;
        for (const auto& pm : group) {
            byTarget[{pm.toRow, pm.toCol}].push_back(pm);
        }
        for (const auto& [target, moves] : byTarget) {
            if (moves.size() != 1) continue; // conflicting target arrivals, all canceled
            const PendingMove& pm = moves[0];
            bool blocked = false;
            for (const auto& future : group) {
                if (&future == &pm) continue;
            }
            // If destination is currently occupied by a friendly or a still-moving piece, cancel.
            if (board[pm.toRow][pm.toCol] != "." && sameColor(pm.piece, board[pm.toRow][pm.toCol])) continue;
            bool occupiedByMoving = false;
            for (const auto& other : group) {
                if (&other != &pm && other.fromRow == pm.toRow && other.fromCol == pm.toCol) {
                    occupiedByMoving = true;
                    break;
                }
            }
            if (occupiedByMoving) continue;
            board[pm.toRow][pm.toCol] = pm.piece;
            if (board[pm.fromRow][pm.fromCol] == pm.piece) {
                board[pm.fromRow][pm.fromCol] = ".";
            }
        }
    };

    for (const auto& pm : pending) {
        if (pm.arrival <= clockMs) {
            if (currentTime < 0 || pm.arrival != currentTime) {
                flushArriving(arriving);
                arriving.clear();
                currentTime = pm.arrival;
            }
            arriving.push_back(pm);
        } else {
            remaining.push_back(pm);
        }
    }
    flushArriving(arriving);
    pending.swap(remaining);
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
            if (trimmed.empty()) continue;
            auto row = splitWords(trimmed);
            if (!row.empty()) board.push_back(row);
        } else if (readingCommands) {
            if (!trimmed.empty()) commands.push_back(trimmed);
        }
    }

    if (board.empty()) return 0;

    int rows = board.size();
    int cols = board[0].size();
    for (const auto& r : board) {
        if ((int)r.size() != cols) {
            cout << "ERROR ROW_WIDTH_MISMATCH" << endl;
            return 0;
        }
        for (const auto& tok : r) {
            if (!isValidToken(tok)) {
                cout << "ERROR UNKNOWN_TOKEN" << endl;
                return 0;
            }
        }
    }

    struct Pos { int row; int col; };
    optional<Pos> selected;
    long long clockMs = 0;
    vector<PendingMove> pending;
    const long long MOVE_TIME = 2000;

    auto inBoard = [&](int c, int r) {
        return c >= 0 && c < cols && r >= 0 && r < rows;
    };

    auto printBoard = [&]() {
        applyArrivedMoves(board, pending, clockMs);
        for (const auto& r : board) {
            for (int i = 0; i < (int)r.size(); ++i) {
                if (i) cout << " ";
                cout << r[i];
            }
            cout << '\n';
        }
    };

    for (const auto& cmdLine : commands) {
        auto tok = splitWords(cmdLine);
        if (tok.empty()) continue;

        if (tok[0] == "click" && tok.size() == 3) {
            applyArrivedMoves(board, pending, clockMs);
            int x = stoi(tok[1]);
            int y = stoi(tok[2]);
            int col = x / 100;
            int row = y / 100;
            if (!inBoard(col, row)) continue;

            const string& clicked = board[row][col];
            if (isPiece(clicked)) {
                if (!selected.has_value()) {
                    if (!sourceIsMoving(row, col, pending)) selected = Pos{row, col};
                } else {
                    const string selTok = board[selected->row][selected->col];
                    if (sameColor(selTok, clicked)) {
                        if (!sourceIsMoving(row, col, pending)) selected = Pos{row, col};
                    } else {
                        if (sourceIsMoving(selected->row, selected->col, pending)) continue;
                        long long arrival = clockMs + MOVE_TIME;
                        if (destIsReserved(row, col, arrival, pending)) continue;
                        if (!canMovePiece(selTok, selected->row, selected->col, row, col, board)) continue;
                        pending.push_back(PendingMove{selected->row, selected->col, row, col, selTok, arrival});
                        selected.reset();
                    }
                }
            } else {
                if (!selected.has_value()) continue;
                if (sourceIsMoving(selected->row, selected->col, pending)) continue;
                if (destIsReserved(row, col, clockMs + MOVE_TIME, pending)) continue;
                const string selTok = board[selected->row][selected->col];
                if (!canMovePiece(selTok, selected->row, selected->col, row, col, board)) continue;
                if (board[row][col] != "." && sameColor(selTok, board[row][col])) continue;
                pending.push_back(PendingMove{selected->row, selected->col, row, col, selTok, clockMs + MOVE_TIME});
                selected.reset();
            }
        } else if (tok[0] == "wait" && tok.size() == 2) {
            int ms = stoi(tok[1]);
            if (ms > 0) {
                clockMs += ms;
                applyArrivedMoves(board, pending, clockMs);
            }
        } else if (tok[0] == "print" && tok.size() == 2 && tok[1] == "board") {
            printBoard();
        }
    }

    return 0;
}
