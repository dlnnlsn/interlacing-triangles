#include <algorithm>
#include <iostream>
#include <tuple>
#include <vector>

using namespace std;

pair<int, int> find_move(
    int max_row,
    int to_move,
    const vector<vector<int>>& value_by_position,
    const vector<pair<int, int>>& position_by_value
) {
    const auto [row, col] = position_by_value[to_move - 1];
    for (int new_col = col - 1; new_col >= 0; --new_col) {
        if (value_by_position[row][new_col] > to_move) continue;
        if (row == max_row) {
            return pair<int, int>{row, new_col};
        }
        else {
            if (value_by_position[row + 1][new_col] > to_move) return pair<int, int>{row, new_col};
            if (value_by_position[row + 1][new_col + 1] > to_move) return pair<int, int>{row, new_col}; 
        }
    }

    for (int new_row = row - 1; new_row >= 0; --new_row) {
        for (int new_col = new_row; new_col >= 0; --new_col) {
            if (value_by_position[new_row][new_col] > to_move) continue;
            if (value_by_position[new_row + 1][new_col] > to_move) return pair<int, int>{new_row, new_col};
            if (value_by_position[new_row + 1][new_col + 1] > to_move) return pair<int, int>{new_row, new_col};
        }
    }

    return pair<int, int>{-1, -1};
}

bool next_candidate(
    int rows,
    vector<vector<int>>& value_by_position,
    vector<pair<int, int>>& position_by_value
) {
    const int max_num = (rows * (rows + 1)) / 2;
    int row = -1, col = -1;
    int to_move = 1;
    while (to_move < max_num && row == -1) {
        ++to_move;
        tie(row, col) = find_move(rows - 1, to_move, value_by_position, position_by_value);
    }
    if (row == -1) return false;

    vector<pair<int, int>> positions;
    for (int i = 1; i <= to_move; ++i) {
        const pair<int, int> pos = position_by_value[i - 1];
        if (pos.first == row && pos.second == col) continue;
        positions.push_back(pos);
    }

    sort(positions.begin(), positions.end());

    value_by_position[row][col] = to_move;
    position_by_value[to_move - 1] = pair<int, int>{row, col};
    for (int num = to_move - 1; num >= 1; --num) {
        const auto [num_row, num_col] = positions[num - 1];
        value_by_position[num_row][num_col] = num;
        position_by_value[num - 1] = positions[num - 1];
    }

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

void print_triangle(const vector<vector<int>>& triangle) {
    for (const auto& row : triangle) {
        for (const auto& cell : row) {
            cout << " " << cell;
        }
        cout << endl;
    }
    cout << endl;
}

int main() {
    cout << "Enter the number of rows in the triangle: ";
    int n; cin >> n;

    vector<vector<int>> value_by_position;
    vector<pair<int, int>> position_by_value;
    int num = 1;
    for (int row = 0; row < n; ++row) {
        value_by_position.push_back(vector<int>());
        for (int col = 0; col <= row; ++col) {
            value_by_position[row].push_back(num);
            ++num;
            position_by_value.push_back(pair<int, int>{row, col});
        }
    }

    long long int num_triangles = 0;
    if (valid(value_by_position)) ++num_triangles;

    print_triangle(value_by_position);

    while (next_candidate(n, value_by_position, position_by_value)) {
        print_triangle(value_by_position);
        cout << num_triangles << endl;
        if (valid(value_by_position)) ++num_triangles;
    }

    cout << "Number of triangles: " << num_triangles << endl;

}
