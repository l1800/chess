// ============================================================================
// Unit Tests for Chess Game
// Framework: Google Test (gtest)
// Run: ctest --verbose
// Coverage: All critical functions (target 100%)
// ============================================================================

#include <gtest/gtest.h>
#include "chess.h"
#include <memory>

// ============================================================================
// Test Fixtures
// ============================================================================

class BoardTest : public ::testing::Test {
protected:
    unique_ptr<TextBoard> board;

    void SetUp() override {
        vector<vector<string>> data = {
            {".", ".", "."},
            {".", "wK", "."},
            {".", ".", "."}
        };
        board = make_unique<TextBoard>(data);
    }
};

class PieceValidationTest : public ::testing::Test {
protected:
    unique_ptr<TextBoard> board;
    PieceRegistry registry;

    void SetUp() override {
        vector<vector<string>> data = {
            {".", "bR", "."},
            {".", ".", "."},
            {"wP", ".", "."}
        };
        board = make_unique<TextBoard>(data);
        registry = createDefaultPieceRegistry();
    }
};

// ============================================================================
// Board Abstraction Tests (SRP: Board manages storage)
// ============================================================================

TEST_F(BoardTest, GetValidPiece) {
    EXPECT_EQ(board->getPiece(1, 1), "wK");
}

TEST_F(BoardTest, GetEmptyCell) {
    EXPECT_EQ(board->getPiece(0, 0), ".");
}

TEST_F(BoardTest, SetPiece) {
    board->setPiece(0, 0, "wP");
    EXPECT_EQ(board->getPiece(0, 0), "wP");
}

TEST_F(BoardTest, GetDimensions) {
    EXPECT_EQ(board->getRows(), 3);
    EXPECT_EQ(board->getCols(), 3);
}

TEST_F(BoardTest, IsValidBoundary) {
    EXPECT_TRUE(board->isValid(0, 0));
    EXPECT_TRUE(board->isValid(2, 2));
    EXPECT_FALSE(board->isValid(-1, 0));
    EXPECT_FALSE(board->isValid(3, 0));
}

TEST_F(BoardTest, SetOutOfBounds) {
    board->setPiece(5, 5, "wQ");
    EXPECT_EQ(board->getPiece(5, 5), "");  // Should not be set
}

// ============================================================================
// Helper Functions Tests (SRP: each function does one thing)
// ============================================================================

TEST(HelperFunctions, IsSameColorWhite) {
    EXPECT_TRUE(isSameColor("wK", "wP"));
    EXPECT_FALSE(isSameColor("wK", "bP"));
}

TEST(HelperFunctions, IsSameColorBlack) {
    EXPECT_TRUE(isSameColor("bK", "bR"));
    EXPECT_FALSE(isSameColor("bK", "wK"));
}

TEST(HelperFunctions, IsSameColorEmpty) {
    EXPECT_FALSE(isSameColor("", "wK"));
    EXPECT_FALSE(isSameColor("wK", ""));
}

TEST(HelperFunctions, IsPieceType) {
    EXPECT_TRUE(isPieceType("wK"));
    EXPECT_TRUE(isPieceType("bP"));
    EXPECT_FALSE(isPieceType("."));
}

TEST(HelperFunctions, TrimWhitespace) {
    EXPECT_EQ(trim("  hello  "), "hello");
    EXPECT_EQ(trim("\t\nworld\n\t"), "world");
    EXPECT_EQ(trim(""), "");
}

TEST(HelperFunctions, SplitWords) {
    auto tokens = splitWords("click 100 200");
    EXPECT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0], "click");
    EXPECT_EQ(tokens[1], "100");
}

// ============================================================================
// Path Clearing Tests (SRP: validates movement corridors)
// ============================================================================

TEST(PathClear, HorizontalPathClear) {
    vector<vector<string>> data = {
        {"wR", ".", ".", "bK"}
    };
    TextBoard board(data);
    EXPECT_TRUE(pathClear(board, 0, 0, 0, 3));
}

TEST(PathClear, HorizontalPathBlocked) {
    vector<vector<string>> data = {
        {"wR", ".", "wP", "bK"}
    };
    TextBoard board(data);
    EXPECT_FALSE(pathClear(board, 0, 0, 0, 3));
}

TEST(PathClear, VerticalPathClear) {
    vector<vector<string>> data = {
        {"wR"},
        {"."},
        {"."},
        {"bK"}
    };
    TextBoard board(data);
    EXPECT_TRUE(pathClear(board, 0, 0, 3, 0));
}

