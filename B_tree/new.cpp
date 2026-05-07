#include <iostream> // для cin, cout
#include <fstream> // для ifstream, ofstream
#include <sstream> // для istringstream
#include <string> // для string
#include <cstdint> // для uint64_t, uint32_t

using namespace std; // чтобы не писать std::

#ifdef DMALLOC
#include <dmalloc.h> // подключение отладчика памяти, если он используется
#endif

const int T = 32; // минимальная степень b-дерева

struct TPair {
    string Key; // ключ словаря
    uint64_t Value; // значение, соответствующее ключу
};

struct TBTreeNode {
    bool Leaf; // признак листа
    int N; // текущее количество ключей в узле
    TPair Keys[2 * T - 1]; // массив ключей, максимум 2*t - 1
    TBTreeNode* Children[2 * T]; // массив детей, максимум 2*t

    TBTreeNode(bool leaf = true) : Leaf(leaf), N(0) { // конструктор узла
        for (int i = 0; i < 2 * T; ++i) { // проходим по всем детям
            Children[i] = nullptr; // сначала детей нет
        }
    }
};

class TBTree {
private:
    TBTreeNode* Root; // указатель на корень дерева

private:
    // создает новый узел
    TBTreeNode* CreateNode(bool leaf) {
        return new TBTreeNode(leaf); // выделяем память под новый узел
    }

    // рекурсивно удаляет поддерево
    void Destroy(TBTreeNode* node) {
        if (node == nullptr) { // если узла нет
            return; // удалять нечего
        }

        if (!node->Leaf) { // если узел не лист
            for (int i = 0; i <= node->N; ++i) { // у узла с n ключами может быть n + 1 детей
                Destroy(node->Children[i]); // удаляем каждого ребенка
            }
        }

        delete node; // удаляем сам узел
    }

    // приводит слово к нижнему регистру
    static string Normalize(const string& s) {
        string result = s; // делаем копию строки
        for (char& c : result) { // идем по всем символам
            if (c >= 'A' && c <= 'Z') { // если буква большая
                c = static_cast<char>(c - 'A' + 'a'); // переводим в маленькую
            }
        }
        return result; // возвращаем нормализованную строку
    }

    // ищет первый ключ, который не меньше key
    int FindKey(TBTreeNode* node, const string& key) const {
        int idx = 0; // начинаем с первого ключа
        while (idx < node->N && node->Keys[idx].Key < key) { // пока текущий ключ меньше нужного
            ++idx; // идем дальше
        }
        return idx; // возвращаем позицию первого ключа >= key
    }

    // рекурсивный поиск по дереву
    TBTreeNode* Search(TBTreeNode* node, const string& key, int& pos) const {
        if (node == nullptr) { // если дерево пустое или дошли до пустого узла
            return nullptr; // ничего не нашли
        }

        int i = 0; // индекс внутри текущего узла
        while (i < node->N && key > node->Keys[i].Key) { // ищем место, где key <= текущему ключу
            ++i; // двигаемся вправо по ключам узла
        }

        if (i < node->N && node->Keys[i].Key == key) { // если нашли точное совпадение
            pos = i; // запоминаем позицию ключа в узле
            return node; // возвращаем указатель на узел
        }

        if (node->Leaf) { // если это лист и ключ не найден
            return nullptr; // значит такого слова нет
        }

        return Search(node->Children[i], key, pos); // рекурсивно идем в подходящего ребенка
    }

