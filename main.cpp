#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <sys/stat.h>

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

int indexForSymbol(char c, vector<pair<char, unsigned int>> table)//Функция, возвращающая номер буквы в таблице
{
    for (int i = 0; i < table.size(); i++)
        if (c == table[i].first)
            return i+2;//Возвращаем на 2 больше, так как ищем в таблице интервалов
    return -1;
}

void WriteBit(unsigned int bit, unsigned int* bit_len, unsigned char* string_of_bits, FILE* output)//Функция, записывающая биты в файл
{
    (*string_of_bits) >>= 1;//Бит записываем в начало, поэтому сначала делаем сдвиг вправо
    if (bit)
        (*string_of_bits) |= (1 << 7);
    (*bit_len)--;
    if ((*bit_len) == 0)//В файл делаем запись когда кончается длина переменной, в которую заносятся биты
    {
        fputc((*string_of_bits), output);
        (*bit_len) = 8;
    }
}

void BitsPlusFollow(unsigned int bit, unsigned int* bits_to_follow, unsigned int* bit_len, unsigned char* string_of_bits, FILE* output)//Перенос найденных битов в файл
{
    WriteBit(bit, bit_len, string_of_bits, output);
    for (; (*bits_to_follow) > 0; (*bits_to_follow)--)
        WriteBit(!bit, bit_len, string_of_bits, output);
}

void coder(const char* file_name = "input.txt", const char* encoded_name = "output.txt") {
    unsigned int *alphabet = new unsigned int[256];
    for (int i = 0; i < 256; i++)
        alphabet[i] = 0;
    FILE *input = fopen(file_name, "rb");
    if (input == nullptr)
        throw invalid_argument("File not found.");
    char letter;
    while (!feof(input))//Формирование начального алфавита, составляющего текст
    {
        letter = fgetc(input);
        if (!feof(input))
            alphabet[letter]++;
    }
    fclose(input);
    vector<pair<char, unsigned int>> table;//Формирование таблицы в виде пар: буква - количество вхождений
    for (int i = 0; i < 256; i++)
        if (alphabet[i] != 0)
            table.emplace_back((char) i, alphabet[i]);
    sort(table.begin(), table.end(), comp());//Сортировка таблицы
    cout << "------Alphabet------" << endl;
    for (auto pair: table)
    {
        cout << pair.first << " " << pair.second << endl;
    }
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
    {
        throw std::invalid_argument("Error, too long count.");
    }
    input = fopen(file_name, "rb");
    FILE *output = fopen(encoded_name, "wb");
    char all_letters = table.size();//Запоминаем количество используемых симолов, необходимо для шапки
    fputc(all_letters, output);
    for (int i = 0; i < 256; i++)//Формируем шапку, которая понадобится при декодировании: записываем символ + количество его вхождений(в виде массива символов)
        if (alphabet[i] != 0)
        {
            fputc((char) i, output);
            fwrite(reinterpret_cast<const char *>(&alphabet[i]), sizeof(unsigned int), 1, output);
        }
    unsigned int lower_bound = 0, higher_bound = ((static_cast<unsigned int>(1) << 16) - 1), delitel = intervals[table.size() + 1];
    unsigned int first_qtr = (higher_bound + 1) / 4, half = first_qtr * 2, third_qtr = first_qtr * 3;
    unsigned int bits_to_follow = 0;//Переменная показывающая сколько битов записаны "условно"
    unsigned int difference = higher_bound - lower_bound + 1;//Вводим переменную разницы, так как нет возможности хранить предыдущие значения границ
    unsigned int bit_len = 8;//Переменная, обозначающая сколько битов в символьной переменной осталось незаполненными
    unsigned char string_of_bits = 0;//Символьная переменная куда будет записываться результат работы алгоритма
    int j = 1;
    while (!feof(input))//Алгоритм кодирования
    {
        letter = fgetc(input);
        if (!feof(input))
        {
            j = indexForSymbol(letter, table);//Находим индекс символа в таблице инервалов
            higher_bound = lower_bound + intervals[j] * difference / delitel - 1;//Рассчитываем новые границы
            lower_bound = lower_bound + intervals[j - 1] * difference / delitel;
            for (;;)
            {
                if (higher_bound < half)//Если верхняя граница лежит в первой половине то записываем 0
                    BitsPlusFollow(0, &bits_to_follow, &bit_len, &string_of_bits, output);
                else if (lower_bound >= half)//Если нижняя граница лежит во второй половине записываем 1 и меняем границы
                {
                    BitsPlusFollow(1, &bits_to_follow, &bit_len, &string_of_bits, output);
                    lower_bound -= half;
                    higher_bound -= half;
                }
                else if ((lower_bound >= first_qtr) && (higher_bound < third_qtr))//Если обе границы лежат во второй четверти записываем бит "условно" и меняем границы
                {
                    bits_to_follow++;
                    lower_bound -= first_qtr;
                    higher_bound -= first_qtr;
                }
                else//Иначе заканчиваем цикл
                    break;
                lower_bound += lower_bound;
                higher_bound += higher_bound + 1;
            }
        }
        else//Кодирование конечного символа
        {
            higher_bound = lower_bound + intervals[1] * difference / delitel - 1;
            lower_bound = lower_bound + intervals[0] * difference / delitel;
            for (;;)
            {
                if (higher_bound < half)
                    BitsPlusFollow(0, &bits_to_follow, &bit_len, &string_of_bits, output);
                else if (lower_bound >= half)
                {
                    BitsPlusFollow(1, &bits_to_follow, &bit_len, &string_of_bits, output);
                    lower_bound -= half;
                    higher_bound -= half;
                }
                else if ((lower_bound >= first_qtr) && (higher_bound < third_qtr))
                {
                    bits_to_follow++;
                    lower_bound -= first_qtr;
                    higher_bound -= first_qtr;
                }
                else
                    break;
                lower_bound += lower_bound;
                higher_bound += higher_bound + 1;
            }
            bits_to_follow++;
            if (lower_bound < first_qtr)//Записываем конечный бит в файл, если нижняя граница лежит в первой четверти то 0, иначе 1
                BitsPlusFollow(0, &bits_to_follow,&bit_len, &string_of_bits, output);
            else
                BitsPlusFollow(1, &bits_to_follow,&bit_len, &string_of_bits, output);
            string_of_bits >>= bit_len;//Записываем остаток
            fputc(string_of_bits, output);
        }
        difference = higher_bound - lower_bound + 1;
    }
    cout << "You did well!" << endl;
    fclose(input);
    fclose(output);
    long double file_full_size = 0, compress_size = 0;//Проверяем коэффициент сжатия
    struct stat sb{};
    struct stat se{};
    if (!stat(file_name, &sb))
        file_full_size = sb.st_size;
    else
        perror("stat");
    if (!stat(encoded_name, &se))
        compress_size = se.st_size;
    else
        perror("stat");
    cout << compress_size / file_full_size;
}

int main()
{
    coder();
}
