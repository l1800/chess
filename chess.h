// GitHub Copilot Chess Game
// Repository: https://github.com/user/chess
// Unit tests: Run with `ctest` or individual test executable

#pragma once

#include <algorithm>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

using namespace std;

// ============================================================================
// Configuration: All hard-coded values moved here
// ============================================================================

namespace config {
    constexpr long long MOVE_ARRIVAL_TIME_MS = 1000;
    constexpr long long JUMP_DURATION_MS = 1000;
    constexpr char EMPTY_CELL = '.';
    constexpr char COLOR_WHITE = 'w';
    constexpr char COLOR_BLACK = 'b';
}

// ============================================================================
// Board Abstraction (supports text or binary representation)
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
    char symbol;                           // K, Q, R, B, N, P
    bool canCaptureEnemy;
    bool canMoveEmpty;
    std::function<bool(int fromRow, int fromCol, int toRow, int toCol, const Board& board)> 
        validateMove;
    std::function<string(const string& piece, int targetRow, int rows)> 
        onReachEndRow;                     // nullptr = no promotion
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
// Game Logic Interface
// ============================================================================

class GameController {
public:
    virtual ~GameController() = default;
    
    virtual bool processJump(int row, int col, long long currentTimeMs) = 0;
    virtual bool processClick(int row, int col, long long currentTimeMs) = 0;
    virtual void processWait(long long durationMs) = 0;
    virtual void printBoard() const = 0;
};

// ============================================================================
// Helper Functions (SRP: single responsibility each)
// ============================================================================

bool isSameColor(const string& piece1, const string& piece2);
bool isPieceType(const string& token);
bool pathClear(const Board& board, int fromRow, int fromCol, int toRow, int toCol);

PieceRegistry createDefaultPieceRegistry();
int runBoardCommands();