TEST(PathClear, DiagonalPathClear) {
    vector<vector<string>> data = {
        {"wB", ".", "."},
        {".", ".", "."},
        {".", ".", "bK"}
    };
    TextBoard board(data);
    EXPECT_TRUE(pathClear(board, 0, 0, 2, 2));
}

// ============================================================================
// Movement Validation Tests (SRP: each piece type tested independently)
// ============================================================================

TEST_F(PieceValidationTest, KingValidMove) {
    vector<vector<string>> data = {
        {"wK", ".", "."},
        {".", ".", "."},
        {".", ".", "."}
    };
    TextBoard testBoard(data);
    EXPECT_TRUE(canMovePiece("wK", 0, 0, 0, 1, testBoard));
    EXPECT_TRUE(canMovePiece("wK", 0, 0, 1, 1, testBoard));
}

TEST_F(PieceValidationTest, KingInvalidMove) {
    vector<vector<string>> data = {
        {"wK", ".", "."},
        {".", ".", "."},
        {".", ".", "."}
    };
    TextBoard testBoard(data);
    EXPECT_FALSE(canMovePiece("wK", 0, 0, 0, 2, testBoard));
}

TEST_F(PieceValidationTest, KnightValidMove) {
    vector<vector<string>> data = {
        {"wN", ".", "."},
        {".", ".", "."},
        {".", ".", "."}
    };
    TextBoard testBoard(data);
    EXPECT_TRUE(canMovePiece("wN", 0, 0, 1, 2, testBoard));
}

TEST_F(PieceValidationTest, RookValidHorizontal) {
    vector<vector<string>> data = {
        {"wR", ".", ".", "."}
    };
    TextBoard testBoard(data);
    EXPECT_TRUE(canMovePiece("wR", 0, 0, 0, 3, testBoard));
}

TEST_F(PieceValidationTest, RookValidVertical) {
    vector<vector<string>> data = {
        {"wR"},
        {"."},
        {"."},
        {"."}
    };
    TextBoard testBoard(data);
    EXPECT_TRUE(canMovePiece("wR", 0, 0, 3, 0, testBoard));
}

TEST_F(PieceValidationTest, BishopValidDiagonal) {
    vector<vector<string>> data = {
        {"wB", ".", "."},
        {".", ".", "."},
        {".", ".", "."}
    };
    TextBoard testBoard(data);
    EXPECT_TRUE(canMovePiece("wB", 0, 0, 2, 2, testBoard));
}

TEST_F(PieceValidationTest, QueenValidHorizontal) {
    vector<vector<string>> data = {
        {"wQ", ".", "."}
    };
    TextBoard testBoard(data);
    EXPECT_TRUE(canMovePiece("wQ", 0, 0, 0, 2, testBoard));
}

TEST_F(PieceValidationTest, QueenValidDiagonal) {
    vector<vector<string>> data = {
        {"wQ", ".", "."},
        {".", ".", "."},
        {".", ".", "."}
    };
    TextBoard testBoard(data);
    EXPECT_TRUE(canMovePiece("wQ", 0, 0, 2, 2, testBoard));
}

// ============================================================================
// Pawn Movement Tests (Complex logic isolated for testing)
// ============================================================================

TEST(PawnMovement, WhitePawnForwardOne) {
    vector<vector<string>> data = {
        {".", "."},
        {".", "."},
        {"wP", "."}
    };
    TextBoard board(data);
    EXPECT_TRUE(canMovePiece("wP", 2, 0, 1, 0, board));
}

TEST(PawnMovement, WhitePawnForwardTwo) {
    vector<vector<string>> data = {
        {".", "."},
        {".", "."},
        {"wP", "."}
    };
    TextBoard board(data);
    EXPECT_TRUE(canMovePiece("wP", 2, 0, 0, 0, board));
}

TEST(PawnMovement, WhitePawnDoubleBlockedByPiece) {
    vector<vector<string>> data = {
        {".", "."},
        {"bP", "."},
        {"wP", "."}
    };
    TextBoard board(data);
    EXPECT_FALSE(canMovePiece("wP", 2, 0, 0, 0, board));
}

TEST(PawnMovement, WhitePawnDiagonalCapture) {
    vector<vector<string>> data = {
        {".", "."},
        {".", "bP"},
        {"wP", "."}
    };
    TextBoard board(data);
    EXPECT_TRUE(canMovePiece("wP", 2, 0, 1, 1, board));
}

TEST(PawnMovement, WhitePawnCannotCaptureOwn) {
    vector<vector<string>> data = {
        {".", "."},
        {".", "wQ"},
        {"wP", "."}
    };
    TextBoard board(data);
    EXPECT_FALSE(canMovePiece("wP", 2, 0, 1, 1, board));
}

