// ============================================================================
// Chess Game - Platform-Compatible Version
// Repository: https://github.com/l1800/chess
// This is a single-file version for VPL/ultracode.education platform
// ============================================================================

#include <algorithm>
#include <cctype>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

using namespace std;

// ============================================================================
// Configuration: All hard-coded values centralized
// ============================================================================

namespace config {
    constexpr long long MOVE_ARRIVAL_TIME_MS = 1000;
    constexpr long long JUMP_DURATION_MS = 1000;
    constexpr char EMPTY_CELL = '.';
    constexpr char COLOR_WHITE = 'w';
    constexpr char COLOR_BLACK = 'b';
}

// ============================================================================
// String Utilities
// ============================================================================

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

// ============================================================================
// Board Abstraction (supports text or future binary representation)
// ============================================================================

class Board {
public:
    virtual ~Board() = default;
    virtual string getPiece(int row, int col) const = 0;
    virtual void setPiece(int row, int col, const string& piece) = 0;
    virtual int getRows() const = 0;
    virtual int getCols() const = 0;
    virtual bool isValid(int row, int col) const = 0;
};

class TextBoard : public Board {
private:
    vector<vector<string>> cells;

public:
    TextBoard() = default;
    explicit TextBoard(const vector<vector<string>>& data) : cells(data) {}

    string getPiece(int row, int col) const override {
        if (!isValid(row, col)) return "";
        return cells[row][col];
    }

    void setPiece(int row, int col, const string& piece) override {
        if (isValid(row, col)) cells[row][col] = piece;
    }

    int getRows() const override { return cells.size(); }
    int getCols() const override { return cells.empty() ? 0 : cells[0].size(); }
    bool isValid(int row, int col) const override {
        return row >= 0 && row < getRows() && col >= 0 && col < getCols();
    }

    vector<vector<string>>& getRawData() { return cells; }
    const vector<vector<string>>& getRawData() const { return cells; }
};

// ============================================================================
// Piece System (extensible for custom games)
// ============================================================================

struct PieceDefinition {
    char symbol;
    bool canCaptureEnemy;
    bool canMoveEmpty;
    std::function<bool(int fromRow, int fromCol, int toRow, int toCol, const Board& board)> 
        validateMove;
    std::function<string(const string& piece, int targetRow, int rows)> 
        onReachEndRow;
};

class PieceRegistry {
private:
    unordered_map<char, PieceDefinition> pieces;

public:
    void registerPiece(const PieceDefinition& def) {
        pieces[def.symbol] = def;
    }

    optional<PieceDefinition> getPiece(char symbol) const {
        auto it = pieces.find(symbol);
        return it != pieces.end() ? optional(it->second) : nullopt;
    }

    set<char> getValidSymbols() const {
        set<char> symbols;
        for (const auto& p : pieces) symbols.insert(p.first);
        return symbols;
    }
};

// ============================================================================
// Game State Structures
// ============================================================================

struct PendingMove {
    int fromRow, fromCol, toRow, toCol;
    string piece;
    long long arrivalTimeMs;
};

struct AirborneState {
    int row, col;
    string piece;
    long long landTimeMs;
};

// ============================================================================
// Helper Functions
// ============================================================================

bool isSameColor(const string& piece1, const string& piece2) {
    return !piece1.empty() && !piece2.empty() && piece1[0] == piece2[0];
}

bool isPieceType(const string& token) {
    return token != string(1, config::EMPTY_CELL);
}

bool pathClear(const Board& board, int fromRow, int fromCol, int toRow, int toCol) {
    int dRow = (toRow > fromRow) ? 1 : ((toRow < fromRow) ? -1 : 0);
    int dCol = (toCol > fromCol) ? 1 : ((toCol < fromCol) ? -1 : 0);
    int row = fromRow + dRow;
    int col = fromCol + dCol;

    while (row != toRow || col != toCol) {
        if (board.getPiece(row, col) != string(1, config::EMPTY_CELL)) return false;
        row += dRow;
        col += dCol;
    }
    return true;
}

