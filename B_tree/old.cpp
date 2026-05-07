#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>

using namespace std;

#ifdef DMALLOC
#include <dmalloc.h>
#endif

const int T = 32;

struct TPair {
    string Key;
    uint64_t Value;
};

struct TBTreeNode {
    bool Leaf;
    int N;
    TPair Keys[2 * T - 1];
    TBTreeNode* Children[2 * T];

    TBTreeNode(bool leaf = true) : Leaf(leaf), N(0) {
        for (int i = 0; i < 2 * T; ++i) {
            Children[i] = nullptr;
        }
    }
};

class TBTree {
private:
    TBTreeNode* Root;

private:
    // создает новый узел
    TBTreeNode* CreateNode(bool leaf) {
        return new TBTreeNode(leaf);
    }

    // рекурсивно удаляет поддерево
    void Destroy(TBTreeNode* node) {
        if (node == nullptr) {
            return;
        }

        if (!node->Leaf) {
            for (int i = 0; i <= node->N; ++i) {
                Destroy(node->Children[i]);
            }
        }

        delete node;
    }

    // приводит слово к нижнему регистру
    static string Normalize(const string& s) {
        string result = s;
        for (char& c : result) {
            if (c >= 'A' && c <= 'Z') {
                c = static_cast<char>(c - 'A' + 'a');
            }
        }
        return result;
    }

    // ищет первый ключ, который не меньше key
    int FindKey(TBTreeNode* node, const string& key) const {
        int idx = 0;
        while (idx < node->N && node->Keys[idx].Key < key) {
            ++idx;
        }
        return idx;
    }

    // рекурсивный поиск по дереву
    TBTreeNode* Search(TBTreeNode* node, const string& key, int& pos) const {
        if (node == nullptr) {
            return nullptr;
        }

        int i = 0;
        while (i < node->N && key > node->Keys[i].Key) {
            ++i;
        }

        if (i < node->N && node->Keys[i].Key == key) {
            pos = i;
            return node;
        }

        if (node->Leaf) {
            return nullptr;
        }

        return Search(node->Children[i], key, pos);
    }

    // делит полного ребенка parent->Children[index]
    void SplitChild(TBTreeNode* parent, int index) {
        TBTreeNode* fullChild = parent->Children[index];
        TBTreeNode* newChild = CreateNode(fullChild->Leaf);

        newChild->N = T - 1;

        // переносим правую половину ключей
        for (int j = 0; j < T - 1; ++j) {
            newChild->Keys[j] = fullChild->Keys[j + T];
        }

        // если это внутренний узел, переносим и детей
        if (!fullChild->Leaf) {
            for (int j = 0; j < T; ++j) {
                newChild->Children[j] = fullChild->Children[j + T];
                fullChild->Children[j + T] = nullptr;
            }
        }

        fullChild->N = T - 1;

        // сдвигаем детей родителя
        for (int j = parent->N; j >= index + 1; --j) {
            parent->Children[j + 1] = parent->Children[j];
        }
        parent->Children[index + 1] = newChild;

        // сдвигаем ключи родителя
        for (int j = parent->N - 1; j >= index; --j) {
            parent->Keys[j + 1] = parent->Keys[j];
        }

        // средний ключ поднимаем наверх
        parent->Keys[index] = fullChild->Keys[T - 1];
        parent->N += 1;
    }

    // вставка в неполный узел
    void InsertNonFull(TBTreeNode* node, const TPair& entry) {
        int i = node->N - 1;

        if (node->Leaf) {
            // в листе просто ищем место и сдвигаем вправо
            while (i >= 0 && entry.Key < node->Keys[i].Key) {
                node->Keys[i + 1] = node->Keys[i];
                --i;
            }

            node->Keys[i + 1] = entry;
            node->N += 1;
        } else {
            // находим ребенка, куда нужно спускаться
            while (i >= 0 && entry.Key < node->Keys[i].Key) {
                --i;
            }
            ++i;

            // если ребенок полный, сначала делим его
            if (node->Children[i]->N == 2 * T - 1) {
                SplitChild(node, i);

                if (entry.Key > node->Keys[i].Key) {
                    ++i;
                }
            }

            InsertNonFull(node->Children[i], entry);
        }
    }

