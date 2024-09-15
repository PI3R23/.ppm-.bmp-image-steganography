#include <iostream>
#include <fmt/core.h>
#include <fmt/format.h>
#include <map>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <fstream>
#include <bitset>
#include <fmt/ostream.h>
#include "fmt/color.h"
auto noFilePermissionHandler(){
    fmt::print(fg(fmt::color::red),"no file permission \n");
}
auto validateIfFilePathExists(std::string& path) -> bool {
    return std::filesystem::exists(std::filesystem::path(path));
}
auto validateFileExtension(std::string& path) -> bool{
    auto pictureSignature = std::string();
    auto file = std::fstream(path);
    if(!file.is_open()) {
        noFilePermissionHandler();
        return false;
    }
    file>>pictureSignature;
    file.close();
    if(pictureSignature.substr(0,2) == "BM" || pictureSignature == "P3")
        return true;

    return false;
}
auto txtToBin(const std::string& str) -> std::string{
    auto res = std::string();

    for(auto s:str) {
        std::bitset<7> vbinary((int)s);
        res+=vbinary.to_string();
    }
    return res;
}
auto findMessage(std::vector<char>& vecOfLastBytes, const std::string& key) -> std::string{
    auto stringRes = std::vector<char>();
    auto res = std::string();
    auto flagCounter=0;
    auto indexes = std::vector<int>();
    for (int i = 0; i < vecOfLastBytes.size(); ++i) {
        auto tmp = std::string();
        for (int j = 0; j < 21; ++j) {
            tmp+=vecOfLastBytes[i+j];
        }
        if(tmp==key)
        {
            flagCounter++;
            indexes.push_back(i);
        }
    }
    if(flagCounter==2)
        for (int i = indexes[0]+21; i < indexes[1]; ++i)
            stringRes.push_back(vecOfLastBytes[i]);

    auto tmpSevenBytes = std::string();

    for (int i = 0; i < stringRes.size(); ++i) {
        tmpSevenBytes+=stringRes[i];
        if(i%7==6 && i!=0) {
            res += std::stoi(tmpSevenBytes, nullptr, 2);
            tmpSevenBytes="";
        }
    }
    return res;
}
auto infoForPPM(const std::string& filePath) {
    auto file = std::fstream(filePath);
    if(!file.is_open()) {
        noFilePermissionHandler();
        return 1;
    }
    auto tempString = std::string();
    auto allVec = std::vector<std::string>();

    auto dataStartIndex = 4;

    while (file >> tempString) {
        allVec.push_back(tempString);
    }
    if (allVec[1] == "#") {
        auto i = 1;
        while (!isdigit(allVec[i][0])) {
            dataStartIndex++;
            i++;
        }
    }
    file.close();
    fmt::println("image resolution: {}x{}", allVec[dataStartIndex - 3], allVec[dataStartIndex - 2]);
    return 0;
}
auto infoForBMP(const std::string& filePath) {
    auto file = std::fstream (filePath,std::fstream::in | std::ios::binary);
    if(!file.is_open()) {
        noFilePermissionHandler();
        return 1;
    }
    file.seekg(0, std::ios::end);
    int fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    auto allVec = std::vector<unsigned char>(fileSize);
    file.read(reinterpret_cast<char*>(allVec.data()), fileSize);

    int imageWidth = allVec[18] + (allVec[19]<<8) + (allVec[20]<<16) + (allVec[21]<<24);
    int imageHeight = allVec[22] + (allVec[23]<<8) + (allVec[24]<<16) + (allVec[25]<<24);

    file.close();
    fmt::println("image resolution: {}x{}",imageWidth,imageHeight);
    return 0;
}
auto encryptMessageInBMP(const std::string& filePath, std::string message, std::string key = "PJC"){
    auto file = std::fstream (filePath,std::fstream::in | std::ios::binary);
    if(!file.is_open()) {
        noFilePermissionHandler();
        return 1;
    }


    auto colorVec = std::vector<std::string>();

    file.seekg(0, std::ios::end);
    int fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    auto allVec = std::vector<unsigned char>(fileSize);
    file.read(reinterpret_cast<char*>(allVec.data()), fileSize);

    int dataStart = allVec[10] + (allVec[11]<<8) + (allVec[12]<<16) + (allVec[13]<<24);

    auto charCapacity = ((fileSize-dataStart)/7)-6;

    if(message.size()>charCapacity){
        fmt::print(fg(fmt::color::red),"your message is too big!!!\n");
        fmt::println("message length {}, available space in current picture: {}",message.size(),charCapacity);
        return 1;
    }

    auto outputFile = std::fstream (filePath.substr(0,filePath.size()-4)+"ENCRYPTED.bmp" , std::fstream::out | std::ios::binary);

    for (int i = dataStart; i < allVec.size(); ++i) {
        std::bitset<8> binaryForm(allVec[i]);
        colorVec.push_back(binaryForm.to_string());
    }

    for(auto& bit:colorVec){
        std::ranges::reverse(bit);
    }

    key = txtToBin(key);
    message = txtToBin(message);

    for (int i = 0; i < key.size(); ++i) {
        colorVec[i].pop_back();
        colorVec[i].push_back(key[i]);
    }
    for (int i = key.size(); i < key.size()+message.size(); ++i) {
        colorVec[i].pop_back();
        colorVec[i].push_back(message[i-key.size()]);
    }
    for (int i = key.size()+message.size(); i < 2*key.size()+message.size(); ++i) {
        colorVec[i].pop_back();
        colorVec[i].push_back(key[i-(key.size()+message.size())]);
    }

    for(auto& bit:colorVec){
        std::ranges::reverse(bit);
    }

    auto index=0;
    for (int i = dataStart; i < allVec.size(); ++i) {
        auto intValue = stoi(colorVec[index++], nullptr, 2);
        auto ucharValue = static_cast<unsigned char>(intValue);
        allVec[i]=ucharValue;
    }

    outputFile.write(reinterpret_cast<char*>(allVec.data()), fileSize);

    file.close();
    outputFile.close();
    return 0;
}
auto decryptMessageFromBMP(const std::string& filePath,const std::string& key = "PJC"){
    auto file = std::fstream (filePath);

    if(!file.is_open()) {
        noFilePermissionHandler();
        return 1;
    }

    file.seekg(0, std::ios::end);
    int fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    auto allVec = std::vector<unsigned char>(fileSize);
    auto lastBytes = std::vector<char>();

    file.read(reinterpret_cast<char*>(allVec.data()), fileSize);

    int dataStart = allVec[10] + (allVec[11]<<8) + (allVec[12]<<16) + (allVec[13]<<24);

    for (int i = dataStart; i < allVec.size(); ++i) {
        std::bitset<8> binaryForm(allVec[i]);
        lastBytes.push_back(binaryForm.to_string().front());
    }
    file.close();
    fmt::print("hidden message: ");
    fmt::print(fmt::emphasis::bold,"{}\n",findMessage(lastBytes,txtToBin(key)));
    return 0;
}
auto encryptMessageInPPM(const std::string& filePath, std::string message, std::string key = "PJC"){
    auto file = std::fstream (filePath);
    if(!file.is_open()) {
        noFilePermissionHandler();
        return 1;
    }

    auto eachChar = std::string();
    auto allVec = std::vector<std::string>();
    auto colorVec = std::vector<std::string>();

    auto charCapacity =0;
    auto dataStartIndex = 4;

    while(file >> eachChar){
        allVec.push_back(eachChar);
    }

    if(allVec[1]=="#"){
        auto i=1;
        while(!isdigit(allVec[i][0])){
            dataStartIndex++;
            i++;
        }
    }

    for (int i = dataStartIndex; i < allVec.size(); ++i) {
        charCapacity++;
        std::bitset<8> binaryForm(std::stoi(allVec[i]));
        colorVec.push_back(binaryForm.to_string());
    }

    key = txtToBin(key);
    message = txtToBin(message);

    if(message.size()>charCapacity){
        fmt::print(fg(fmt::color::red),"your message is too big!!!\n");
        fmt::println("message length {}, available space in current picture: {}",message.size(),charCapacity);
        return 1;
    }

    auto outputFile = std::fstream (filePath.substr(0,filePath.size()-4)+"ENCRYPTED.ppm" , std::fstream::out);

    for (int i = 0; i < key.size(); ++i) {
        colorVec[i].pop_back();
        colorVec[i].push_back(key[i]);
    }
    for (int i = key.size(); i < key.size()+message.size(); ++i) {
        colorVec[i].pop_back();
        colorVec[i].push_back(message[i - key.size()]);
    }
    for (int i = key.size()+message.size(); i < 2*key.size()+message.size(); ++i) {
        colorVec[i].pop_back();
        colorVec[i].push_back(key[i - (key.size()+message.size())]);
    }

    fmt::println(outputFile,"{}",allVec[0]);
    fmt::print(outputFile,"{} {}",allVec[dataStartIndex-3],allVec[dataStartIndex-2]);
    fmt::println(outputFile,"");
    fmt::println(outputFile,"{}",allVec[dataStartIndex-1]);


    for (int i = 0; i <colorVec.size() ; i+=12) {
        auto vec = std::vector<int>();
        for (int j = 0; j < 12; ++j) {
            vec.push_back(std::stoi(colorVec[i+j], nullptr,2));
        }
        std::string formattedString = fmt::format("{}", fmt::join(vec, " "));
        fmt::println(outputFile,"{}",formattedString);
    }

    file.close();
    outputFile.close();
    return 0;
}
auto decryptMessageFromPPM(const std::string& filePath,const std::string& key = "PJC"){
    auto file = std::fstream (filePath);
    if(!file.is_open()) {
        noFilePermissionHandler();
        return 1;
    }
    auto eachChar = std::string();
    auto allVec = std::vector<std::string>();
    auto colorVec = std::vector<std::string>();
    auto lastBytes = std::vector<char>();

    auto dataStartIndex = 4;

    while(file >> eachChar){
        allVec.push_back(eachChar);
    }

    if(allVec[1]=="#"){
        auto i=1;
        while(!isdigit(allVec[i][0])){
            dataStartIndex++;
            i++;
        }
    }

    for (int i = dataStartIndex; i < allVec.size(); ++i) {
        std::bitset<8> binaryForm(std::stoi(allVec[i]));
        colorVec.push_back(binaryForm.to_string());
    }

    for (auto bit : colorVec) {
        lastBytes.push_back(bit.back());
    }

    file.close();
    fmt::print("hidden message: ");
    fmt::print(fmt::emphasis::bold,"{}\n",findMessage(lastBytes,txtToBin(key)));
    return 0;
}
auto checkPPMFile(const std::string& filePath,std::string message,const std::string& key = "PJC"){
    auto file = std::fstream (filePath);
    if(!file.is_open()) {
        noFilePermissionHandler();
        return 1;
    }
    auto eachChar = std::string();
    auto allVec = std::vector<std::string>();
    auto colorVec = std::vector<std::string>();
    auto lastBytes = std::vector<char>();
    auto charCapacity =0;
    auto dataStartIndex = 4;

    while(file >> eachChar){
        allVec.push_back(eachChar);
    }

    if(allVec[1]=="#"){
        auto i=1;
        while(!isdigit(allVec[i][0])){
            dataStartIndex++;
            i++;
        }
    }

    for (int i = dataStartIndex; i < allVec.size(); ++i) {
        charCapacity++;
        std::bitset<8> binaryForm(std::stoi(allVec[i]));
        colorVec.push_back(binaryForm.to_string());
    }

    message = txtToBin(message);

    fmt::print("message length ");
    fmt::print(fmt::emphasis::bold,"{}",message.size());
    fmt::print(", available space in current image: ");
    fmt::print(fmt::emphasis::bold,"{}",charCapacity);
    fmt::println("");

    for (auto bit : colorVec) {
        lastBytes.push_back(bit.back());
    }


    if(findMessage(lastBytes, txtToBin(key)) != ""){
        fmt::print(fg(fmt::color::light_green),"the image contains a hidden message\n");
    }
    else{
        fmt::print(fg(fmt::color::orange),"the image doesn't contain hidden message\n");
    }
    return 0;

}
auto checkBMPFile(const std::string& filePath,std::string message,const std::string& key = "PJC"){
    auto file = std::fstream (filePath,std::fstream::in | std::ios::binary);
    if(!file.is_open()) {
        noFilePermissionHandler();
        return 1;
    }

    file.seekg(0, std::ios::end);
    int fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    auto allVec = std::vector<unsigned char>(fileSize);
    file.read(reinterpret_cast<char*>(allVec.data()), fileSize);
    auto lastBytes = std::vector<char>();
    auto charCapacity =0;

    int dataStart = allVec[10] + (allVec[11]<<8) + (allVec[12]<<16) + (allVec[13]<<24);

    for (int i = dataStart; i < allVec.size(); ++i) {
        charCapacity++;
        std::bitset<8> binaryForm(allVec[i]);
        lastBytes.push_back(binaryForm.to_string().front());
    }

    message = txtToBin(message);

    fmt::print("message length ");
    fmt::print(fmt::emphasis::bold,"{}",message.size());
    fmt::print(", available space in current image: ");
    fmt::print(fmt::emphasis::bold,"{}",charCapacity);
    fmt::println("");

    if(findMessage(lastBytes, txtToBin(key)) != ""){
        fmt::print(fg(fmt::color::light_green),"the image contains a hidden message\n");
    }
    else{
        fmt::print(fg(fmt::color::orange),"the image doesn't contain hidden message\n");
    }
    return 0;
}
auto helpInfo(){
    fmt::println("");
    fmt::print(fmt::emphasis::bold,"{:^112}","STEGANOGRAPHY APP\n");
    fmt::println("");
    fmt::println("-i/-info <path_to_file>");
    fmt::println("    -> Shows information about the format, image size, memory usage, and the timestamp of its last modification.\n");
    fmt::println("-e/-encrypt <path_to_file> <message_to_hide> *<key>");
    fmt::println("    -> Opens the image and writes the specified message into it.\n");
    fmt::println("-d/-decrypt <path_to_file> *<key>");
    fmt::println("    -> Opens the image and reads the hidden message from it.\n");
    fmt::println("-c/-check <path_to_file> <message_to_check> *<key>");
    fmt::println("    -> Checks if a message can be saved in the given file or if a hidden message could exist.\n");
    fmt::println("-h/-help");
    fmt::println("    -> Prints information about the program.\n");
    fmt::println("*OPTIONALLY encrypt/decrypt with a custom key. The default key is 'PJC'. The key MUST contain exactly 3 characters!\n");
    fmt::println("Supported formats: .PPM, .BMP\n");
    fmt::println("Example use:\n");
    fmt::println("steganography.exe -e <path_to_image> \"my secret password\" \"C04\"");
}
auto errorHandler(const std::string& message,const std::string& optionallyArg = ""){
    fmt::print(fg(fmt::color::red),"{} {}\n",message,optionallyArg);
    fmt::print(fmt::emphasis::bold | fmt::emphasis::italic,"type: \"steganography.exe -h\" for more information\n");
}