    // делит полного ребенка
    void SplitChild(TBTreeNode* parent, int index) {
        TBTreeNode* fullChild = parent->Children[index]; // полный ребенок, которого надо делить
        TBTreeNode* newChild = CreateNode(fullChild->Leaf); // создаем нового узла того же типа: лист или нет

        newChild->N = T - 1; // у нового правого узла будет t - 1 ключей

        for (int j = 0; j < T - 1; ++j) { // копируем правую половину ключей
            newChild->Keys[j] = fullChild->Keys[j + T]; // берем ключи после среднего
        }

        if (!fullChild->Leaf) { // если делим не лист
            for (int j = 0; j < T; ++j) { // переносим правую половину детей
                newChild->Children[j] = fullChild->Children[j + T]; // забираем правых детей
                fullChild->Children[j + T] = nullptr; // очищаем старые ссылки
            }
        }

        fullChild->N = T - 1; // в левом узле тоже остается t - 1 ключей

        for (int j = parent->N; j >= index + 1; --j) { // сдвигаем детей родителя вправо
            parent->Children[j + 1] = parent->Children[j]; // освобождаем место под нового ребенка
        }
        parent->Children[index + 1] = newChild; // вставляем нового правого ребенка

        for (int j = parent->N - 1; j >= index; --j) { // сдвигаем ключи родителя вправо
            parent->Keys[j + 1] = parent->Keys[j]; // освобождаем место под средний ключ
        }

        parent->Keys[index] = fullChild->Keys[T - 1]; // поднимаем средний ключ в родителя
        parent->N += 1; // у родителя стало на один ключ больше
    }

    // вставка в неполный узел
    void InsertNonFull(TBTreeNode* node, const TPair& entry) {
        int i = node->N - 1; // начинаем с последнего существующего ключа

        if (node->Leaf) { // если узел листовой
            while (i >= 0 && entry.Key < node->Keys[i].Key) { // пока новый ключ меньше текущего
                node->Keys[i + 1] = node->Keys[i]; // сдвигаем ключ вправо
                --i; // идем влево
            }

            node->Keys[i + 1] = entry; // вставляем новый ключ на найденное место
            node->N += 1; // увеличиваем число ключей
        } else { // если узел внутренний
            while (i >= 0 && entry.Key < node->Keys[i].Key) { // ищем, в какого ребенка идти
                --i; // двигаемся влево
            }
            ++i; // переходим к индексу нужного ребенка

            if (node->Children[i]->N == 2 * T - 1) { // если нужный ребенок полный
                SplitChild(node, i); // сначала делим его

                if (entry.Key > node->Keys[i].Key) { // после деления проверяем, в левый или правый из двух новых узлов идти
                    ++i; // если ключ больше поднятого среднего, идем вправо
                }
            }

            InsertNonFull(node->Children[i], entry); // рекурсивно вставляем в неполный ребенок
        }
    }

    // удаление из листа
    void RemoveFromLeaf(TBTreeNode* node, int index) {
        for (int i = index + 1; i < node->N; ++i) { // все ключи правее удаляемого
            node->Keys[i - 1] = node->Keys[i]; // сдвигаем влево
        }
        node->N -= 1; // уменьшаем количество ключей
    }

    // предшественник
    TPair GetPredecessor(TBTreeNode* node, int index) const {
        TBTreeNode* cur = node->Children[index]; // идем в левое поддерево
        while (!cur->Leaf) { // пока не дошли до листа
            cur = cur->Children[cur->N]; // идем в самого правого ребенка
        }
        return cur->Keys[cur->N - 1]; // берем самый правый ключ
    }

    // последователь
    TPair GetSuccessor(TBTreeNode* node, int index) const {
        TBTreeNode* cur = node->Children[index + 1]; // идем в правое поддерево
        while (!cur->Leaf) { // пока не дошли до листа
            cur = cur->Children[0]; // идем в самого левого ребенка
        }
        return cur->Keys[0]; // берем самый левый ключ
    }

