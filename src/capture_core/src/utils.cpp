//
// Created by Han Zhang on 1/13/20.
//
#include <cstring>
#include <cstdarg>     /* va_list, va_start, va_arg, va_end */
#include <netinet/in.h>
#include <netdb.h>
#include <zconf.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <stdexcept>
#include <fcntl.h>
#include <sys/wait.h>

#include "utils.hpp"
#include "config.h"

using std::string;

const char TAG[] = "Utils";

bool startsWith(const char *pre, const char *str) {
    size_t lenpre = strlen(pre),
            lenstr = strlen(str);
    return lenstr < lenpre ? false : memcmp(pre, str, lenpre) == 0;
}

void change_dir_owner(const string &file_path, const string &username) {
    execute_cmd("sudo chown -R %s %s", username.c_str(), file_path.c_str());
}

bool check_port_available(int portno) {
    char const *hostname = "localhost";

    int sockfd;
    struct sockaddr_in serv_addr{};
    struct hostent *server;

    bool port_availiable = false;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
    }

    server = gethostbyname(hostname);

    if (server == nullptr) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr,
          (char *) &serv_addr.sin_addr.s_addr,
          static_cast<size_t>(server->h_length));

    serv_addr.sin_port = htons(portno);
    port_availiable = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0;

    close(sockfd);
    return port_availiable;
}

string construct_vlan_if_name(int vlan_id) {
    // Construct ap/vlan interface name
    string vlan_if = string_format("%s.%d", AP_VLAN_INTERFACE, vlan_id);

    while (!is_interface_online(vlan_if.c_str())) {
        // busy wait for interface to be available
    }

    return vlan_if;
}

bool is_interface_online(char const *interface) {
    struct ifreq ifr{};
    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, interface);
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {} // Error: No such device. Ignore for now.
    close(sock);
    return (ifr.ifr_flags & IFF_UP) != 0; // original code: return !!(ifr.ifr_flags & IFF_UP);
}

pid_t system2(char const *command, bool wait_p, int *infp, int *outfp) {
    int p_stdin[2];
    int p_stdout[2];
    pid_t pid;

    if (pipe(p_stdin) == -1)
        return -1;

    if (pipe(p_stdout) == -1) {
        close(p_stdin[0]);
        close(p_stdin[1]);
        return -1;
    }

    pid = fork();

    if (pid < 0) {
        close(p_stdin[0]);
        close(p_stdin[1]);
        close(p_stdout[0]);
        close(p_stdout[1]);
        return pid;
    } else if (pid == 0) {
        // child process
        close(p_stdin[1]);
        dup2(p_stdin[0], 0);
        close(p_stdout[0]);
        dup2(p_stdout[1], 1);
        dup2(::open("/dev/null", O_RDONLY), 2);
        /// Close all other descriptors for the safety sake.
        for (int i = 3; i < 4096; ++i)
            ::close(i);

        setsid();
        execl("/bin/sh", "sh", "-c", command, nullptr);
        _exit(1);
    } else {
        // parent
        if (wait_p) {
            int status;
            wait(&status);
        }
    }

    close(p_stdin[0]);
    close(p_stdout[1]);

    if (infp == nullptr) {
        close(p_stdin[1]);
    } else {
        *infp = p_stdin[1];
    }

    if (outfp == nullptr) {
        close(p_stdout[0]);
    } else {
        *outfp = p_stdout[0];
    }

    return pid;
}