bool canMovePawn(const string& piece, int fromRow, int fromCol, int toRow, int toCol, const Board& board) {
    int dRow = toRow - fromRow;
    int dCol = toCol - fromCol;
    int absCol = abs(dCol);
    int rows = board.getRows();

    char color = piece[0];
    string empty(1, config::EMPTY_CELL);

    if (color == config::COLOR_WHITE) {
        if (dCol == 0 && dRow == -1 && board.getPiece(toRow, toCol) == empty) return true;
        if (dCol == 0 && dRow == -2 && fromRow == rows - 1 && 
            board.getPiece(fromRow - 1, fromCol) == empty && 
            board.getPiece(toRow, toCol) == empty) return true;
        if (dRow == -1 && absCol == 1 && board.getPiece(toRow, toCol) != empty &&
            !isSameColor(piece, board.getPiece(toRow, toCol))) return true;
        return false;
    } else if (color == config::COLOR_BLACK) {
        if (dCol == 0 && dRow == 1 && board.getPiece(toRow, toCol) == empty) return true;
        if (dCol == 0 && dRow == 2 && fromRow == 0 && 
            board.getPiece(fromRow + 1, fromCol) == empty && 
            board.getPiece(toRow, toCol) == empty) return true;
        if (dRow == 1 && absCol == 1 && board.getPiece(toRow, toCol) != empty &&
            !isSameColor(piece, board.getPiece(toRow, toCol))) return true;
        return false;
    }
    return false;
}

bool canMovePiece(const string& piece, int fromRow, int fromCol, int toRow, int toCol, const Board& board) {
    if (fromRow == toRow && fromCol == toCol) return false;
    if (!board.isValid(toRow, toCol)) return false;

    string empty(1, config::EMPTY_CELL);
    string targetPiece = board.getPiece(toRow, toCol);

    if (targetPiece != empty && isSameColor(piece, targetPiece)) return false;
    if (piece.length() < 2) return false;

    char type = piece[1];
    if (type == 'P') return canMovePawn(piece, fromRow, fromCol, toRow, toCol, board);

    int dRow = toRow - fromRow;
    int dCol = toCol - fromCol;
    int absRow = abs(dRow);
    int absCol = abs(dCol);

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
        default:
            return false;
    }
}

bool isPromotionRow(int row, const string& piece, int rows) {
    if (piece.length() < 2) return false;
    char type = piece[1];
    return (type == 'P' && piece[0] == config::COLOR_WHITE && row == 0) ||
           (type == 'P' && piece[0] == config::COLOR_BLACK && row == rows - 1);
}

bool isValidToken(const string& token) {
    if (token == string(1, config::EMPTY_CELL)) return true;
    if (token.length() != 2) return false;
    char color = token[0];
    char symbol = token[1];
    if (color != config::COLOR_WHITE && color != config::COLOR_BLACK) return false;
    return (symbol == 'K' || symbol == 'Q' || symbol == 'R' || symbol == 'B' || symbol == 'N' || symbol == 'P');
}

// ============================================================================
// Main Game Logic
// ============================================================================