    // удаление ключа из листа
    void RemoveFromLeaf(TBTreeNode* node, int index) {
        for (int i = index + 1; i < node->N; ++i) {
            node->Keys[i - 1] = node->Keys[i];
        }
        node->N -= 1;
    }

    // находит предшественника
    TPair GetPredecessor(TBTreeNode* node, int index) const {
        TBTreeNode* cur = node->Children[index];
        while (!cur->Leaf) {
            cur = cur->Children[cur->N];
        }
        return cur->Keys[cur->N - 1];
    }

    // находит последователя
    TPair GetSuccessor(TBTreeNode* node, int index) const {
        TBTreeNode* cur = node->Children[index + 1];
        while (!cur->Leaf) {
            cur = cur->Children[0];
        }
        return cur->Keys[0];
    }

    // сливает детей index и index + 1
    void Merge(TBTreeNode* node, int index) {
        TBTreeNode* child = node->Children[index];
        TBTreeNode* sibling = node->Children[index + 1];

        // ключ из родителя опускаем вниз
        child->Keys[T - 1] = node->Keys[index];

        // копируем ключи правого брата
        for (int i = 0; i < sibling->N; ++i) {
            child->Keys[i + T] = sibling->Keys[i];
        }

        // копируем детей правого брата
        if (!child->Leaf) {
            for (int i = 0; i <= sibling->N; ++i) {
                child->Children[i + T] = sibling->Children[i];
                sibling->Children[i] = nullptr;
            }
        }

        // сдвигаем ключи родителя
        for (int i = index + 1; i < node->N; ++i) {
            node->Keys[i - 1] = node->Keys[i];
        }

        // сдвигаем детей родителя
        for (int i = index + 2; i <= node->N; ++i) {
            node->Children[i - 1] = node->Children[i];
        }

        child->N += sibling->N + 1;
        node->N -= 1;

        delete sibling;
    }

    // берет ключ у левого соседа
    void BorrowFromPrev(TBTreeNode* node, int index) {
        TBTreeNode* child = node->Children[index];
        TBTreeNode* sibling = node->Children[index - 1];

        for (int i = child->N - 1; i >= 0; --i) {
            child->Keys[i + 1] = child->Keys[i];
        }

        if (!child->Leaf) {
            for (int i = child->N; i >= 0; --i) {
                child->Children[i + 1] = child->Children[i];
            }
        }

        child->Keys[0] = node->Keys[index - 1];

        if (!child->Leaf) {
            child->Children[0] = sibling->Children[sibling->N];
            sibling->Children[sibling->N] = nullptr;
        }

        node->Keys[index - 1] = sibling->Keys[sibling->N - 1];

        child->N += 1;
        sibling->N -= 1;
    }

    // берет ключ у правого соседа
    void BorrowFromNext(TBTreeNode* node, int index) {
        TBTreeNode* child = node->Children[index];
        TBTreeNode* sibling = node->Children[index + 1];

        child->Keys[child->N] = node->Keys[index];

        if (!child->Leaf) {
            child->Children[child->N + 1] = sibling->Children[0];
        }

        node->Keys[index] = sibling->Keys[0];

        for (int i = 1; i < sibling->N; ++i) {
            sibling->Keys[i - 1] = sibling->Keys[i];
        }

        if (!sibling->Leaf) {
            for (int i = 1; i <= sibling->N; ++i) {
                sibling->Children[i - 1] = sibling->Children[i];
            }
            sibling->Children[sibling->N] = nullptr;
        }

        child->N += 1;
        sibling->N -= 1;
    }

    // перед спуском делаем так, чтобы у ребенка было хотя бы T ключей
    void Fill(TBTreeNode* node, int index) {
        if (index != 0 && node->Children[index - 1]->N >= T) {
            BorrowFromPrev(node, index);
        } else if (index != node->N && node->Children[index + 1]->N >= T) {
            BorrowFromNext(node, index);
        } else {
            if (index != node->N) {
                Merge(node, index);
            } else {
                Merge(node, index - 1);
            }
        }
    }

    // удаление из внутреннего узла
    void RemoveFromNonLeaf(TBTreeNode* node, int index) {
        string key = node->Keys[index].Key;

        if (node->Children[index]->N >= T) {
            TPair pred = GetPredecessor(node, index);
            node->Keys[index] = pred;
            Remove(node->Children[index], pred.Key);
        } else if (node->Children[index + 1]->N >= T) {
            TPair succ = GetSuccessor(node, index);
            node->Keys[index] = succ;
            Remove(node->Children[index + 1], succ.Key);
        } else {
            Merge(node, index);
            Remove(node->Children[index], key);
        }
    }