TEST(PawnMovement, BlackPawnForwardOne) {
    vector<vector<string>> data = {
        {"bP", "."},
        {".", "."},
        {".", "."}
    };
    TextBoard board(data);
    EXPECT_TRUE(canMovePiece("bP", 0, 0, 1, 0, board));
}

TEST(PawnMovement, BlackPawnForwardTwo) {
    vector<vector<string>> data = {
        {"bP", "."},
        {".", "."},
        {".", "."}
    };
    TextBoard board(data);
    EXPECT_TRUE(canMovePiece("bP", 0, 0, 2, 0, board));
}

// ============================================================================
// Promotion Tests (SRP: promotion logic isolated)
// ============================================================================

TEST(Promotion, WhitePawnPromotesAtRow0) {
    EXPECT_TRUE(isPromotionRow(0, "wP", 8));
}

TEST(Promotion, BlackPawnPromotesAtLastRow) {
    EXPECT_TRUE(isPromotionRow(7, "bP", 8));
}

TEST(Promotion, PawnNoPromotionMidboard) {
    EXPECT_FALSE(isPromotionRow(3, "wP", 8));
    EXPECT_FALSE(isPromotionRow(4, "bP", 8));
}

TEST(Promotion, NonPawnNoPromotion) {
    EXPECT_FALSE(isPromotionRow(0, "wK", 8));
    EXPECT_FALSE(isPromotionRow(7, "bR", 8));
}

// ============================================================================
// Token Validation Tests (SRP: validation isolated)
// ============================================================================

TEST(TokenValidation, ValidPiece) {
    PieceRegistry registry = createDefaultPieceRegistry();
    EXPECT_TRUE(isValidToken("wK", registry));
    EXPECT_TRUE(isValidToken("bP", registry));
    EXPECT_TRUE(isValidToken(".", registry));
}

TEST(TokenValidation, InvalidColor) {
    PieceRegistry registry = createDefaultPieceRegistry();
    EXPECT_FALSE(isValidToken("rK", registry));
}

TEST(TokenValidation, InvalidSymbol) {
    PieceRegistry registry = createDefaultPieceRegistry();
    EXPECT_FALSE(isValidToken("wX", registry));
}

TEST(TokenValidation, InvalidLength) {
    PieceRegistry registry = createDefaultPieceRegistry();
    EXPECT_FALSE(isValidToken("wKQ", registry));
}

// ============================================================================
// Piece Registry Tests (Extensibility for custom games)
// ============================================================================

TEST(PieceRegistry, RegisterAndRetrieve) {
    PieceRegistry registry;
    PieceDefinition def = {'K', true, true, nullptr, nullptr};
    registry.registerPiece(def);
    auto retrieved = registry.getPiece('K');
    EXPECT_TRUE(retrieved.has_value());
}

TEST(PieceRegistry, UnregisteredPiece) {
    PieceRegistry registry;
    auto retrieved = registry.getPiece('X');
    EXPECT_FALSE(retrieved.has_value());
}

TEST(PieceRegistry, DefaultRegistryHasAllPieces) {
    PieceRegistry registry = createDefaultPieceRegistry();
    EXPECT_TRUE(registry.getPiece('K').has_value());
    EXPECT_TRUE(registry.getPiece('Q').has_value());
    EXPECT_TRUE(registry.getPiece('R').has_value());
    EXPECT_TRUE(registry.getPiece('B').has_value());
    EXPECT_TRUE(registry.getPiece('N').has_value());
    EXPECT_TRUE(registry.getPiece('P').has_value());
}

// ============================================================================
// Edge Cases and Boundary Tests
// ============================================================================

TEST(EdgeCases, MoveSameCell) {
    vector<vector<string>> data = {
        {"wK"}
    };
    TextBoard board(data);
    EXPECT_FALSE(canMovePiece("wK", 0, 0, 0, 0, board));
}

TEST(EdgeCases, MoveOutOfBounds) {
    vector<vector<string>> data = {
        {"wK", "."},
        {".", "."}
    };
    TextBoard board(data);
    EXPECT_FALSE(canMovePiece("wK", 0, 0, 2, 2, board));
}

TEST(EdgeCases, InvalidPieceFormat) {
    vector<vector<string>> data = {
        {".", "."}
    };
    TextBoard board(data);
    EXPECT_FALSE(canMovePiece("", 0, 0, 0, 1, board));
    EXPECT_FALSE(canMovePiece("w", 0, 0, 0, 1, board));
}

