#include <bits/stdc++.h> //meow

using namespace std; // чтобы не писать std:: перед vector, cout и т.д.

struct Token { // структура для хранения одного элемента текста

    uint32_t value; // само число (элемент текста)

    int line; // номер строки, в которой встретилось это число

    int word; // номер слова в строке (позиция в строке)

};

vector<int> zFunction(const vector<uint32_t>& s) {

    int n = (int)s.size(); // длина массива (паттерна или строки)

    vector<int> z(n, 0); // создаём массив Z длины n, изначально все значения 0

    int l = 0, r = 0; // границы текущего "окна" [l, r], где уже знаем совпадения

    for (int i = 1; i < n; ++i) { // начинаем с 1, потому что z[0] обрабатывается отдельно

        if (i <= r) { 
            // если текущая позиция i находится внутри уже известного окна [l, r]

            z[i] = min(r - i + 1, z[i - l]); 
            // используем уже посчитанную информацию:
            // копируем значение из "зеркальной" позиции i-l
            // но не выходим за границу окна
        }

        while (i + z[i] < n && s[z[i]] == s[i + z[i]]) {
            // пытаемся расширить совпадение:
            // сравниваем префикс строки s[0...] с подстрокой, начинающейся в i

            ++z[i]; // увеличиваем длину совпадения
        }

        if (i + z[i] - 1 > r) {
            // если мы вышли за текущий правый край окна

            l = i; // сдвигаем левую границу окна
            r = i + z[i] - 1; // обновляем правую границу окна
        }

    }

    if (n > 0) {
        z[0] = n; // по определению: вся строка совпадает сама с собой
    }

    return z; // возвращаем массив Z
}

vector<int> buildN(const vector<uint32_t>& pattern) {

    int m = (int)pattern.size(); 
    // длина паттерна

    vector<uint32_t> revPattern = pattern; 
    // копируем паттерн, чтобы не менять оригинал

    reverse(revPattern.begin(), revPattern.end()); 
    // разворачиваем паттерн (нужно для вычисления N через Z)

    vector<int> zRev = zFunction(revPattern); 
    // считаем Z-функцию для перевёрнутого паттерна

    vector<int> nArray(m, 0); 
    // создаём массив N длины m

    for (int i = 0; i < m; ++i) {

        nArray[i] = zRev[m - 1 - i]; 
        // переносим значения из Z:
        // индекс "зеркалим", потому что работали с перевёрнутой строкой

        if (nArray[i] > i + 1) {
            // защита от выхода за границы подстроки P[0..i]

            nArray[i] = i + 1; 
            // длина совпадения не может быть больше длины самой подстроки
        }

    }

    return nArray; 
    // возвращаем массив N
}
vector<int> buildGoodSuffixShift(const vector<uint32_t>& pattern) {

    int m = (int)pattern.size(); 
    // длина паттерна

    vector<int> shift(m + 1, 0); 
    // массив сдвигов (good suffix), индекс i означает:
    // на сколько сдвинуться при несовпадении в позиции i-1

    vector<int> borderPos(m + 1, 0); 
    // массив границ (border) — хранит информацию о "бордерах" (префикс = суффикс)

    int i = m; 
    // начинаем с конца паттерна

    int j = m + 1; 
    // j указывает на позицию, куда мы "перепрыгиваем" при поиске бордера

    borderPos[i] = j; 
    // для пустого суффикса (в конце) бордер находится за границей строки

    while (i > 0) { 
        // идём справа налево по паттерну

        while (j <= m && pattern[i - 1] != pattern[j - 1]) {
            // пока символы не совпадают — ищем следующий бордер

            if (shift[j] == 0) {
                // если для этой позиции ещё не записан сдвиг

                shift[j] = j - i; 
                // записываем сдвиг:
                // насколько нужно сдвинуть, чтобы выровнять совпавший суффикс
            }

            j = borderPos[j]; 
            // прыгаем к следующему возможному бордеру
        }

        --i; 
        // двигаемся левее

        --j; 
        // синхронно двигаем j

        borderPos[i] = j; 
        // сохраняем найденный бордер для позиции i
    }

    j = borderPos[0]; 
    // берём самый длинный бордер для всей строки

    for (i = 0; i <= m; ++i) { 
        // заполняем оставшиеся значения shift

        if (shift[i] == 0) {
            // если сдвиг ещё не определён

            shift[i] = j; 
            // используем наибольший бордер
        }

        if (i == j) {
            // если дошли до текущего бордера

            j = borderPos[j]; 
            // переходим к следующему бордеру
        }
    }

    return shift; 
    // возвращаем таблицу good suffix сдвигов
}
vector<pair<int, int>> apostolicoGiancarlo(

    const vector<uint32_t>& pattern, // паттерн (что ищем)

    const vector<Token>& text // текст (массив токенов с координатами)

) {

    vector<pair<int, int>> answer; // сюда будем записывать найденные вхождения (line, word)

    int m = (int)pattern.size(); // длина паттерна

    int n = (int)text.size(); // длина текста

    if (m == 0 || n < m) { // если паттерн пуст или длиннее текста
        return answer; // совпадений быть не может
    }

    if (m == 1) { // частный случай: паттерн из одного элемента
        for (int i = 0; i < n; ++i) { // проходим по всему тексту
            if (text[i].value == pattern[0]) { // если совпало
                answer.push_back({text[i].line, text[i].word}); // добавляем координаты
            }
        }
        return answer; // возвращаем результат
    }

    vector<int> nArray = buildN(pattern); 
    // массив N — информация о суффиксах внутри паттерна

    vector<int> goodShift = buildGoodSuffixShift(pattern); 
    // таблица good suffix (ППС)

    unordered_map<uint32_t, int> lastOccurrence; 
    // таблица плохого символа (ПХС): последнее вхождение символа в паттерне

    lastOccurrence.reserve((size_t)m * 2); 
    // резервируем память для ускорения

    for (int i = 0; i < m; ++i) {
        lastOccurrence[pattern[i]] = i; 
        // сохраняем последнюю позицию каждого символа
    }

    vector<int> memory(n, 0); 
    // массив M: сколько символов справа уже совпало (ключевая идея AG)

    int s = 0; // текущая позиция начала окна (куда "прикладываем" паттерн)

    while (s <= n - m) { // пока паттерн помещается в текст

        int h = s + m - 1; // позиция правого конца паттерна в тексте
        int i = m - 1; // индекс в паттерне (начинаем с конца)
        int j = h; // индекс в тексте

        bool mismatch = false; // флаг несовпадения

        while (i >= 0) { // сравниваем справа налево

            int known = memory[j]; // сколько уже известно совпадающих символов

            if (known == 0) { 
                // случай 1: ничего не знаем — сравниваем напрямую

                if (pattern[i] == text[j].value) { // если совпало
                    --i; // двигаемся влево по паттерну
                    --j; // двигаемся влево по тексту
                } else {
                    mismatch = true; // нашли несовпадение
                    break;
                }

            } else {

                int ni = nArray[i]; // значение N[i]

                if (known < ni) {
                    // случай 2: текст совпал меньше, чем позволяет паттерн

                    i -= known; // перепрыгиваем known символов
                    j -= known;

                    mismatch = true; // гарантированно будет mismatch
                    break;

                } else if (known == ni) {
                    // случай 3: совпадение ровно совпадает

                    i -= known; // просто перепрыгиваем
                    j -= known;

                } else {
                    // случай 4: текст совпал "слишком хорошо"

                    if (ni == i + 1) {
                        // совпал весь префикс

                        i -= ni;
                        j -= ni;

                    } else {

                        i -= ni;
                        j -= ni;

                        mismatch = true; // гарантированное несовпадение
                        break;
                    }

                }

            }

        }

        int matchedFromRight = m - 1 - i; 
        // сколько символов совпало справа

        memory[h] = matchedFromRight; 
        // записываем это в M (ключевая оптимизация)

        if (!mismatch && i < 0) {
            // если дошли до начала паттерна → полное совпадение

            answer.push_back({text[s].line, text[s].word}); 
            // записываем координаты начала

            int shift = goodShift[0]; // сдвиг по good suffix

            if (shift <= 0) {
                shift = 1; // защита от нулевого сдвига
            }

            s += shift; // сдвигаем окно

        } else {

            int badCharShift = 1; // сдвиг по плохому символу

            auto it = lastOccurrence.find(text[j].value); 
            // ищем символ в таблице

            if (it == lastOccurrence.end()) {
                badCharShift = i + 1; // символа нет в паттерне
            } else {
                badCharShift = i - it->second; // стандартное правило ПХС

                if (badCharShift <= 0) {
                    badCharShift = 1; // минимум 1
                }
            }

            int goodSuffixShift = goodShift[i + 1]; 
            // сдвиг по хорошему суффиксу

            if (goodSuffixShift <= 0) {
                goodSuffixShift = 1;
            }

            s += max(badCharShift, goodSuffixShift); 
            // берём максимум двух эвристик
        }

    }

    return answer; // возвращаем все найденные позиции
}