    // основная рекурсивная функция удаления
    void Remove(TBTreeNode* node, const string& key) {
        int index = FindKey(node, key);

        // ключ найден в текущем узле
        if (index < node->N && node->Keys[index].Key == key) {
            if (node->Leaf) {
                RemoveFromLeaf(node, index);
            } else {
                RemoveFromNonLeaf(node, index);
            }
        } else {
            // дошли до листа и не нашли
            if (node->Leaf) {
                return;
            }

            bool flag = (index == node->N);

            if (node->Children[index]->N < T) {
                Fill(node, index);
            }

            if (flag && index > node->N) {
                Remove(node->Children[index - 1], key);
            } else {
                Remove(node->Children[index], key);
            }
        }
    }

    // собирает все пары в отсортированном порядке
    void CollectEntries(TBTreeNode* node, vector<TPair>& entries) const {
        if (node == nullptr) {
            return;
        }

        for (int i = 0; i < node->N; ++i) {
            if (!node->Leaf) {
                CollectEntries(node->Children[i], entries);
            }
            entries.push_back(node->Keys[i]);
        }

        if (!node->Leaf) {
            CollectEntries(node->Children[node->N], entries);
        }
    }

    // записывает строку в бинарный файл
    bool WriteString(ofstream& out, const string& s) const {
        uint32_t len = static_cast<uint32_t>(s.size());
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        if (!out) {
            return false;
        }

        if (len > 0) {
            out.write(s.data(), len);
            if (!out) {
                return false;
            }
        }

        return true;
    }

    // читает строку из бинарного файла
    bool ReadString(ifstream& in, string& s) const {
        uint32_t len = 0;
        in.read(reinterpret_cast<char*>(&len), sizeof(len));
        if (!in) {
            return false;
        }

        if (len > 256) {
            return false;
        }

        s.assign(len, '\0');
        if (len > 0) {
            in.read(&s[0], len);
            if (!in) {
                return false;
            }
        }

        return true;
    }

public:
    TBTree() : Root(nullptr) {
    }

    ~TBTree() {
        Clear();
    }

    // полностью очищает дерево
    void Clear() {
        Destroy(Root);
        Root = nullptr;
    }

    // поиск слова
    bool Find(const string& key, uint64_t& value) const {
        string normKey = Normalize(key);
        int pos = -1;
        TBTreeNode* node = Search(Root, normKey, pos);

        if (node == nullptr) {
            return false;
        }

        value = node->Keys[pos].Value;
        return true;
    }

    // вставка нового слова
    bool Insert(const string& key, uint64_t value) {
        string normKey = Normalize(key);

        uint64_t dummy = 0;
        if (Find(normKey, dummy)) {
            return false;
        }

        TPair entry;
        entry.Key = normKey;
        entry.Value = value;

        if (Root == nullptr) {
            Root = CreateNode(true);
            Root->Keys[0] = entry;
            Root->N = 1;
            return true;
        }

        if (Root->N == 2 * T - 1) {
            TBTreeNode* newRoot = CreateNode(false);
            newRoot->Children[0] = Root;

            SplitChild(newRoot, 0);

            int i = 0;
            if (entry.Key > newRoot->Keys[0].Key) {
                i = 1;
            }

            InsertNonFull(newRoot->Children[i], entry);
            Root = newRoot;
        } else {
            InsertNonFull(Root, entry);
        }

        return true;
    }

    // удаление слова
    bool Erase(const string& key) {
        if (Root == nullptr) {
            return false;
        }

        string normKey = Normalize(key);

        uint64_t dummy = 0;
        if (!Find(normKey, dummy)) {
            return false;
        }

        Remove(Root, normKey);

        // если корень опустел, уменьшаем высоту
        if (Root->N == 0) {
            TBTreeNode* oldRoot = Root;

            if (Root->Leaf) {
                Root = nullptr;
            } else {
                Root = Root->Children[0];
                oldRoot->Children[0] = nullptr;
            }

            delete oldRoot;
        }

        return true;
    }

