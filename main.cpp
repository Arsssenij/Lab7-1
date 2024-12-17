#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <boost/crc.hpp> // Для CRC32
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <regex>
#include <set> // Для хранения дубликатов

namespace fs = std::filesystem;

// Функция для вычисления CRC32
uint32_t calculate_crc32(const std::string& data) {
    boost::crc_32_type crc; // Создаем объект для вычисления CRC32
    crc.process_bytes(data.data(), data.size()); // Обрабатываем данные
    return crc.checksum(); // Возвращаем полученную контрольную сумму
}

// Функция для обработки файла и получения последовательности хэшей
std::vector<uint32_t> file_processing(const fs::path& filePath, size_t blockSize) {
    std::vector<uint32_t> hashSequence; // Вектор для хранения хэшей блоков
    std::ifstream file(filePath, std::ios::binary); // Открываем файл в бинарном режиме

    if (!file) {
        throw std::runtime_error("Cannot open file: " + filePath.string());
    }

    std::string buffer(blockSize, '\0'); // Буфер для чтения блоков

    while (file.read(&buffer[0], blockSize) || file.gcount() > 0) { // чтение данных из файла блоками
        // Урезаем буфер до фактического размера прочитанных данных
        size_t bytesRead = static_cast<size_t>(file.gcount()); // количество прочитанных байтов
        buffer.resize(bytesRead); // обновление размера буфера до количества прочитанных байтов
        
        // Дополняем до размера блока нулями
        if (bytesRead < blockSize) {
            buffer.resize(blockSize, '\0');
        }

        // Вычисляем хэш блока и добавляем в последовательность
        uint32_t hash = calculate_crc32(buffer);
        hashSequence.push_back(hash);
    }

    return hashSequence; // Возвращаем вектор хэшей
}

// Функция для сравнения двух файлов по их хэшам
bool compare_hashes(const std::vector<uint32_t>& hashes1, const std::vector<uint32_t>& hashes2) {
    if (hashes1.size() != hashes2.size()) return false; // Если размеры разные, файлы разные

    for (size_t i = 0; i < hashes1.size(); ++i) {
        if (hashes1[i] != hashes2[i]) return false; // Если хотя бы один хэш не совпадает, файлы разные
    }

    return true; // Файлы идентичны
}

void shouldProcessFile(const fs::directory_entry& entry, const std::vector<fs::path>& exclusions, size_t minSize, const std::regex& maskRegex, size_t blockSize, std::vector<std::pair<fs::path, std::vector<uint32_t>>>& hashVector) {
    if (entry.is_regular_file()) {  // Проверяем, является ли это обычным файлом
        // Проверка на исключения. если родительская директория в списке исключений, пропускаем файл
        if (std::find(exclusions.begin(), exclusions.end(), entry.path().parent_path()) != exclusions.end()) {
            return;
        }

        // Проверка минимального размера файла
        if (entry.file_size() < minSize) {
            return;
        }

        // Проверка маски имени файла
        if (!std::regex_match(entry.path().filename().string(), maskRegex)) {
            return;
        }
        auto hashes = file_processing(entry.path(), blockSize); // Получаем хэши файла по блокам
        // Сохраняем путь к файлу и его хэши в hashVector
        hashVector.emplace_back(entry.path(), hashes);    
    }
}

