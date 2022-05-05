#include <algorithm>
#include <boost/multiprecision/cpp_int.hpp>
#include <future>
#include <iostream>
#include <mutex>
#include <tuple>
#include <vector>

using namespace std;
using namespace boost::multiprecision;

int factorials[11] = {1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800};

mutex output_mutex;

void print_triangle(int128_t num_valid, int num_rows, int values[10][10]) {
   output_mutex.lock();
   cout << num_valid << endl;
   for (int row = 0; row < num_rows; ++row) {
        for (int col = 0; col <= row; ++col) {
            cout << " " << values[row][col];
        }
        cout << endl;
   } 
   cout << endl;
   output_mutex.unlock();
}

class interlacing_thread {
    private:
        int values[10][10];
        pair<int, int> positions[100];
        pair<int, int> latest_positions[100];
        pair<int, int> earliest_positions[100];
        int num_rows;
        int max_num;

        void update_latest_and_earliest_positions() {
            this->latest_positions[0] = this->positions[0];
            this->earliest_positions[0] = this->positions[0];
            for (int num = 1; num <= this->max_num; ++num) {
                this->latest_positions[num] = max(this->latest_positions[num - 1], this->positions[num]);
                this->earliest_positions[num] = min(this->earliest_positions[num - 1], this->positions[num]);
            }
        }

        pair<int, int> find_move(int to_move) {
            pair<int, int> max_pos{-1, -1};

            if (this->latest_positions[to_move].first == this->earliest_positions[to_move].first) {
                return max_pos;
            }

            pair<int, int> neighbour_pos = this->positions[to_move + 1];
            pair<int, int> current_pos = this->positions[to_move];

            for (int replaces = 0; replaces < to_move; ++replaces) {
                pair<int, int> new_pos = this->positions[replaces];
                
                if (new_pos.first == neighbour_pos.first) {
                    if (new_pos.second > neighbour_pos.second - 2) {
                        continue;
                    }
                }

                if (new_pos > current_pos) continue;

                if (new_pos.first == this->num_rows - 1) {
                    max_pos = max(max_pos, new_pos);
                }
                else {
                    if ((to_move >= this->values[new_pos.first + 1][new_pos.second])
                     != (to_move >= this->values[new_pos.first + 1][new_pos.second + 1])) {
                        max_pos = max(max_pos, new_pos);
                    }
                }
            }

            return max_pos;
        }

        bool next_candidate() {

            int to_move = -1;
            pair<int, int> new_pos;
            for (int candidate = 1; candidate < this->max_num - 1; ++candidate) {
                new_pos = this->find_move(candidate);
                if (new_pos.first != -1) {
                    to_move = candidate;     
                    break; 
                }
            }
            if (to_move == -1) return false;

            pair<int, int> positions[100];
            int num_positions = 0;

            for (int i = 0; i <= to_move; ++i) {
                if (this->positions[i] == new_pos) continue;
                positions[num_positions++] = this->positions[i]; 
            }
            sort(positions, positions + num_positions);

            this->set_value(new_pos, to_move);
            
            for (int num = to_move - 1; num >= 0; --num) {
                this->set_value(positions[num], num);
            }

            if (this->positions[to_move - 1].first == new_pos.first) {
                if (this->positions[to_move - 1].second > new_pos.second) {
                    for (int num = to_move - 1; num >= 0; --num) {
                        if (positions[num].first < new_pos.first) {
                            this->set_value(positions[num], to_move - 1);
                            for (int i = num + 1; i < to_move; ++i) {
                                this->set_value(positions[i], i - 1);
                            }
                            break;
                        }
                    }
                }
            }

            this->update_latest_and_earliest_positions();

            return true;
        }

        int128_t num_valid() {
            for (int row = 0; row < this->num_rows - 1; ++row) {
                for (int col = 0; col <= row; ++col) {
                    if ((this->values[row][col] > this->values[row + 1][col])
                     == (this->values[row][col] > this->values[row + 1][col + 1])) {
                        return 0;
                    }
                }
            }

            int num_consecutive = 0;
            int current_row = this->positions[this->max_num].first;
            int128_t result = 1;

            for (int num = this->max_num; num >= 0; --num) {
                if (this->positions[num].first == current_row) ++num_consecutive;
                else {
                    current_row = this->positions[num].first;
                    result = result * factorials[num_consecutive];
                    num_consecutive = 1;
                }
            }

            return result * factorials[num_consecutive];
        }

        void set_value(pair<int, int> pos, int value) {
            this->values[pos.first][pos.second] = value;
            this->positions[value] = pos;
        }

        void set_value(int row, int col, int value) {
            this->values[row][col] = value;
            this->positions[value] = pair<int, int>{row, col};
        }

    public:
        interlacing_thread(int num_rows, int max_pos, int second_row, int second_col) {

            this->num_rows = num_rows;
            const int max_num = (num_rows * (num_rows + 1)) / 2 - 1;
            this->max_num = max_num;

            int num = max_num;

            this->set_value(num_rows - 1, max_pos, num--);
            this->set_value(second_row, second_col, num--);

            int start_row = num_rows - 1;
            if (second_row == num_rows - 1) {
                this->set_value(num_rows - 2, num_rows - 2, num--);
                for (int col = num_rows - 1; col >= 0; --col) {
                    if (col == max_pos) continue;
                    if (col == second_col) continue;
                    this->set_value(num_rows - 1, col, num--);
                }
                for (int col = num_rows - 3; col >= 0; --col) {
                    this->set_value(num_rows - 2, col, num--);
                }
                start_row = num_rows - 3;
            }

            for (int row = start_row; row >= 0; --row) {
                for (int col = row; col >= 0; --col) {
                    if (row == num_rows - 1 && col == max_pos) continue;
                    if (row == second_row && col == second_col) continue;        
                    this->set_value(row, col, num--);
                }
            }

            this->update_latest_and_earliest_positions();
        }

        void operator()(promise<int128_t> result_promise) {
            int128_t total = this->num_valid();
            //print_triangle(this->num_valid(), this->num_rows, this->values);
            while (this->next_candidate()) {
                //print_triangle(this->num_valid(), this->num_rows, this->values);
                total += this->num_valid();
            }
            result_promise.set_value(total);
        }
};

int main() {
    cout << "Enter the number of rows in the triangle: ";
    int n; cin >> n;

    vector<future<int128_t>> futures;
    vector<thread> threads;

    for (int max_pos = 0; max_pos < n; ++max_pos) {
        for (int second_col = 0; second_col < max_pos - 1; ++second_col) {
            promise<int128_t> instance_promise;
            futures.push_back(instance_promise.get_future());
            threads.push_back(thread(interlacing_thread(n, max_pos, n - 1, second_col), move(instance_promise)));
        }
        
        if (max_pos < n - 1) {
            promise<int128_t> left_promise;
            futures.push_back(left_promise.get_future());
            threads.push_back(thread(interlacing_thread(n, max_pos, n - 2, max_pos), move(left_promise)));
        }

        if (max_pos > 0) {
            promise<int128_t> right_promise;
            futures.push_back(right_promise.get_future());
            threads.push_back(thread(interlacing_thread(n, max_pos, n - 2, max_pos - 1), move(right_promise)));
        }
    }

    int128_t num_triangles = 0;

    for (int i = 0; i < threads.size(); ++i) {
        num_triangles += futures[i].get();
        threads[i].join();
    }

    cout << "Number of triangles: " << num_triangles << endl;

}