    // сохранение словаря в бинарный файл
    bool Save(const string& path, string& errorMessage) const {
        ofstream out(path, ios::binary | ios::trunc);
        if (!out.is_open()) {
            errorMessage = "can't open file";
            return false;
        }

        vector<TPair> entries;
        CollectEntries(Root, entries);

        const char magic[4] = {'B', 'T', 'R', 'E'};
        out.write(magic, 4);
        if (!out) {
            errorMessage = "can't write file";
            return false;
        }

        uint32_t version = 1;
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));
        if (!out) {
            errorMessage = "can't write file";
            return false;
        }

        uint64_t count = static_cast<uint64_t>(entries.size());
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        if (!out) {
            errorMessage = "can't write file";
            return false;
        }

        for (const TPair& entry : entries) {
            if (!WriteString(out, entry.Key)) {
                errorMessage = "can't write file";
                return false;
            }

            out.write(reinterpret_cast<const char*>(&entry.Value), sizeof(entry.Value));
            if (!out) {
                errorMessage = "can't write file";
                return false;
            }
        }

        return true;
    }

    // загрузка словаря из бинарного файла
    bool Load(const string& path, string& errorMessage) {
        ifstream in(path, ios::binary);

        // отсутствие файла считаем пустым словарем
        if (!in.is_open()) {
            Clear();
            return true;
        }

        in.seekg(0, ios::end);
        streampos fileSize = in.tellg();
        in.seekg(0, ios::beg);

        // пустой файл тоже считаем пустым словарем
        if (fileSize == 0) {
            Clear();
            return true;
        }

        char magic[4];
        in.read(magic, 4);
        if (!in) {
            errorMessage = "bad file format";
            return false;
        }

        if (!(magic[0] == 'B' && magic[1] == 'T' && magic[2] == 'R' && magic[3] == 'E')) {
            errorMessage = "bad file format";
            return false;
        }

        uint32_t version = 0;
        in.read(reinterpret_cast<char*>(&version), sizeof(version));
        if (!in || version != 1) {
            errorMessage = "bad file format";
            return false;
        }

        uint64_t count = 0;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        if (!in) {
            errorMessage = "bad file format";
            return false;
        }

        TBTree tempTree;

        for (uint64_t i = 0; i < count; ++i) {
            string key;
            uint64_t value = 0;

            if (!ReadString(in, key)) {
                errorMessage = "bad file format";
                return false;
            }

            in.read(reinterpret_cast<char*>(&value), sizeof(value));
            if (!in) {
                errorMessage = "bad file format";
                return false;
            }

            for (char c : key) {
                if (!(c >= 'a' && c <= 'z')) {
                    errorMessage = "bad file format";
                    return false;
                }
            }

            if (!tempTree.Insert(key, value)) {
                errorMessage = "bad file format";
                return false;
            }
        }

        char extra;
        if (in.read(&extra, 1)) {
            errorMessage = "bad file format";
            return false;
        }

        Clear();
        Root = tempTree.Root;
        tempTree.Root = nullptr;

        return true;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    TBTree tree;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) {
            continue;
        }

        if (line[0] == '+') {
            // + word number
            istringstream iss(line);
            char op;
            string word;
            uint64_t value;

            iss >> op >> word >> value;

            if (tree.Insert(word, value)) {
                cout << "OK\n";
            } else {
                cout << "Exist\n";
            }
        } else if (line[0] == '-') {
            // - word
            istringstream iss(line);
            char op;
            string word;

            iss >> op >> word;

            if (tree.Erase(word)) {
                cout << "OK\n";
            } else {
                cout << "NoSuchWord\n";
            }
        } else if (line[0] == '!') {
            // ! Save path или ! Load path
            istringstream iss(line);
            char mark;
            string command;
            string path;

            iss >> mark >> command >> path;

            string errorMessage;

            if (command == "Save") {
                if (tree.Save(path, errorMessage)) {
                    cout << "OK\n";
                } else {
                    cout << "ERROR: " << errorMessage << '\n';
                }
            } else if (command == "Load") {
                if (tree.Load(path, errorMessage)) {
                    cout << "OK\n";
                } else {
                    cout << "ERROR: " << errorMessage << '\n';
                }
            }
        } else {
            // просто слово - это поиск
            uint64_t value = 0;
            if (tree.Find(line, value)) {
                cout << "OK: " << value << '\n';
            } else {
                cout << "NoSuchWord\n";
            }
        }
    }

    return 0;
}