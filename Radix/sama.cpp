#include <cstdio>
#include <cstring>


struct Record { //строка+ключ для сортировки [][]
    char* line; //указатель на массив с исх строка чтоб потом вывести ее
    unsigned char* digits; //указатель на массив с цифры номера телефона отдельно
    int digits_len; //колво цифр 
};

static bool is_digit(char c){ //
    return c >= '0' && c <= '9';
}

static int get_digit_from_right(const Record& r, int pos_from_right){ //const не изменяю, Record& не копирую а ссылаюсь
    if (pos_from_right >= r.digits_len) return 0; //если слева цифр нет то считаю что там 0
    return (int)r.digits[r.digits_len - 1 - pos_from_right]; //иначе вызвр цифру на этой позиции
}

static void counting_sort_by_pos(Record* a, Record* tmp, int n, int pos_from_right){
    int count[10];
    //o(k)+
    for (int i = 0; i < 10; ++i) {
        count[i] = 0;} //массив нулей

    //o(n)
    for (int i = 0; i < n; ++i){
        int d = get_digit_from_right(a[i], pos_from_right);
        ++count[d]; //для кажлой цифры записываем скок раз она встр
    }

    //o(k)
    for (int d = 1; d < 10; ++d){ //prefix_sum
        count[d] += count[d - 1]; //для позиций кадого числа те показываю позиция конца блока цифры d
    }
    
    //o(n)
    for (int i = n - 1; i >= 0; --i){ //обр проход для сохр относит порядока элементов с одинаковой цифрой 
        int d = get_digit_from_right(a[i], pos_from_right);//беру разряд справа
        int pos = --count[d]; //смотрю на какую позициб(prefix_sum-1) поставить число у которого я см разряд
        tmp[pos] = a[i]; //ставлю на эту позицию число
    }
    
    //o(n)
    for (int i = 0; i < n; ++i){
        a[i] = tmp[i];//перезапис осн массив отсортированным
    }

} //o(k)+o(n)+o(k)+o(n)+o(n)=2*o(k)+3*o(n)=o(k+n)=o(n)
//radix проходит D раз => o(Dn) = o(n)


static void radix_sort(Record* a, int n, int max_digits){ 
    if (n <= 1) return;

    Record* tmp = new Record[n]; // 

    for (int pos = 0; pos < max_digits; ++pos){
        counting_sort_by_pos(a, tmp, n, pos);
    }
    delete[] tmp;
}


static void reserve_more(Record*& a, int& cap){
    int new_cap = (cap == 0) ? 1024 : (cap * 2);
    Record* b = new Record[new_cap]; //выделяем массив структур record
    for (int i = 0; i < cap; ++i){
        b[i] = a[i];
    }
    delete[] a;
    a = b;
    cap = new_cap;
}


int main() {

    const int BUF_SIZE = 4096; 
    char buf[BUF_SIZE]; 


    Record* arr = 0; //п дин массив для структур
    int n = 0; //сколько записей всего
    int cap = 0; //сколько выделено памяти 
    int max_digits = 0; //макс длина номера телефона(колво проходов radix)

    
    while (std::fgets(buf, BUF_SIZE, stdin)){ //buf-куда считываем, BUF_SIZE-макс колво записей, stdin откуда ждем ввод 
        //чистим строку
        int len = (int)std::strlen(buf); //strlen считает длину введенной строки
        //в конце строки может быть \r\n -> заменяем все это на \0
        while (len > 0 && ((buf[len -1] == '\n') || (buf[len - 1] == '\r')))
        {
            buf[--len] = '\0';
        }

        if (len == 0) continue;

        //ищем \t для разделения на ключ \t значение

        int tab = -1;
        for (int i = 0; i < len; ++i) {
            if (buf[i] == '\t') {
                tab = i; 
                break;}
        }

        if (tab == -1) {
            tab = len;}

        //считаем колво цифр в ключе
        int digits_cnt = 0;
        for (int i = 0; i < tab; i++){
            if (is_digit(buf[i])) ++digits_cnt;
        }

        //создаем block [line][digits]
        char* block = new char[len + 1 + digits_cnt]; //выделяем блок такого размера
        std::memcpy(block, buf, len + 1); //копируем каждую строку в отдельный блок памяти иначе строки бы затирались 
        //те block независимая копия строки
        unsigned char* digits = (unsigned char*)(block + len + 1);//!! block это указатель -> block+len+1 это указатель на место где начинается [digits]


        //заполняем digits
        int k = 0;
        for (int i = 0; i < tab; i++){
            if (is_digit(block[i])){
                digits[k++] = (unsigned char)(block[i] - '0'); //ascii тк в block у нас строка '7'в ascii это 55, '0' это 48 -> поэтому при вычитании строк получаем число
            }
        }
        
        if (n == cap){
            reserve_more(arr, cap);
        }

        //добавляем запись в массив 
        arr[n].line = block;
        arr[n].digits = digits;
        arr[n].digits_len = digits_cnt;
        ++n;

        if (digits_cnt > max_digits) max_digits = digits_cnt;
        }

    //radix sort
    radix_sort(arr, n, max_digits);

    //вывод
    for (int i = 0; i < n; ++i){
        std::fputs(arr[i].line, stdout); //выводит целую строку но без \n
        std::fputc('\n', stdout); //выводит один символ
    }

    //free
    for (int i = 0; i < n; ++i){
        delete[] arr[i].line; //удаляем пристройки к массиву
    }
    delete[] arr; //удаляем массив

    return 0;

}