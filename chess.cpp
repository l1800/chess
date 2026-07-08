#include "chess.h"
#include <cctype>
#include <iostream>
#include <sstream>

using namespace std;

// ============================================================================
// Helper Functions Implementation
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
// Movement Rules Factory (extensible for custom games)
// ============================================================================

PieceRegistry createDefaultPieceRegistry() {
    PieceRegistry registry;

    // King: moves 1 cell in any direction
    registry.registerPiece({
        'K',
        true, true,
        [](int fr, int fc, int tr, int tc, const Board&) {
            int dRow = abs(tr - fr);
            int dCol = abs(tc - fc);
            return dRow <= 1 && dCol <= 1 && (dRow + dCol > 0);
        },
        nullptr  // no promotion
    });

    // Knight: L-shaped move
    registry.registerPiece({
        'N',
        true, true,
        [](int fr, int fc, int tr, int tc, const Board&) {
            int dRow = abs(tr - fr);
            int dCol = abs(tc - fc);
            return (dRow == 1 && dCol == 2) || (dRow == 2 && dCol == 1);
        },
        nullptr
    });

    // Rook: horizontal or vertical
    registry.registerPiece({
        'R',
        true, true,
        [](int fr, int fc, int tr, int tc, const Board& board) {
            if (fr == tr || fc == tc) return pathClear(board, fr, fc, tr, tc);
            return false;
        },
        nullptr
    });

    // Bishop: diagonal
    registry.registerPiece({
        'B',
        true, true,
        [](int fr, int fc, int tr, int tc, const Board& board) {
            if (abs(tr - fr) == abs(tc - fc)) return pathClear(board, fr, fc, tr, tc);
            return false;
        },
        nullptr
    });

    // Queen: rook + bishop
    registry.registerPiece({
        'Q',
        true, true,
        [](int fr, int fc, int tr, int tc, const Board& board) {
            if (fr == tr || fc == tc || abs(tr - fr) == abs(tc - fc)) {
                return pathClear(board, fr, fc, tr, tc);
            }
            return false;
        },
        nullptr
    });

    // Pawn: complex movement (forward, capture diagonal, double move from start, promotion)
    registry.registerPiece({
        'P',
        true, true,
        [](int fr, int fc, int tr, int tc, const Board& board) {
            // This is delegated to canMovePiece for color-aware logic
            return true;  // Validation happens in game logic
        },
        [](const string& piece, int row, int rows) {
            // Promote pawn to queen at end row
            return string(1, piece[0]) + "Q";
        }
    });

    return registry;
}

// ============================================================================
// Pawn Movement (color-aware, isolated for clarity)
// ============================================================================

