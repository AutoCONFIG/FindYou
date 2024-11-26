#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <cstring>
#include <netdb.h>
#include <arpa/inet.h>
#include <maxminddb.h>

void resolve_and_lookup(const std::string& domain, MMDB_s* mmdb) {
    struct addrinfo hints{};
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;  // 支持 IPv4 和 IPv6
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = nullptr;
    int status = getaddrinfo(domain.c_str(), nullptr, &hints, &res);
    if (status != 0) {
        std::cerr << "获取域名 " << domain << " 的IP地址时出错: " << gai_strerror(status) << std::endl;
        return;
    }

    // 使用智能指针确保资源自动释放
    std::unique_ptr<struct addrinfo, decltype(&freeaddrinfo)> res_ptr(res, freeaddrinfo);

    int mmdb_status;
    MMDB_lookup_result_s result;

    for (struct addrinfo* p = res; p != nullptr; p = p->ai_next) {
        // 调用 MMDB_lookup_sockaddr，传入 p->ai_addr
        result = MMDB_lookup_sockaddr(mmdb, p->ai_addr, &mmdb_status);
        if (mmdb_status != MMDB_SUCCESS) {
            std::cerr << "无法查询域名 " << domain << " 的地理位置信息: " << MMDB_strerror(mmdb_status) << std::endl;
            continue;
        }

        if (result.found_entry) {
            MMDB_entry_data_s entry_data;
            status = MMDB_get_value(&result.entry, &entry_data, "country", "names", "en", nullptr);
            if (status == MMDB_SUCCESS && entry_data.has_data &&
                entry_data.type == MMDB_DATA_TYPE_UTF8_STRING &&
                entry_data.utf8_string != nullptr && entry_data.data_size > 0) {

                char ip_str[INET6_ADDRSTRLEN];
                void* addr = nullptr;
                int family = p->ai_family;

                if (family == AF_INET) {
                    auto* ipv4 = reinterpret_cast<struct sockaddr_in*>(p->ai_addr);
                    addr = &(ipv4->sin_addr);
                } else if (family == AF_INET6) {
                    auto* ipv6 = reinterpret_cast<struct sockaddr_in6*>(p->ai_addr);
                    addr = &(ipv6->sin6_addr);
                } else {
                    continue;
                }

                inet_ntop(family, addr, ip_str, sizeof(ip_str));
                std::cout << "域名 " << domain << " 对应的IP地址 " << ip_str << " 所属国家: "
                          << std::string(entry_data.utf8_string, entry_data.data_size) << std::endl;
                break;
            }
        }
    }
}

int main() {
    const char* mmdb_path = "Country.mmdb";
    MMDB_s mmdb;
    int status = MMDB_open(mmdb_path, MMDB_MODE_MMAP, &mmdb);
    if (status != MMDB_SUCCESS) {
        std::cerr << "无法打开GeoIP数据库 " << mmdb_path << ": " << MMDB_strerror(status) << std::endl;
        return 1;
    }

    // 使用智能指针确保资源自动释放
    std::unique_ptr<MMDB_s, decltype(&MMDB_close)> mmdb_ptr(&mmdb, MMDB_close);

    std::ifstream infile("domains.txt");
    if (!infile.is_open()) {
        std::cerr << "无法打开文件 domains.txt" << std::endl;
        return 1;
    }

    std::string line;
    while (std::getline(infile, line)) {
        // 移除字符串首尾的空白字符和换行符
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            continue;  // 跳过空行
        }
        size_t end = line.find_last_not_of(" \t\r\n");
        std::string domain = line.substr(start, end - start + 1);

        if (!domain.empty()) {
            resolve_and_lookup(domain, &mmdb);
        }
    }

    // mmdb 会由 mmdb_ptr 自动关闭
    return 0;
}