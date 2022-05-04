#include <iostream>
#include <vector>

using namespace std;

bool next_candidate(int rows, vector<vector<int>>& value_by_position, vector<pair<int, int>>& position_by_value, vector<int>& above) {
    const int max_num = (rows * (rows + 1)) / 2;
    bool move_found = false;
    int to_move = 1;
    int move_i = -1, move_j = -1, move_above = -1;

    while (to_move <= max_num && !move_found) {
        auto [i, j] = position_by_value[to_move - 1];
        if (i == rows) {
           for (int new_j = j - 1; j >= 0; --j) {
               if (value_by_position[i - 1][new_j - 1] < to_move) {
                    move_i = i;
                    move_j = new_j;
                    move_above = max_num + 1;
                    move_found = true;
                    break;
               } 
           }
        }
        if (move_found) break;

        if (i < rows) {
            auto [above_i, above_j] = position_by_value[above[to_move - 1] - 1];
            if (above_j == j - 1) {
                if (j < i - 1 && value_by_position[i - 2][j] < to_move) {
                   move_i = i - 1; 
                   move_j = j + 1;
                   move_above = above[to_move - 1];
                   move_found = true;
                }
            }
        }
        if (move_found) break;

        for (int candidate_above = min(max_num, above[to_move - 1]); candidate_above > to_move; --candidate_above) {            
            auto [above_i, above_j] = position_by_value[candidate_above - 1];
            if (above_i == 1) continue;
            if (above_j > 1 && value_by_position[above_i - 2][above_j - 2] < to_move) {
                move_i = above_i - 1;
                move_j = above_j - 1;
                move_above = candidate_above;
                move_found = true;
                break;
            }
            if (above_j < above_i && value_by_position[above_i - 2][above_j - 1] < to_move) {
                move_i = above_i - 1;
                move_j = above_j;
                move_above = candidate_above;
                move_found = true;
                break;
            }
        }
    } 
    if (to_move == max_num && !move_found) return false; 



    return true;
}


bool valid(vector<vector<int>> candidate) {
    for (int i = 0; i < candidate.size() - 1; ++i) {
        for (int j = 0; j < candidate[i].size(); ++j) {
            if ((candidate[i][j] < candidate[i + 1][j]) == (candidate[i][j] < candidate[i + 1][j + 1])) {
                return false;
            }
        }
    }
    return true;
}

int main() {
    cout << "Enter the number of rows in the triangle: ";
    int n; cin >> n;
}