bool canMovePawn(const string& piece, int fromRow, int fromCol, int toRow, int toCol, const Board& board) {
    int dRow = toRow - fromRow;
    int dCol = toCol - fromCol;
    int absCol = abs(dCol);
    int rows = board.getRows();

    char color = piece[0];
    string empty(1, config::EMPTY_CELL);

    if (color == config::COLOR_WHITE) {
        // White moves up (decreasing row)
        if (dCol == 0 && dRow == -1 && board.getPiece(toRow, toCol) == empty) return true;
        // Double move from start row
        if (dCol == 0 && dRow == -2 && fromRow == rows - 1 && 
            board.getPiece(fromRow - 1, fromCol) == empty && 
            board.getPiece(toRow, toCol) == empty) return true;
        // Diagonal capture
        if (dRow == -1 && absCol == 1 && board.getPiece(toRow, toCol) != empty &&
            !isSameColor(piece, board.getPiece(toRow, toCol))) return true;
        return false;
    } else if (color == config::COLOR_BLACK) {
        // Black moves down (increasing row)
        if (dCol == 0 && dRow == 1 && board.getPiece(toRow, toCol) == empty) return true;
        // Double move from start row
        if (dCol == 0 && dRow == 2 && fromRow == 0 && 
            board.getPiece(fromRow + 1, fromCol) == empty && 
            board.getPiece(toRow, toCol) == empty) return true;
        // Diagonal capture
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

    // Can't capture own piece
    if (targetPiece != empty && isSameColor(piece, targetPiece)) return false;

    if (piece.length() < 2) return false;
    char type = piece[1];

    if (type == 'P') return canMovePawn(piece, fromRow, fromCol, toRow, toCol, board);

    // Use registry for other pieces (extensible approach)
    PieceRegistry registry = createDefaultPieceRegistry();
    if (auto def = registry.getPiece(type)) {
        return def->validateMove(fromRow, fromCol, toRow, toCol, board);
    }

    return false;
}

bool isPromotionRow(int row, const string& piece, int rows) {
    if (piece.length() < 2) return false;
    char type = piece[1];
    return (type == 'P' && piece[0] == config::COLOR_WHITE && row == 0) ||
           (type == 'P' && piece[0] == config::COLOR_BLACK && row == rows - 1);
}

// ============================================================================
// Board Validation
// ============================================================================

bool isValidToken(const string& token, const PieceRegistry& registry) {
    if (token == string(1, config::EMPTY_CELL)) return true;
    if (token.length() != 2) return false;

    char color = token[0];
    char symbol = token[1];

    if (color != config::COLOR_WHITE && color != config::COLOR_BLACK) return false;
    return registry.getPiece(symbol).has_value();
}

// ============================================================================
// Game Implementation
// ============================================================================

class ChessGame : public GameController {
private:
    unique_ptr<Board> board;
    vector<PendingMove> pendingMoves;
    vector<AirborneState> airborne;
    long long clockMs = 0;
    PieceRegistry registry;

    struct Selection {
        optional<pair<int, int>> pos;
    } selection;

    void applyArrivedMoves(long long currentTime) {
        vector<PendingMove> remaining;
        sort(pendingMoves.begin(), pendingMoves.end(),
             [](const PendingMove& a, const PendingMove& b) { return a.arrivalTimeMs < b.arrivalTimeMs; });

        string empty(1, config::EMPTY_CELL);

        for (const auto& pm : pendingMoves) {
            if (pm.arrivalTimeMs <= currentTime) {
                string source = board->getPiece(pm.fromRow, pm.fromCol);
                if (source != pm.piece) continue;  // Source changed

                // Check if airborne enemy is at destination
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
                if (target != empty && isSameColor(pm.piece, target)) continue;  // Can't move there

                board->setPiece(pm.toRow, pm.toCol, pm.piece);
                board->setPiece(pm.fromRow, pm.fromCol, empty);

                if (isPromotionRow(pm.toRow, pm.piece, board->getRows())) {
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
    }

    bool isAirborneAt(int row, int col) const {
        for (const auto& air : airborne) {
            if (air.row == row && air.col == col) return true;
        }
        return false;
    }

    bool hasPendingMoveFrom(int row, int col) const {
        for (const auto& pm : pendingMoves) {
            if (pm.fromRow == row && pm.fromCol == col) return true;
        }
        return false;
    }

public:
    ChessGame(const vector<vector<string>>& boardData, const PieceRegistry& reg)
        : board(make_unique<TextBoard>(boardData)), registry(reg) {}

    bool processJump(int row, int col, long long currentTimeMs) override {
        applyArrivedMoves(currentTimeMs);

        string piece = board->getPiece(row, col);
        if (isPieceType(piece) && !hasPendingMoveFrom(row, col) && !isAirborneAt(row, col)) {
            airborne.push_back({row, col, piece, currentTimeMs + config::JUMP_DURATION_MS});
        }
        selection.pos.reset();
        return true;
    }

    bool processClick(int row, int col, long long currentTimeMs) override {
        applyArrivedMoves(currentTimeMs);

        string clicked = board->getPiece(row, col);

        if (isPieceType(clicked)) {
            if (!selection.pos) {
                selection.pos = make_pair(row, col);
            } else {
                auto [selRow, selCol] = *selection.pos;
                string selPiece = board->getPiece(selRow, selCol);

                if (selRow == row && selCol == col) {
                    // Double-click to jump
                    if (!hasPendingMoveFrom(row, col) && !isAirborneAt(row, col)) {
                        airborne.push_back({row, col, selPiece, currentTimeMs + config::JUMP_DURATION_MS});
                    }
                    selection.pos.reset();
                } else if (isSameColor(selPiece, clicked)) {
                    // Reselect own piece
                    selection.pos = make_pair(row, col);
                } else if (canMovePiece(selPiece, selRow, selCol, row, col, *board)) {
                    // Move to enemy
                    pendingMoves.push_back({selRow, selCol, row, col, selPiece, currentTimeMs + config::MOVE_ARRIVAL_TIME_MS});
                    selection.pos.reset();
                }
            }
        } else {
            if (selection.pos) {
                auto [selRow, selCol] = *selection.pos;
                string selPiece = board->getPiece(selRow, selCol);
                if (canMovePiece(selPiece, selRow, selCol, row, col, *board)) {
                    pendingMoves.push_back({selRow, selCol, row, col, selPiece, currentTimeMs + config::MOVE_ARRIVAL_TIME_MS});
                    selection.pos.reset();
                }
            }
        }
        return true;
    }

    void processWait(long long durationMs) override {
        if (durationMs > 0) {
            clockMs += durationMs;
            applyArrivedMoves(clockMs);
        }
    }

    void printBoard() const override {
        TextBoard* textBoard = dynamic_cast<TextBoard*>(board.get());
        if (!textBoard) return;

        const auto& data = textBoard->getRawData();
        for (const auto& row : data) {
            for (size_t i = 0; i < row.size(); ++i) {
                if (i) cout << " ";
                cout << row[i];
            }
            cout << '\n';
        }
    }

    long long getTime() const { return clockMs; }
};

// ============================================================================
// Input Processing
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

    PieceRegistry registry = createDefaultPieceRegistry();

    // Validate board
    for (const auto& row : boardData) {
        if ((int)row.size() != cols) {
            cout << "ERROR ROW_WIDTH_MISMATCH" << endl;
            return 0;
        }
        for (const auto& token : row) {
            if (!isValidToken(token, registry)) {
                cout << "ERROR UNKNOWN_TOKEN" << endl;
                return 0;
            }
        }
    }

    ChessGame game(boardData, registry);

    for (const auto& cmdLine : commands) {
        auto tokens = splitWords(cmdLine);
        if (tokens.empty()) continue;

        const string& cmd = tokens[0];

        if (cmd == "jump" && tokens.size() == 3) {
            int col = stoi(tokens[1]) / 100;
            int row = stoi(tokens[2]) / 100;
            if (row >= 0 && row < rows && col >= 0 && col < cols) {
                game.processJump(row, col, game.getTime());
            }
        } else if (cmd == "click" && tokens.size() == 3) {
            int col = stoi(tokens[1]) / 100;
            int row = stoi(tokens[2]) / 100;
            if (row >= 0 && row < rows && col >= 0 && col < cols) {
                game.processClick(row, col, game.getTime());
            }
        } else if (cmd == "wait" && tokens.size() == 2) {
            long long ms = stoll(tokens[1]);
            if (ms > 0) {
                game.processWait(ms);
            }
        } else if (cmd == "print" && tokens.size() == 2 && tokens[1] == "board") {
            game.printBoard();
        }
    }

    return 0;
}
