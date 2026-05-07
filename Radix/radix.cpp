#include <cstdio>
#include <cstring>

// Одна запись = исходная строка + массив цифр номера для сортировки
struct Record {
    // Один выделенный блок памяти: сначала line (C-строка), затем массив digits.
    char* line;                 // исходная строка (без \n/\r)
    unsigned char* digits;      // цифры телефонного номера (0..9)
    int digits_len;             // количество цифр
};

static inline bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

// Берём цифру "с конца" (LSD-логика).
// pos_from_right = 0 -> самая правая цифра.
// Если номер короче (цифры закончились), считаем что слева стоят нули (как при дополнении слева нулями).
static inline int get_digit_from_right(const Record& r, int pos_from_right) {
    if (pos_from_right >= r.digits_len) return 0;
    return (int)r.digits[r.digits_len - 1 - pos_from_right];
}

// Стабильный Counting Sort по одной позиции (pos_from_right) в ключе
static void counting_sort_by_pos(Record* a, Record* tmp, int n, int pos_from_right) {
    int count[10];
    for (int i = 0; i < 10; ++i) count[i] = 0;

    // 1) Подсчёт
    for (int i = 0; i < n; ++i) {
        int d = get_digit_from_right(a[i], pos_from_right);
        ++count[d];
    }

    // 2) Префиксные суммы: count[d] = сколько элементов <= d
    for (int d = 1; d < 10; ++d) {
        count[d] += count[d - 1];
    }

    // 3) Заполняем справа налево -> стабильность
    for (int i = n - 1; i >= 0; --i) {
        int d = get_digit_from_right(a[i], pos_from_right);
        int pos = --count[d];       // позиция для этого элемента
        tmp[pos] = a[i];
    }

    // 4) Копируем обратно
    for (int i = 0; i < n; ++i) {
        a[i] = tmp[i];
    }
}

// Radix Sort (LSD): от младшего разряда к старшему
static void radix_sort(Record* a, int n, int max_digits) {
    if (n <= 1) return;

    Record* tmp = new Record[n];

    // pos = 0..max_digits-1 (0 = последняя цифра)
    for (int pos = 0; pos < max_digits; ++pos) {
        counting_sort_by_pos(a, tmp, n, pos);
    }

    delete[] tmp;
}

// Увеличение вместимости самодельного динамического массива
static void reserve_more(Record*& a, int& cap) {
    int new_cap = (cap == 0) ? 1024 : (cap * 2);
    Record* b = new Record[new_cap];
    // ВАЖНО: вызываем reserve_more только когда n == cap,
    // значит все элементы 0..cap-1 заполнены и их можно копировать.
    for (int i = 0; i < cap; ++i) {
        b[i] = a[i];
    }
    delete[] a;
    a = b;
    cap = new_cap;
}

int main() {
    const int BUF_SIZE = 4096;
    char buf[BUF_SIZE];

    Record* arr = 0;
    int n = 0;
    int cap = 0;
    int max_digits = 0;

    while (std::fgets(buf, BUF_SIZE, stdin)) {
        // Убираем \n и возможный \r в конце
        int len = (int)std::strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
            buf[--len] = '\0';
        }
        if (len == 0) continue; // пустую строку не учитываем

        // Ищем табуляцию: ключ \t значение
        int tab = -1;
        for (int i = 0; i < len; ++i) {
            if (buf[i] == '\t') { tab = i; break; }
        }
        if (tab == -1) tab = len; // если таба нет — считаем, что вся строка это ключ

        // Считаем число цифр в ключе
        int digits_cnt = 0;
        for (int i = 0; i < tab; ++i) {
            if (is_digit(buf[i])) ++digits_cnt;
        }

        // Выделяем один блок памяти:
        // [строка (len+1 байт)] + [digits_cnt байт под цифры]
        char* block = new char[len + 1 + digits_cnt]; //выделяем блок памяти 
        std::memcpy(block, buf, len + 1);//копируем каждую строку в отдельный блок памяти иначе строки бы затирались 
        //те block независимая копия строки
        unsigned char* digits = (unsigned char*)(block + len + 1);

        // Заполняем digits (0..9) из ключа
        int k = 0;
        for (int i = 0; i < tab; ++i) {
            if (is_digit(block[i])) {
                digits[k++] = (unsigned char)(block[i] - '0');
            }
        }

        // Добавляем запись в массив
        if (n == cap) {
            reserve_more(arr, cap);
        }

        arr[n].line = block;
        arr[n].digits = digits;
        arr[n].digits_len = digits_cnt;
        ++n;

        if (digits_cnt > max_digits) max_digits = digits_cnt;
    }

    // Сортировка
    radix_sort(arr, n, max_digits);

    // Вывод
    for (int i = 0; i < n; ++i) {
        std::fputs(arr[i].line, stdout);
        std::fputc('\n', stdout);
    }

    // Освобождаем память
    for (int i = 0; i < n; ++i) {
        delete[] arr[i].line; // digits лежат в этом же блоке
    }
    delete[] arr;

    return 0;
}