    // слияние двух детей
    void Merge(TBTreeNode* node, int index) {
        TBTreeNode* child = node->Children[index]; // левый ребенок
        TBTreeNode* sibling = node->Children[index + 1]; // правый сосед

        child->Keys[T - 1] = node->Keys[index]; // опускаем ключ из родителя между двумя детьми

        for (int i = 0; i < sibling->N; ++i) { // копируем все ключи правого соседа
            child->Keys[i + T] = sibling->Keys[i]; // дописываем их в конец левого узла
        }

        if (!child->Leaf) { // если это внутренние узлы
            for (int i = 0; i <= sibling->N; ++i) { // переносим всех детей правого соседа
                child->Children[i + T] = sibling->Children[i]; // дописываем детей
                sibling->Children[i] = nullptr; // очищаем старые ссылки
            }
        }

        for (int i = index + 1; i < node->N; ++i) { // сдвигаем ключи родителя влево
            node->Keys[i - 1] = node->Keys[i]; // закрываем дыру после удаления ключа
        }

        for (int i = index + 2; i <= node->N; ++i) { // сдвигаем детей родителя влево
            node->Children[i - 1] = node->Children[i]; // закрываем дыру после удаления ребенка
        }

        child->N += sibling->N + 1; // в левом узле теперь старые ключи + ключ родителя + ключи соседа
        node->N -= 1; // у родителя стало на один ключ меньше

        delete sibling; // удаляем правого соседа
    }

    // берем ключ у левого соседа
    void BorrowFromPrev(TBTreeNode* node, int index) {
        TBTreeNode* child = node->Children[index]; // ребенок, которому не хватает ключей
        TBTreeNode* sibling = node->Children[index - 1]; // левый сосед

        for (int i = child->N - 1; i >= 0; --i) { // сдвигаем ключи ребенка вправо
            child->Keys[i + 1] = child->Keys[i]; // освобождаем место в начале
        }

        if (!child->Leaf) { // если ребенок не лист
            for (int i = child->N; i >= 0; --i) { // сдвигаем его детей вправо
                child->Children[i + 1] = child->Children[i]; // освобождаем место для нового левого ребенка
            }
        }

        child->Keys[0] = node->Keys[index - 1]; // ключ из родителя опускаем в начало ребенка

        if (!child->Leaf) { // если есть дети
            child->Children[0] = sibling->Children[sibling->N]; // крайний правый ребенок соседа переходит ребенку
            sibling->Children[sibling->N] = nullptr; // очищаем старую ссылку
        }

        node->Keys[index - 1] = sibling->Keys[sibling->N - 1]; // последний ключ левого соседа поднимаем в родителя

        child->N += 1; // у ребенка стало больше ключей
        sibling->N -= 1; // у соседа стало меньше ключей
    }

    // берем ключ у правого соседа
    void BorrowFromNext(TBTreeNode* node, int index) {
        TBTreeNode* child = node->Children[index]; // ребенок, которому не хватает ключей
        TBTreeNode* sibling = node->Children[index + 1]; // правый сосед

        child->Keys[child->N] = node->Keys[index]; // ключ из родителя добавляем в конец ребенка

        if (!child->Leaf) { // если ребенок не лист
            child->Children[child->N + 1] = sibling->Children[0]; // первый ребенок соседа переходит ребенку
        }

        node->Keys[index] = sibling->Keys[0]; // первый ключ правого соседа поднимаем в родителя

        for (int i = 1; i < sibling->N; ++i) { // сдвигаем ключи соседа влево
            sibling->Keys[i - 1] = sibling->Keys[i]; // закрываем дырку после взятия первого ключа
        }

        if (!sibling->Leaf) { // если сосед не лист
            for (int i = 1; i <= sibling->N; ++i) { // сдвигаем его детей влево
                sibling->Children[i - 1] = sibling->Children[i]; // закрываем дырку после передачи первого ребенка
            }
            sibling->Children[sibling->N] = nullptr; // очищаем лишнюю ссылку
        }

        child->N += 1; // у ребенка стало больше ключей
        sibling->N -= 1; // у соседа стало меньше ключей
    }

    // подготавливает ребенка перед спуском
    void Fill(TBTreeNode* node, int index) {
        if (index != 0 && node->Children[index - 1]->N >= T) { // если слева есть сосед и у него можно занять ключ
            BorrowFromPrev(node, index); // берем у левого соседа
        } else if (index != node->N && node->Children[index + 1]->N >= T) { // иначе если справа есть сосед и у него можно занять
            BorrowFromNext(node, index); // берем у правого соседа
        } else { // если занять нельзя
            if (index != node->N) { // если есть правый сосед
                Merge(node, index); // сливаем с правым
            } else { // иначе
                Merge(node, index - 1); // сливаем с левым
            }
        }
    }

