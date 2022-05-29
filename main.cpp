#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <algorithm>

using namespace std;

struct comp//Функция сравнения, используется при сортировке вектора, так как нам нужно упорядочить элементы в таблице
{
    bool operator()(const pair<char, unsigned int> &left, const pair<char, unsigned int> &right)
    {
        if (left.second != right.second)
            return left.second >= right.second;
        return left.first < right.first;
    }
};

int input_bit(unsigned char* string_of_bits, unsigned int* bit_len, FILE* input, unsigned int* garbage_bits)//Функция возвращающая бит
{
    if ((*bit_len) == 0)//Если нет записанных битов
    {
        (*string_of_bits) = fgetc(input);//Читаем символ
        if (feof(input))//Если файл закончился записываем ненужные биты
        {
            (*garbage_bits)++;
            if ((*garbage_bits) > 14)
                throw std::invalid_argument("Can't decompress");
        }
        (*bit_len) = 8;
    }
    int t = (*string_of_bits) & 1;//Запоминаем последний бит
    (*string_of_bits) >>= 1;
    (*bit_len)--;
    return t;
}

void decoder(const char* file_name = "output.txt", const char* encoded_name = "decoded.txt")
{
    unsigned int *alphabet = new unsigned int[256];
    for (int i = 0; i < 256; i++)
        alphabet[i] = 0;
    FILE *input = fopen(file_name, "rb");
    if (input == nullptr)
        throw invalid_argument("File not found.");
    char header = fgetc(input);
    int all_letters = (int)header;//Смотрим сколько символов используется в тексте
    char letter;
    for(int i = 0; i < all_letters; i++)//После чего заполняем алфавит
    {
        letter = fgetc(input);
        fread(reinterpret_cast<char*>(&alphabet[letter]), sizeof(unsigned int), 1, input);
    }
    vector <pair<char, unsigned int>> table;//Формирование таблицы в виде пар: буква - количество вхождений
    for (int i = 0; i < 256; i++)
        if (alphabet[i] != 0)
            table.emplace_back((char) i, alphabet[i]);
    sort(table.begin(), table.end(), comp());//Сортировка таблицы
    cout << "------Alphabet------" << endl;
    for (auto pair: table)
        cout << pair.first << " " << pair.second << endl;
    unsigned int *intervals = new unsigned int[table.size() + 2];//Формирование таблицы целочисленных интервалов
    intervals[0] = 0;
    intervals[1] = 1;
    for (int i = 0; i < table.size(); i++)
    {
        unsigned int b = table[i].second;
        for (int j = 0; j < i; j++)
            b += table[j].second;
        intervals[i + 2] = b;
    }
    if (intervals[table.size()] > (1 << 13))
        throw std::invalid_argument("Error, too long count.");
    FILE *output = fopen(encoded_name, "wb");
    unsigned int lower_bound = 0, higher_bound = ((static_cast<unsigned int>(1) << 16) - 1), delitel = intervals[table.size() + 1];
    unsigned int first_qtr = (higher_bound + 1) / 4, half = first_qtr * 2, third_qtr = first_qtr * 3;
    unsigned int value = 0;//Переменная хранящая значение кода
    unsigned int difference = higher_bound - lower_bound + 1;//Вводим переменную разницы, так как нет возможности хранить предыдущие значения границ
    unsigned int bit_len = 0;//Переменная, обозначающая сколько битов в символьной переменной осталось незаполненными
    unsigned int garbage_bits = 0;
    unsigned char string_of_bits = 0;//Символьная переменная куда будет записываться результат работы алгоритма
    for (int i = 1; i <= 16; i++)
        value = 2 * value + input_bit(&string_of_bits, &bit_len, input, &garbage_bits);
    for (;;)//Начало декодирования
    {
        unsigned int freq = ((value - lower_bound + 1) * delitel - 1) / difference;//Рассчитываем общую частоту
        int j;
        for (j = 1; intervals[j] <= freq; j++);//Находим нужный символ
        higher_bound = lower_bound +  intervals[j] * difference / delitel - 1;
        lower_bound = lower_bound + intervals[j - 1] * difference / delitel;
        for (;;)
        {
            if (higher_bound < half)//Если верхняя граница лежит в первой половине ничего не делаем
                ;
            else if (lower_bound >= half)//Если нижняя граница лежит во второй половине смещаем значения на половину
            {
                lower_bound -= half;
                higher_bound -= half;
                value -= half;
            }
            else if ((lower_bound >= first_qtr) && (higher_bound < third_qtr))//Если обе границы лежат во второй четверти смещаем значения на четверть
            {
                lower_bound -= first_qtr;
                higher_bound -= first_qtr;
                value -= first_qtr;
            }
            else
                break;
            lower_bound += lower_bound;
            higher_bound += higher_bound + 1;
            value += value + input_bit(&string_of_bits, &bit_len, input, &garbage_bits);;
        }
        if (j == 1)//Заканчиваем когда дойдем до конца массива интервалов
            break;
        fputc(table[j-2].first, output);//Записываем декодированный символ
        difference = higher_bound - lower_bound + 1;
    }
    cout << "You did well!";
    fclose(input);//Проверка на правильность декодирования
    fclose(output);
    FILE*  final = fopen("decoded.txt", "rb");
    FILE* initial = fopen("input.txt", "rb");
    char after = 0, before = 0;
    while (!feof(initial) && !feof(final))
    {
        after = fgetc(final);
        before = fgetc(initial);
        if(!feof(initial) && !feof(final))
            if(after != before)
            {
                cout << "Not OK!";
                return;
            }
    }
    fclose(final);
    fclose(initial);
}
int main()
{
    decoder();
    return 0;
}
