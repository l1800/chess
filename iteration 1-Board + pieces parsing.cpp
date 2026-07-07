#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <set>

using namespace std;

// Valid piece tokens
set<string> validTokens = {
    ".", 
    "wK", "wQ", "wR", "wB", "wN", "wP",
    "bK", "bQ", "bR", "bB", "bN", "bP"
};

int parseAndPrintBoard() {
    vector<vector<string>> board;
    string line;
    bool readingBoard = false;
    
    // Read input and extract board lines
    while (getline(cin, line)) {
        // Skip empty lines at the start
        if (line == "Board:") {
            readingBoard = true;
            continue;
        }
        
        // Stop when we hit Commands
        if (line == "Commands:" || line.find("Commands") == 0) {
            break;
        }
        
        // Skip lines before Board:
        if (!readingBoard) {
            continue;
        }
        
        if (line.empty()) {
            continue;
        }
        
        // Parse the row - split by spaces
        vector<string> cells;
        string cell;
        
        for (char c : line) {
            if (isspace(c)) {
                if (!cell.empty()) {
                    cells.push_back(cell);
                    cell.clear();
                }
            } else {
                cell += c;
            }
        }
        if (!cell.empty()) {
            cells.push_back(cell);
        }
        
        if (!cells.empty()) {
            board.push_back(cells);
        }
    }
    
    // Validate board is not empty
    if (board.empty()) {
        return 0;
    }
    
    // Check for unknown tokens
    for (const auto& row : board) {
        for (const auto& token : row) {
            if (validTokens.find(token) == validTokens.end()) {
                cout << "ERROR UNKNOWN_TOKEN" << endl;
                return 0;
            }
        }
    }
    
    // Validate all rows have same width
    int width = board[0].size();
    for (const auto& row : board) {
        if ((int)row.size() != width) {
            cout << "ERROR ROW_WIDTH_MISMATCH" << endl;
            return 0;
        }
    }
    
    // Output in canonical form: space-separated cells
    for (const auto& row : board) {
        for (int i = 0; i < (int)row.size(); ++i) {
            if (i > 0) cout << " ";
            cout << row[i];
        }
        cout << "\n";
    }
    
    return 0;
}