    // удаление из внутреннего узла
    void RemoveFromNonLeaf(TBTreeNode* node, int index) {
        string key = node->Keys[index].Key; // сохраняем удаляемый ключ

        if (node->Children[index]->N >= T) { // если в левом ребенке достаточно ключей
            TPair pred = GetPredecessor(node, index); // берем предшественника
            node->Keys[index] = pred; // заменяем удаляемый ключ предшественником
            Remove(node->Children[index], pred.Key); // удаляем предшественника из левого поддерева
        } else if (node->Children[index + 1]->N >= T) { // если в правом ребенке достаточно ключей
            TPair succ = GetSuccessor(node, index); // берем последователя
            node->Keys[index] = succ; // заменяем удаляемый ключ последователем
            Remove(node->Children[index + 1], succ.Key); // удаляем последователя из правого поддерева
        } else { // если оба ребенка минимальные
            Merge(node, index); // сливаем их вместе с ключом из родителя
            Remove(node->Children[index], key); // удаляем ключ уже из объединенного узла
        }
    }

    // основная рекурсивная функция удаления
    void Remove(TBTreeNode* node, const string& key) {
        int index = FindKey(node, key); // находим первую позицию, где ключ >= искомого

        if (index < node->N && node->Keys[index].Key == key) { // если ключ найден в текущем узле
            if (node->Leaf) { // если узел листовой
                RemoveFromLeaf(node, index); // просто удаляем из листа
            } else { // если узел внутренний
                RemoveFromNonLeaf(node, index); // применяем сложную логику удаления из внутреннего узла
            }
        } else { // если ключа в этом узле нет
            if (node->Leaf) { // если это лист
                return; // значит ключа в дереве нет
            }

            bool flag = (index == node->N); // запоминаем, хотели ли идти в самого правого ребенка

            if (node->Children[index]->N < T) { // если у нужного ребенка меньше t ключей
                Fill(node, index); // сначала подготавливаем его
            }

            if (flag && index > node->N) { // если после слияния правая граница сместилась
                Remove(node->Children[index - 1], key); // идем в левого объединенного ребенка
            } else {
                Remove(node->Children[index], key); // иначе идем в нужного ребенка
            }
        }
    }

    // считает количество записей без доп памяти
    uint64_t CountEntries(TBTreeNode* node) const {
        if (node == nullptr) { // если узла нет
            return 0; // записей нет
        }

        uint64_t result = node->N; // сначала считаем ключи в текущем узле
        if (!node->Leaf) { // если узел не лист
            for (int i = 0; i <= node->N; ++i) { // обходим всех детей
                result += CountEntries(node->Children[i]); // добавляем количество записей из поддеревьев
            }
        }
        return result; // возвращаем общее количество
    }

    // пишет строку в бинарный файл
    bool WriteString(ofstream& out, const string& s) const {
        uint32_t len = static_cast<uint32_t>(s.size()); // длина строки
        out.write(reinterpret_cast<const char*>(&len), sizeof(len)); // сначала пишем длину
        if (!out) { // если запись не удалась
            return false; // сообщаем об ошибке
        }

        if (len > 0) { // если строка не пустая
            out.write(s.data(), len); // пишем ее символы
            if (!out) { // если запись не удалась
                return false; // сообщаем об ошибке
            }
        }

        return true; // строка успешно записана
    }

    // читает строку из бинарного файла
    bool ReadString(ifstream& in, string& s) const {
        uint32_t len = 0; // сюда читаем длину строки
        in.read(reinterpret_cast<char*>(&len), sizeof(len)); // читаем длину
        if (!in) { // если чтение не удалось
            return false; // ошибка
        }

        if (len > 256) { // защита от некорректной длины
            return false; // считаем формат плохим
        }

        s.assign(len, '\0'); // создаем строку нужной длины
        if (len > 0) { // если длина не ноль
            in.read(&s[0], len); // читаем символы строки
            if (!in) { // если чтение не удалось
                return false; // ошибка
            }
        }

        return true; // строка успешно прочитана
    }

