#include <fstream>
#include <iostream>
#include <cstring>
#include <bitset>


using FileInfo = struct
{
    char file_name[11];
    uint8_t file_attr;
    uint8_t reserved;
    uint8_t creation_ms;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_visited_date;
    uint16_t elder_word;
    uint16_t last_change_time;
    uint16_t last_change_date;
    uint16_t first_cluster;
    uint32_t file_size;
};


void print_file_name(char file_name[])
{
    std::string file_name_ext {};
    bool name_started {false}, ext_exists {false};
    for (int i = 10; i >=0; --i)
    {
        if (i == 7 && ext_exists)
        {
            file_name_ext.insert(0, ".");
            continue;
        }
        if (file_name[i] == ' ' && !name_started)
            continue;
        
        if (i < 7)
            name_started = true;
        if (i > 7)
            ext_exists = true;

        file_name_ext.insert(0, std::string{file_name[i]});
    }
    std::cout << file_name_ext << std::endl;
}


void print_file_attributes(const FileInfo & cfile)
{
    if (cfile.file_size)
        printf("\tFile size: %d\n", cfile.file_size);
    else
        printf("\tFile is a directory\n");

    printf("\tFile's attributes: \n");
    if (cfile.file_attr & 0x01)
        printf("\t\tRead-only \n");
    if (cfile.file_attr & 0x02)
        printf("\t\tHidden \n");
    if (cfile.file_attr & 0x04)
        printf("\t\tSystem \n");
    if (cfile.file_attr & 0x08)
        printf("\t\tVolume label\n");
    if (cfile.file_attr & 0x0f)
        printf("\t\tLong file name\n");
    if (cfile.file_attr & 0x10)
        printf("\t\tDirectory\n");
    if (cfile.file_attr & 0x20)
        printf("\t\tArchive\n");
}


void print_date(uint16_t date, uint16_t time)
{
    std::cout << 1 + (date & 0b11111) << "/" << ((date >> 5) & 0b1111) << "/" << 1980 + ((date >> 9) & 0b1111111) << " ";
    std::cout << ((time >> 11) & 0b11111) << ":" << ((time >> 5) & 0b111111) << ":" << 2 * (time & 0b11111) << std::endl;
}


void print_file_info(FileInfo & file_info, const std::string & data, off_t offset)
{
    memcpy(&file_info, data.c_str() + offset, 32);
    if (file_info.file_name[0] == '\0') return;

    std::cout << "File name: ";
    print_file_name(file_info.file_name);

    print_file_attributes(file_info);
    
    std::cout << "\tCreation date & time: ";
    print_date(file_info.creation_date, file_info.creation_time);

    std::cout << "\tLast modification date & time: ";
    print_date(file_info.last_change_date, file_info.last_change_time);

    std::cout << "\tFirst cluster number: " << file_info.first_cluster << std::endl;
    std::cout << "\n";
}


int main(int argc, char * argv[])
{
    if (argc != 2)
    {
        std::cerr << "You should specify exactly one argument - image path" << std::endl;
        return -1;
    }
    std::ifstream infile{argv[1], std::ios::binary};
    std::string data((std::istreambuf_iterator<char>(infile)),
                     std::istreambuf_iterator<char>());

    std::cout << "Boot sector info:" << std::endl;

// - Розмір сектора
    uint16_t bytes_per_sector;
    memcpy(&bytes_per_sector, data.c_str() + 0x0B, 2 * 8);
    std::cout << "\tSector size (bytes): " << bytes_per_sector << std::endl;

// - Кількість секторів у кластері
    uint8_t sectors_per_cluster;
    memcpy(&sectors_per_cluster, data.c_str() + 0x0D, 1 * 8);
    std::cout << "\tCluster size (sectors): " << unsigned(sectors_per_cluster) << std::endl;

// - Кількість копій FAT
    uint8_t number_of_fats;
    memcpy(&number_of_fats, data.c_str() + 0x10, 1 * 8);
    std::cout << "\tFAT copies: " << unsigned(number_of_fats) << std::endl;

// - Розмір копії FAT у секторах та байтах
    uint16_t sectors_per_fat;
    memcpy(&sectors_per_fat, data.c_str() + 0x16, 2 * 8);
    std::cout << "\tFAT copy size (sectors): " << sectors_per_fat << std::endl;
    std::cout << "\tFAT copy size (bytes): " << sectors_per_fat * bytes_per_sector << std::endl;
    
// - Кількість записів кореневого каталогу, його розмір в байтах
    uint16_t root_entries;
    memcpy(&root_entries, data.c_str() + 0x11, 2 * 8);
    std::cout << "\tRoot entries: " << root_entries << std::endl;

// - Кількість зарезервованих секторів
    uint16_t reserved_sectors;
    memcpy(&reserved_sectors, data.c_str() + 0x0E, 2 * 8);
    std::cout << "\tReserved sectors: " << reserved_sectors << std::endl;

// - Чи є в кінці коректна сигнатура
    uint16_t boot_signature;
    memcpy(&boot_signature, data.c_str() + 0x1FE, 2 * 8);
    std::cout << std::boolalpha;
    std::cout << "\tIs boot signature correct: " << (boot_signature == 0xAA55) << std::endl;

    std::cout << "Root catalog info:" << std::endl;
    // offset for root catalog = size of boot sector + size of all FAT copies
    int off = bytes_per_sector * reserved_sectors + bytes_per_sector * sectors_per_fat * number_of_fats;
    FileInfo current_file;

    for (int i=0; i < root_entries ; ++i)
        print_file_info(current_file, data, off + 32 * i);

    return 0;
}