int runBoardCommands() {
    vector<vector<string>> boardData;
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
            if (!row.empty()) boardData.push_back(row);
        } else if (readingCommands) {
            if (!trimmed.empty()) commands.push_back(trimmed);
        }
    }

    if (boardData.empty()) return 0;

    int rows = boardData.size();
    int cols = boardData[0].size();

    for (const auto& row : boardData) {
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

    unique_ptr<Board> board = make_unique<TextBoard>(boardData);
    vector<PendingMove> pendingMoves;
    vector<AirborneState> airborne;
    long long clockMs = 0;
    string empty(1, config::EMPTY_CELL);

    auto printBoard = [&]() {
        TextBoard* textBoard = dynamic_cast<TextBoard*>(board.get());
        if (!textBoard) return;
        const auto& data = textBoard->getRawData();
        for (const auto& r : data) {
            for (size_t i = 0; i < r.size(); ++i) {
                if (i) cout << " ";
                cout << r[i];
            }
            cout << '\n';
        }
    };

    auto applyArrivedMoves = [&](long long currentTime) {
        vector<PendingMove> remaining;
        sort(pendingMoves.begin(), pendingMoves.end(),
             [](const PendingMove& a, const PendingMove& b) { return a.arrivalTimeMs < b.arrivalTimeMs; });

        for (const auto& pm : pendingMoves) {
            if (pm.arrivalTimeMs <= currentTime) {
                string source = board->getPiece(pm.fromRow, pm.fromCol);
                if (source != pm.piece) continue;

                bool capturedByAirborne = false;
                for (const auto& air : airborne) {
                    if (air.row == pm.toRow && air.col == pm.toCol && !isSameColor(pm.piece, air.piece)) {
                        board->setPiece(pm.fromRow, pm.fromCol, empty);
                        capturedByAirborne = true;
                        break;
                    }
                }
                if (capturedByAirborne) continue;

                string target = board->getPiece(pm.toRow, pm.toCol);
                if (target != empty && isSameColor(pm.piece, target)) continue;

                board->setPiece(pm.toRow, pm.toCol, pm.piece);
                board->setPiece(pm.fromRow, pm.fromCol, empty);

                if (isPromotionRow(pm.toRow, pm.piece, rows)) {
                    board->setPiece(pm.toRow, pm.toCol, string(1, pm.piece[0]) + "Q");
                }
            } else {
                remaining.push_back(pm);
            }
        }

        vector<AirborneState> stillAirborne;
        for (const auto& air : airborne) {
            if (air.landTimeMs > currentTime) {
                stillAirborne.push_back(air);
            }
        }

        pendingMoves = remaining;
        airborne = stillAirborne;
    };

    optional<pair<int, int>> selected;

    for (const auto& cmdLine : commands) {
        auto tokens = splitWords(cmdLine);
        if (tokens.empty()) continue;

        const string& cmd = tokens[0];

        if (cmd == "jump" && tokens.size() == 3) {
            applyArrivedMoves(clockMs);
            int col = stoi(tokens[1]) / 100;
            int r = stoi(tokens[2]) / 100;
            if (board->isValid(r, col)) {
                string piece = board->getPiece(r, col);
                if (isPieceType(piece)) {
                    bool isMoving = false;
                    for (const auto& pm : pendingMoves) {
                        if (pm.fromRow == r && pm.fromCol == col) {
                            isMoving = true;
                            break;
                        }
                    }
                    bool isAirborne = false;
                    for (const auto& air : airborne) {
                        if (air.row == r && air.col == col) {
                            isAirborne = true;
                            break;
                        }
                    }
                    if (!isMoving && !isAirborne) {
                        airborne.push_back({r, col, piece, clockMs + config::JUMP_DURATION_MS});
                    }
                }
            }
            selected.reset();
        } else if (cmd == "click" && tokens.size() == 3) {
            applyArrivedMoves(clockMs);
            int col = stoi(tokens[1]) / 100;
            int r = stoi(tokens[2]) / 100;
            if (board->isValid(r, col)) {
                string clicked = board->getPiece(r, col);
                if (isPieceType(clicked)) {
                    if (!selected) {
                        selected = make_pair(r, col);
                    } else {
                        auto [selRow, selCol] = *selected;
                        string selPiece = board->getPiece(selRow, selCol);
                        if (selRow == r && selCol == col) {
                            bool isMoving = false;
                            for (const auto& pm : pendingMoves) {
                                if (pm.fromRow == r && pm.fromCol == col) {
                                    isMoving = true;
                                    break;
                                }
                            }
                            bool isAirborne = false;
                            for (const auto& air : airborne) {
                                if (air.row == r && air.col == col) {
                                    isAirborne = true;
                                    break;
                                }
                            }
                            if (!isMoving && !isAirborne) {
                                airborne.push_back({r, col, selPiece, clockMs + config::JUMP_DURATION_MS});
                            }
                            selected.reset();
                        } else if (isSameColor(selPiece, clicked)) {
                            selected = make_pair(r, col);
                        } else if (canMovePiece(selPiece, selRow, selCol, r, col, *board)) {
                            pendingMoves.push_back({selRow, selCol, r, col, selPiece, clockMs + config::MOVE_ARRIVAL_TIME_MS});
                            selected.reset();
                        }
                    }
                } else {
                    if (selected) {
                        auto [selRow, selCol] = *selected;
                        string selPiece = board->getPiece(selRow, selCol);
                        if (canMovePiece(selPiece, selRow, selCol, r, col, *board)) {
                            pendingMoves.push_back({selRow, selCol, r, col, selPiece, clockMs + config::MOVE_ARRIVAL_TIME_MS});
                            selected.reset();
                        }
                    }
                }
            }
        } else if (cmd == "wait" && tokens.size() == 2) {
            long long ms = stoll(tokens[1]);
            if (ms > 0) {
                clockMs += ms;
                applyArrivedMoves(clockMs);
            }
        } else if (cmd == "print" && tokens.size() == 2 && tokens[1] == "board") {
            applyArrivedMoves(clockMs);
            printBoard();
        }
    }

    return 0;
}

int main() {
    return runBoardCommands();
}
