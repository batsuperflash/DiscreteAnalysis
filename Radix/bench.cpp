#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdlib>

struct Record {
    unsigned char digits[16];
    int digits_len;
};


// Генерация случайных телефонных номеров

std::vector<Record> generate(int n) {
    std::vector<Record> v(n);

    for (int i = 0; i < n; ++i) {
        v[i].digits_len = 11;
        for (int j = 0; j < 11; ++j)
            v[i].digits[j] = rand() % 10;
    }

    return v;
}


// RADIX SORT

void counting_sort(std::vector<Record>& a, int pos) {
    int n = a.size();
    std::vector<Record> tmp(n);
    int count[10] = {0};

    for (int i = 0; i < n; ++i) {
        int d = (pos < a[i].digits_len)
                ? a[i].digits[a[i].digits_len - 1 - pos]
                : 0;
        ++count[d];
    }

    for (int i = 1; i < 10; ++i)
        count[i] += count[i - 1];

    for (int i = n - 1; i >= 0; --i) {
        int d = (pos < a[i].digits_len)
                ? a[i].digits[a[i].digits_len - 1 - pos]
                : 0;
        tmp[--count[d]] = a[i];
    }

    a = tmp;
}

void radix_sort(std::vector<Record>& a, int max_digits) {
    for (int pos = 0; pos < max_digits; ++pos)
        counting_sort(a, pos);
}


// std::sort


bool cmp(const Record& a, const Record& b) {
    if (a.digits_len != b.digits_len)
        return a.digits_len < b.digits_len;

    return std::memcmp(a.digits, b.digits, a.digits_len) < 0;
}


// MAIN BENCHMARK


int main() {

    srand(time(nullptr));

    int sizes[] = {10000, 50000, 100000, 200000, 500000};

    std::cout << "N\tRadix(ms)\tstd::sort(ms)\n";

    for (int s : sizes) {

        auto data = generate(s);
        auto data_copy = data;  // копия для второй сортировки

        // ---- Radix ----
        auto start1 = std::chrono::high_resolution_clock::now();
        radix_sort(data, 11);
        auto end1 = std::chrono::high_resolution_clock::now();

        auto radix_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();

        // ---- std::sort ----
        auto start2 = std::chrono::high_resolution_clock::now();
        std::sort(data_copy.begin(), data_copy.end(), cmp);
        auto end2 = std::chrono::high_resolution_clock::now();

        auto std_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();

        std::cout << s << "\t"
                  << radix_time << "\t\t"
                  << std_time << "\n";
    }

    return 0;
}