// Функция для поиска дубликатов
void find_duplicates(const std::vector<fs::path>& directories, const std::vector<fs::path>& exclusions, size_t blockSize, size_t minSize, std::regex& maskRegex, int scanLevel) {
    std::unordered_map<std::string, std::set<fs::path>> hashMap; // Словарь для хранения путей дубликатов
    std::vector<std::pair<fs::path, std::vector<uint32_t>>> hashVector; // Вектор для хранения всех обработанных файлов и их хэшей

    for (const auto& dir : directories) {
        if (!fs::exists(dir) || !fs::is_directory(dir)) {
            std::cerr << "Директория не существует или не является директорией: " << dir << std::endl;
            continue;
        }

        // Выбор итератора в зависимости от уровня сканирования
        if (scanLevel == 0) { // Только указанная директория без вложенных
            for (const auto& entry : fs::directory_iterator(dir)) {
                shouldProcessFile(entry, exclusions, minSize, maskRegex, blockSize, hashVector);
                
            }
        } else { // Рекурсивное сканирование
            for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                shouldProcessFile(entry, exclusions, minSize, maskRegex, blockSize, hashVector);
                
            }
        }
    }
    // Сравнение хешей и добавление дубликатов в hashMap
    for (size_t i = 0; i < hashVector.size(); ++i) {
        for (size_t j = i + 1; j < hashVector.size(); ++j) {
            if (compare_hashes(hashVector[i].second, hashVector[j].second)) {  // Если файлы идентичны по хэшам
                std::string hashKey(reinterpret_cast<const char*>(hashVector[i].second.data()), hashVector[i].second.size() * sizeof(uint32_t)); // последовательность байтов из вектора хэшей
                hashMap[hashKey].insert(hashVector[i].first);
                hashMap[hashKey].insert(hashVector[j].first);
            }
        }
    }

    // Вывод результатов
    for (const auto& pair : hashMap) {
        std::cout << "Дубликаты:\n";
        for (const auto& file : pair.second) {
            std::cout << file << std::endl;
        }
        std::cout << std::endl;
        
    }

}

int main() {
    std::vector<fs::path> directories;
    std::vector<fs::path> exclusions;
    size_t blockSize = 4096; // Значение по умолчанию
    size_t minSize = 1;      // Минимальный размер файла по умолчанию
    int scanLevel = 1;       // Уровень сканирования по умолчанию (1 - рекурсивное)

    // Запрос директорий для сканирования
    int numDirs;
    std::cout << "Введите количество директорий для сканирования: ";
    std::cin >> numDirs;

    for (int i = 0; i < numDirs; ++i) {
        fs::path dir;
        std::cout << "Введите путь к директории " << (i + 1) << ": ";
        std::cin >> dir;
        directories.push_back(dir);
    }

    // Запрос директорий для исключения
    int numExclusions;
    std::cout << "Введите количество директорий для исключения: ";
    std::cin >> numExclusions;

    for (int i = 0; i < numExclusions; ++i) {
        fs::path exclDir;
        std::cout << "Введите путь к директории для исключения " << (i + 1) << ": ";
        std::cin >> exclDir;
        exclusions.push_back(exclDir);
    }

    // Запрос уровня сканирования
    std::cout << "Введите уровень сканирования (0 - только указанная директория без вложенных, 1 - рекурсивное): ";
    std::cin >> scanLevel;

    // Запрос маски имен файлов
    std::string maskString;
    std::cout << "Введите маску имен файлов разрешенных для сравнения (например, *.txt или file?.txt): ";
    std::cin >> maskString;

    // Преобразуем маску в регулярное выражение
    std::regex star_regex("\\*");
    std::regex question_regex("\\?");
    maskString = "^" + std::regex_replace(maskString, star_regex, ".*"); // Заменяем * на .*
    maskString = std::regex_replace(maskString, question_regex, "."); // Заменяем ? на .
    maskString += "$"; // Добавляем конец строки

    std::cout << "Регулярное выражение: " << maskString << std::endl;
    
    // Запрос минимального размера файла
    std::cout << "Введите минимальный размер файла (по умолчанию 1): ";
    size_t inputminSize;
    if (std::cin >> inputminSize && inputminSize > 0) {
        minSize = inputminSize;
    }

    // Запрос размера блока
    std::cout << "Введите размер блока (по умолчанию 4096): ";
    size_t inputBlockSize;
    if (std::cin >> inputBlockSize && inputBlockSize > 0) {
        blockSize = inputBlockSize;
    }

    try {
        std::regex maskRegex(maskString, std::regex_constants::icase); // Игнорируем регистр

        find_duplicates(directories, exclusions, blockSize, minSize, maskRegex, scanLevel);
        
    } catch (const std::regex_error& e) {
        std::cerr << "Ошибка в регулярном выражении: " << e.what() << '\n';
        return 1;
    }
    return 0;
}