auto main(int argc, char* argv[]) -> int{
    auto aliases = std::map<std::string,std::string>{
            {"-i","-info"},
            {"-e","-encrypt"},
            {"-d","-decrypt"},
            {"-c","-check"},
            {"-h","-help"}
    };
    if(argc == 1){
        helpInfo();
    }
    for (int i = 1; i < argc; ++i) {

        auto arg = std::string(argv[i]);

        if(arg == "-h" || arg=="-help"){
            if(i+1 < argc || argc>2){
                errorHandler("too many arguments for",arg);
                return 1;
            }
            helpInfo();
        }

        else if(i+1 < argc) {

            auto filePath = std::string(argv[i+1]);

            if(arg == "-i" || arg=="-info"){
                if(argc > i+2){
                    errorHandler("too many arguments for:",arg);
                    return 1;
                }

                if(validateIfFilePathExists(filePath) && validateFileExtension(filePath))
                {
                    fmt::println("path: {}",filePath);
                    fmt::println("size: {} B",std::filesystem::file_size(filePath));
                    if (filePath.substr(filePath.size() - 3) == "ppm") {
                        infoForPPM(filePath);
                    }
                    else{
                        infoForBMP(filePath);
                    }

                    std::filesystem::file_time_type ftime = std::filesystem::last_write_time(
                            std::filesystem::path(filePath))+std::chrono::hours(2);

                    std::cout << std::format("last modified: {}\n", ftime);

                    return 0;
                }
                else{
                    errorHandler("wrong path/file extension");
                    return 1;
                }
            }
            else if(arg == "-e" || arg=="-encrypt"){
                if(argc > i+4){
                    errorHandler("too many arguments for:",arg);
                    return 1;
                }

                if(validateIfFilePathExists(filePath) && validateFileExtension(filePath))
                {
                    if(argv[i+2] == nullptr){
                        errorHandler("message missing!!!");
                        return 1;
                    }

                    if(filePath.substr(filePath.size()-3) == "ppm"){
                        if(argv[i+3] == nullptr){
                            encryptMessageInPPM(filePath, argv[i+2]);
                            return 0;
                        }
                        if(std::strlen(argv[i+3])!=3) {
                            errorHandler("key must be 3 characters long!!!");
                            return 1;
                        }
                        encryptMessageInPPM(filePath, argv[i+2],argv[i+3]);
                        return 0;
                    }
                    else{
                        if(argv[i+3] == nullptr){
                            encryptMessageInBMP(filePath, argv[i+2]);
                            return 0;
                        }
                        if(std::strlen(argv[i+3])!=3) {
                            errorHandler("key must be 3 characters long!!!");
                            return 1;
                        }
                        encryptMessageInBMP(filePath, argv[i+2],argv[i+3]);
                        return 0;
                    }
                }
                else{
                    errorHandler("wrong path/file extension");
                    return 1;
                }

            }
            else if(arg == "-d" || arg=="-decrypt"){
                if(argc > i+3){
                    errorHandler("too many arguments for:",arg);
                    return 1;
                }
                if(validateIfFilePathExists(filePath) && validateFileExtension(filePath))
                {
                    if(filePath.substr(filePath.size()-3) == "ppm"){

                        if(argv[i+2] == nullptr){
                            decryptMessageFromPPM(filePath);
                            return 0;
                        }
                        if(std::strlen(argv[i+2])!=3) {
                            errorHandler("key must be 3 characters long!!!");
                            return 1;
                        }
                        decryptMessageFromPPM(filePath, argv[i+2]);
                        return 0;
                    }
                    else{
                        if(argv[i+2] == nullptr){
                            decryptMessageFromBMP(filePath);
                            return 0;
                        }
                        if(std::strlen(argv[i+2])!=3) {
                            errorHandler("key must be 3 characters long!!!");
                            return 1;
                        }
                        decryptMessageFromBMP(filePath, argv[i+2]);
                        return 0;
                    }
                }
                else{
                    errorHandler("wrong path/file extension");
                    return 1;
                }
            }
            else if(arg == "-c" || arg=="-check"){
                if(argc > i+4){
                    errorHandler("too many arguments for:",arg);
                    return 1;
                }

                else if(argv[i+2]== nullptr){
                    errorHandler("message missing!!!");
                    return 1;
                }
                if(validateIfFilePathExists(filePath) && validateFileExtension(filePath))
                {
                    if(filePath.substr(filePath.size()-3) == "ppm"){
                        if(argv[i+3] == nullptr){
                            checkPPMFile(filePath,argv[i+2]);
                            return 0;
                        }
                        else if(std::strlen(argv[i+3])!=3){
                            errorHandler("key must be 3 characters long!!!");
                            return 1;
                        }
                        checkPPMFile(filePath, argv[i+2],argv[i+3]);
                        return 0;
                    }
                    else{
                        if(argv[i+3] == nullptr){
                            checkBMPFile(filePath,argv[i+2]);
                            return 0;
                        }
                        else if(std::strlen(argv[i+3])!=3){
                            errorHandler("key must be 3 characters long!!!");
                            return 1;
                        }
                        checkBMPFile(filePath, argv[i+2],argv[i+3]);
                        return 0;
                    }
                }
                else{
                    errorHandler("wrong path/file extension");
                    return 1;
                }
            }
            else
            {
                errorHandler("flag: "+arg,"doesn't exist!");
                return 1;
            }

        }
        else{
            if(i+1 == argc &&
            [aliases,argv,i]()->bool
            {
                for(auto alias:aliases)
                    if (argv[i] == alias.second || argv[i] == alias.first)
                        return true;

                return false;
            }())
            {
                errorHandler("missing argument for flag: ",arg);
                return 1;
            }

            errorHandler("flag: "+arg,"doesn't exist!");
            return 1;
        }
    }
    return 0;
}