    // сразу пишет все записи в файл, не копируя их в вектор
    bool WriteEntries(ofstream& out, TBTreeNode* node) const {
        if (node == nullptr) { // если узла нет
            return true; // писать нечего, это не ошибка
        }

        for (int i = 0; i < node->N; ++i) { // идем по всем ключам текущего узла
            if (!node->Leaf) { // если узел не лист
                if (!WriteEntries(out, node->Children[i])) { // сначала пишем левое поддерево
                    return false; // если ошибка записи, выходим
                }
            }

            if (!WriteString(out, node->Keys[i].Key)) { // пишем ключ
                return false; // если ошибка, выходим
            }

            out.write(reinterpret_cast<const char*>(&node->Keys[i].Value), sizeof(node->Keys[i].Value)); // пишем значение
            if (!out) { // если запись не удалась
                return false; // ошибка
            }
        }

        if (!node->Leaf) { // после всех ключей
            if (!WriteEntries(out, node->Children[node->N])) { // пишем самое правое поддерево
                return false; // если ошибка, выходим
            }
        }

        return true; // все записи этого поддерева успешно записаны
    }

public:
    TBTree() : Root(nullptr) { // конструктор, создает пустое дерево
    }

    ~TBTree() { // деструктор
        Clear(); // при уничтожении объекта освобождаем память
    }

    // полностью очищает дерево
    void Clear() {
        Destroy(Root); // удаляем все поддерево
        Root = nullptr; // после очистки дерево пустое
    }

    // поиск слова
    bool Find(const string& key, uint64_t& value) const {
        string normKey = Normalize(key); // приводим слово к нижнему регистру
        int pos = -1; // сюда Search запишет позицию ключа в узле
        TBTreeNode* node = Search(Root, normKey, pos); // ищем нужный ключ

        if (node == nullptr) { // если не нашли
            return false; // возвращаем false
        }

        value = node->Keys[pos].Value; // если нашли, достаем значение
        return true; // сообщаем об успехе
    }

    // вставка нового слова
    bool Insert(const string& key, uint64_t value) {
        string normKey = Normalize(key); // нормализуем ключ

        int pos = -1; // переменная для позиции, если ключ найдется
        if (Search(Root, normKey, pos) != nullptr) { // если такой ключ уже есть
            return false; // вставка невозможна
        }

        TPair entry; // создаем новую пару ключ-значение
        entry.Key = normKey; // записываем ключ
        entry.Value = value; // записываем значение

        if (Root == nullptr) { // если дерево пустое
            Root = CreateNode(true); // создаем корень-лист
            Root->Keys[0] = entry; // кладем первый ключ
            Root->N = 1; // теперь в корне один ключ
            return true; // вставка завершена
        }

        if (Root->N == 2 * T - 1) { // если корень полный
            TBTreeNode* newRoot = CreateNode(false); // создаем новый корень, он не лист
            newRoot->Children[0] = Root; // старый корень становится его первым ребенком

            SplitChild(newRoot, 0); // делим старый корень

            int i = 0; // определяем, в какого ребенка нового корня вставлять
            if (entry.Key > newRoot->Keys[0].Key) { // если ключ больше среднего поднятого ключа
                i = 1; // идем в правого ребенка
            }

            InsertNonFull(newRoot->Children[i], entry); // вставляем в неполный ребенок
            Root = newRoot; // новый корень становится текущим корнем дерева
        } else { // если корень не полный
            InsertNonFull(Root, entry); // просто вставляем в него или в его поддерево
        }

        return true; // вставка успешна
    }