int main() {

    ios::sync_with_stdio(false); // отключаем синхронизацию с C-вводом, чтобы ускорить ввод/вывод

    cin.tie(nullptr); // отвязываем cin от cout, чтобы не было лишних flush

    string line; // строка для чтения входных данных

    if (!getline(cin, line)) { // читаем первую строку (паттерн)
        return 0; // если вход пустой — завершаем программу
    }

    vector<uint32_t> pattern; // массив для хранения паттерна (последовательности чисел)

    {
        stringstream ss(line); // создаём поток для парсинга строки
        unsigned long long x; // читаем числа как 64-битные (на случай больших значений)

        while (ss >> x) { // читаем числа из строки
            pattern.push_back(static_cast<uint32_t>(x)); // приводим к uint32 и сохраняем
        }
    }

    vector<Token> text; // массив для хранения всех чисел текста + их позиции

    int lineNumber = 0; // номер текущей строки текста

    while (getline(cin, line)) { // читаем текст построчно
        ++lineNumber; // увеличиваем номер строки

        stringstream ss(line); // поток для разбора строки
        unsigned long long x; // переменная для чтения числа
        int wordNumber = 0; // номер слова в строке

        while (ss >> x) { // читаем числа из строки
            ++wordNumber; // увеличиваем номер слова

            Token token; // создаём токен
            token.value = static_cast<uint32_t>(x); // сохраняем значение числа
            token.line = lineNumber; // сохраняем номер строки
            token.word = wordNumber; // сохраняем номер слова

            text.push_back(token); // добавляем в массив текста
        }
    }

    vector<pair<int, int>> answer = apostolicoGiancarlo(pattern, text); 
    // запускаем алгоритм поиска

    for (const auto& occurrence : answer) { // перебираем все найденные вхождения
        cout << occurrence.first << ", " << occurrence.second << '\n'; 
        // выводим координаты: строка, слово
    }

    return 0; // завершение программы
}
