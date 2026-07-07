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
	while (end > start && isspace((unsigned char)text[end-1])) end--;
	return text.substr(start, end-start);
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
		row += dRow; col += dCol;
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
			if (piece.empty()) return false;
			char color = piece[0];
			// white moves up (decreasing row), black moves down (increasing row)
			if (color == 'w') {
				if (dRow == -1 && dCol == 0 && board[toRow][toCol] == ".") return true; // forward
				if (dRow == -1 && absCol == 1 && board[toRow][toCol] != "." && !sameColor(piece, board[toRow][toCol])) return true; // capture
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

int main() {
	vector<vector<string>> board;
	vector<string> commands;
	string line;
	bool readingBoard = false;
	bool readingCommands = false;

	while (getline(cin, line)) {
		string t = trim(line);
		if (t == "Board:") { readingBoard = true; readingCommands = false; continue; }
		if (t == "Commands:" || t.rfind("Commands",0) == 0) { readingBoard = false; readingCommands = true; continue; }
		if (readingBoard) {
			if (t.empty()) continue;
			auto row = splitWords(t);
			if (!row.empty()) board.push_back(row);
		} else if (readingCommands) {
			if (!t.empty()) commands.push_back(t);
		}
	}

	if (board.empty()) return 0;

	int rows = board.size();
	int cols = board[0].size();
	for (const auto& r : board) {
		if ((int)r.size() != cols) { cout << "ERROR ROW_WIDTH_MISMATCH" << endl; return 0; }
		for (const auto& tok : r) if (!isValidToken(tok)) { cout << "ERROR UNKNOWN_TOKEN" << endl; return 0; }
	}

	struct Pos { int row; int col; };
	optional<Pos> selected;
	long long clockMs = 0;
	struct Pending { int fr, fc, tr, tc; string piece; long long arrival; };
	vector<Pending> pending;
	const long long MOVE_TIME = 2000; // ms

	auto applyArrived = [&]() {
		if (pending.empty()) return;
		vector<Pending> remain;
		for (auto &p : pending) {
			if (p.arrival <= clockMs) {
				// apply only if piece still at source
				if (board[p.fr][p.fc] == p.piece) {
					board[p.tr][p.tc] = p.piece;
					board[p.fr][p.fc] = ".";
				}
			} else remain.push_back(p);
		}
		pending.swap(remain);
	};

	auto inBoard = [&](int c, int r){ return c>=0 && c<cols && r>=0 && r<rows; };
	auto printBoard = [&]() {
		// Before printing, ensure arrived moves applied for current clock
		applyArrived();
		for (const auto& r : board) {
			for (int i=0;i<(int)r.size();++i) { if (i) cout << ' '; cout << r[i]; }
			cout << '\n';
		}
	};

	for (const auto& cmdLine : commands) {
		auto tok = splitWords(cmdLine);
		if (tok.empty()) continue;
		if (tok[0] == "click" && tok.size()==3) {
			// apply any arrived moves first
			applyArrived();
			int x = stoi(tok[1]);
			int y = stoi(tok[2]);
			int col = x / 100;
			int row = y / 100;
			if (!inBoard(col,row)) continue;
			const string &clicked = board[row][col];
			if (isPiece(clicked)) {
				if (!selected.has_value()) selected = Pos{row,col};
				else {
					const string selTok = board[selected->row][selected->col];
					if (sameColor(selTok, clicked)) selected = Pos{row,col};
					else {
						// attempt capture move
						if (canMovePiece(selTok, selected->row, selected->col, row, col, board)) {
							// prevent redirecting a piece that's already moving
							bool alreadyMoving = false;
							for (auto &pm : pending) if (pm.fr == selected->row && pm.fc == selected->col) { alreadyMoving = true; break; }
							if (!alreadyMoving) {
								Pending p{selected->row, selected->col, row, col, selTok, clockMs + MOVE_TIME};
								pending.push_back(p);
								selected.reset();
							}
						}
					}
				}
			} else {
				if (selected.has_value()) {
					const string selTok = board[selected->row][selected->col];
					if (canMovePiece(selTok, selected->row, selected->col, row, col, board)) {
						bool alreadyMoving = false;
						for (auto &pm : pending) if (pm.fr == selected->row && pm.fc == selected->col) { alreadyMoving = true; break; }
						if (!alreadyMoving) {
							Pending p{selected->row, selected->col, row, col, selTok, clockMs + MOVE_TIME};
							pending.push_back(p);
							selected.reset();
						}
					}
				}
			}
		} else if (tok[0] == "wait" && tok.size()==2) {
			int ms = stoi(tok[1]);
			if (ms>0) {
				clockMs += ms;
				applyArrived();
			}
		} else if (tok[0] == "print" && tok.size()==2 && tok[1]=="board") {
			printBoard();
		}
	}

	return 0;
}