    // удаление слова
    bool Erase(const string& key) {
        if (Root == nullptr) { // если дерево пустое
            return false; // удалять нечего
        }

        string normKey = Normalize(key); // нормализуем ключ

        int pos = -1; // позиция ключа, если найдется
        if (Search(Root, normKey, pos) == nullptr) { // если такого слова нет
            return false; // удаление невозможно
        }

        Remove(Root, normKey); // удаляем ключ из дерева

        if (Root->N == 0) { // если после удаления корень стал пустым
            TBTreeNode* oldRoot = Root; // запоминаем старый корень

            if (Root->Leaf) { // если корень был листом
                Root = nullptr; // дерево становится пустым
            } else { // если корень не лист
                Root = Root->Children[0]; // его единственный ребенок становится новым корнем
                oldRoot->Children[0] = nullptr; // обнуляем ссылку, чтобы не удалить новое дерево вместе со старым корнем
            }

            delete oldRoot; // удаляем старый пустой корень
        }

        return true; // удаление успешно
    }

    // сохранение словаря в бинарный файл
    bool Save(const string& path, string& errorMessage) const {
        ofstream out(path, ios::binary | ios::trunc); // открываем файл на запись в бинарном режиме, старое содержимое очищаем
        if (!out.is_open()) { // если не удалось открыть файл
            errorMessage = "can't open file"; // записываем текст ошибки
            return false; // сохранять не смогли
        }

        const char magic[4] = {'B', 'T', 'R', 'E'}; // сигнатура нашего формата
        out.write(magic, 4); // записываем сигнатуру
        if (!out) { // если запись не удалась
            errorMessage = "can't write file"; // ошибка записи
            return false; // завершение с ошибкой
        }

        uint32_t version = 1; // версия формата файла
        out.write(reinterpret_cast<const char*>(&version), sizeof(version)); // записываем версию
        if (!out) { // если запись не удалась
            errorMessage = "can't write file"; // ошибка записи
            return false; // завершение с ошибкой
        }

        uint64_t count = CountEntries(Root); // считаем число записей в словаре
        out.write(reinterpret_cast<const char*>(&count), sizeof(count)); // записываем это число
        if (!out) { // если запись не удалась
            errorMessage = "can't write file"; // ошибка записи
            return false; // завершение с ошибкой
        }

        if (!WriteEntries(out, Root)) { // записываем все пары ключ-значение
            errorMessage = "can't write file"; // ошибка записи
            return false; // завершение с ошибкой
        }

        return true; // сохранение прошло успешно
    }

    // загрузка словаря из бинарного файла
    bool Load(const string& path, string& errorMessage) {
        ifstream in(path, ios::binary); // открываем файл на чтение в бинарном режиме

        if (!in.is_open()) { // если файл не открылся
            Clear(); // считаем, что загружаем пустой словарь
            return true; // это не ошибка
        }

        in.seekg(0, ios::end); // переходим в конец файла
        streampos fileSize = in.tellg(); // узнаем размер файла
        in.seekg(0, ios::beg); // возвращаемся в начало

        if (fileSize == 0) { // если файл пустой
            Clear(); // делаем словарь пустым
            return true; // загрузка пустого словаря успешна
        }

        char magic[4]; // массив под сигнатуру
        in.read(magic, 4); // читаем первые 4 байта
        if (!in) { // если не удалось прочитать
            errorMessage = "bad file format"; // формат плохой
            return false; // ошибка
        }

        if (!(magic[0] == 'B' && magic[1] == 'T' && magic[2] == 'R' && magic[3] == 'E')) { // проверяем сигнатуру
            errorMessage = "bad file format"; // формат не наш
            return false; // ошибка
        }

        uint32_t version = 0; // сюда читаем версию
        in.read(reinterpret_cast<char*>(&version), sizeof(version)); // читаем версию
        if (!in || version != 1) { // если чтение не удалось или версия не поддерживается
            errorMessage = "bad file format"; // плохой формат
            return false; // ошибка
        }

        uint64_t count = 0; // сюда читаем число записей
        in.read(reinterpret_cast<char*>(&count), sizeof(count)); // читаем число записей
        if (!in) { // если чтение не удалось
            errorMessage = "bad file format"; // плохой формат
            return false; // ошибка
        }

        TBTree tempTree; // временное дерево для безопасной загрузки

        for (uint64_t i = 0; i < count; ++i) { // читаем все записи
            string key; // сюда будет считан ключ
            uint64_t value = 0; // сюда будет считано значение

            if (!ReadString(in, key)) { // читаем строку
                errorMessage = "bad file format"; // если не удалось, формат плохой
                return false; // ошибка
            }

            in.read(reinterpret_cast<char*>(&value), sizeof(value)); // читаем число
            if (!in) { // если чтение не удалось
                errorMessage = "bad file format"; // плохой формат
                return false; // ошибка
            }

            for (char c : key) { // проверяем каждый символ ключа
                if (!(c >= 'a' && c <= 'z')) { // если встретился недопустимый символ
                    errorMessage = "bad file format"; // формат плохой
                    return false; // ошибка
                }
            }

            if (!tempTree.Insert(key, value)) { // вставляем запись во временное дерево
                errorMessage = "bad file format"; // например, если встретился дубликат ключа
                return false; // ошибка
            }
        }

        char extra; // переменная для проверки лишних данных в конце файла
        if (in.read(&extra, 1)) { // если после ожидаемых данных есть еще байты
            errorMessage = "bad file format"; // значит формат плохой
            return false; // ошибка
        }

        Clear(); // очищаем текущее дерево
        Root = tempTree.Root; // передаем себе корень временного дерева
        tempTree.Root = nullptr; // зануляем у tempTree, чтобы он не удалил это дерево в деструкторе

        return true; // загрузка успешна
    }
};

int main() {
    ios::sync_with_stdio(false); // ускоряем ввод-вывод
    cin.tie(nullptr); // отвязываем cin от cout

    TBTree tree; // создаем словарь на основе b-дерева
    string line; // сюда читаем очередную строку команды

    while (getline(cin, line)) { // читаем вход построчно
        if (line.empty()) { // если строка пустая
            continue; // пропускаем ее
        }

        if (line[0] == '+') { // если команда вставки
            istringstream iss(line); // создаем поток для разбора строки
            char op; // сюда читаем символ +
            string word; // сюда читаем слово
            uint64_t value; // сюда читаем число

            iss >> op >> word >> value; // разбираем строку вида + word value

            if (tree.Insert(word, value)) { // пытаемся вставить слово
                cout << "OK\n"; // если удалось
            } else {
                cout << "Exist\n"; // если слово уже было
            }
        } else if (line[0] == '-') { // если команда удаления
            istringstream iss(line); // поток для разбора строки
            char op; // сюда читаем символ -
            string word; // сюда читаем слово

            iss >> op >> word; // разбираем строку вида - word

            if (tree.Erase(word)) { // пытаемся удалить слово
                cout << "OK\n"; // если удалось
            } else {
                cout << "NoSuchWord\n"; // если такого слова нет
            }
        } else if (line[0] == '!') { // если команда save или load
            istringstream iss(line); // поток для разбора строки
            char mark; // сюда читаем символ !
            string command; // сюда читаем Save или Load
            string path; // сюда читаем путь к файлу

            iss >> mark >> command >> path; // разбираем команду

            string errorMessage; // строка для текста ошибки

            if (command == "Save") { // если команда сохранения
                if (tree.Save(path, errorMessage)) { // пытаемся сохранить словарь
                    cout << "OK\n"; // если успешно
                } else {
                    cout << "ERROR: " << errorMessage << '\n'; // если ошибка
                }
            } else if (command == "Load") { // если команда загрузки
                if (tree.Load(path, errorMessage)) { // пытаемся загрузить словарь
                    cout << "OK\n"; // если успешно
                } else {
                    cout << "ERROR: " << errorMessage << '\n'; // если ошибка
                }
            }
        } else { // иначе это просто слово для поиска
            uint64_t value = 0; // сюда запишется найденное значение
            if (tree.Find(line, value)) { // ищем слово
                cout << "OK: " << value << '\n'; // если нашли, печатаем значение
            } else {
                cout << "NoSuchWord\n"; // если не нашли
            }
        }
    }

    return 0; // завершаем программу
}