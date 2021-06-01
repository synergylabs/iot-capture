//
// Created by Han Zhang on 1/13/20.
//

#include <string>
#include <cstdio>
#include <algorithm>  // std::fill
#include <netinet/in.h>
#include <fstream>
#include <unistd.h>  // read()
#include <cstring>

#include "crypto.hpp"
#include "config.h"
#include "logger.hpp"
#include "utils.hpp"
#include "device_client.hpp"
#include "message.hpp"

using std::unique_ptr;
using std::string;
using std::fstream;
using std::ios;

const char TAG[] = "MonitorDeviceClient";

device_client::device_client(int vlan_id, int client_sock)
        : device_client(vlan_id, client_sock, 0, "") {}

device_client::device_client(int vlan_id, int client_sock, const string& driver_path)
        : device_client(vlan_id, client_sock, 0, driver_path) {}


device_client::device_client(int vlan_id, int client_sock, int driver_port)
        : device_client(vlan_id, client_sock, driver_port, "") {}

device_client::device_client(int vlan_id, int client_sock, int driver_port, const string& driver_path_in)
        : vlan_id(vlan_id), client_sock(client_sock), driver_port(driver_port),
          creds(credential_t{}), driver_pid(0) {
    generate_eap_credential();

    set_driver_path(driver_path_in);
}

device_client::~device_client() {
    // free credentials
}


int device_client::generate_eap_credential() {
    char buf[MAX_BUFFER_SIZE] = {0};
    int err = 0;

    snprintf(buf, MAX_BUFFER_SIZE, "testing%d", vlan_id);

    creds.username = buf;
    creds.password = gen_random(PASSWORD_LEN);

    // Add entry to Radius
    std::fill(buf, buf + MAX_BUFFER_SIZE, 0);
    err = snprintf(buf, MAX_BUFFER_SIZE, "%s Cleartext-Password := \"%s\" \\n"
                                         "\\tTunnel-Type = \"VLAN\", \\n"
                                         "\\tTunnel-Medium-Type = \"IEEE-802\", \\n"
                                         "\\tTunnel-Private-Group-ID = \"%d\", \\n"
                                         "\\tReply-Message = \"Hello, %%{User-Name}\" , \\n"
                                         "\\tFall-Through = Yes \\n",
                   creds.username.c_str(), creds.password.c_str(), vlan_id);
    if (err < 0) {
        LOGE("radius user entry truncated");
    }

    // insert entry to radius config file
    execute_cmd("sudo sed -i '1i%s' %s", buf, RADIUS_USERS_FILE);

    // restart freeradius server
    execute_cmd("sudo systemctl restart freeradius");
    return 0;
}

int device_client::respond_eap_credentials() {
    auto msg = new_empty_msg();
    new_eap_creds_msg(msg, static_cast<unsigned int>(driver_port),
                      creds.username.c_str(), creds.password.c_str());

    uint32_t conv = htonl(msg->msg_type);
    send(client_sock, &conv, sizeof(conv), 0);

    auto payload_eap = dynamic_cast<payload_eap_creds_t*>(msg->payload);
    if (payload_eap == nullptr) {
        LOGE("Error generating EAP response payload!")
    }

    conv = htonl(payload_eap->driver_port);
    send(client_sock, &conv, sizeof(conv), 0);

    conv = htonl(payload_eap->username_len);
    send(client_sock, &conv, sizeof(conv), 0);
    conv = htonl(payload_eap->password_len);
    send(client_sock, &conv, sizeof(conv), 0);

    send(client_sock, payload_eap->username, payload_eap->username_len, 0);
    send(client_sock, payload_eap->password, payload_eap->password_len, 0);

    return 0;
}

int device_client::create_linux_user() {
    // pi@raspberrypi:~ $ sudo adduser --disabled-password --gecos "" testing101
    execute_cmd("sudo adduser --disabled-password --gecos \"\" %s", creds.username.c_str());
    return 0;
}

int device_client::start_driver(int port) {
    if (this->driver_port == 0 && port == 0) {
        LOGE("No driver port set!");
    }

    if (port != 0 && this->driver_port != port) {
        set_driver_port(port);
    }

    pid_t child_pid = 0;

    secure_process();

    child_pid = execute_cmd("sudo /bin/su -s /bin/bash -c '%s %d %d 2>&1 > /dev/null' %s &",
                            get_driver_path().c_str(), driver_port, vlan_id,
                            creds.username.c_str());


    if (child_pid < 0) {
        LOGE("Errors on creating new device drivers");
    }

    this->driver_pid = 0;
    get_driver_pid();
    LOGD("new driver PID: %d", this->driver_pid);

    return 0;
}

int device_client::set_driver_port(int port) {
    LOGV("Updating driver process's allocated port");

    this->driver_port = port;

    return 0;
}

void readFile(int fd, char *buffer, size_t max_len) {
    int bytes_read;
    unsigned k = 0;
    do {
        char t = 0;
        bytes_read = read(fd, &t, 1);
        buffer[k++] = t;
    } while (bytes_read != 0 && k + 1 < max_len);

    LOGD("Final result for lsof: %s", buffer);
    LOGD("number is: %d", atoi(buffer));
}

pid_t device_client::get_driver_pid() {
    while (this->driver_pid == 0) {
        int out_fd = 0;

        system2(string_format("sudo lsof -ti :%d", this->driver_port).c_str(),
                true,
                nullptr,
                &out_fd);

        char buffer[MAX_BUFFER_SIZE] = {0};
        readFile(out_fd, buffer, MAX_BUFFER_SIZE);
        this->driver_pid = atoi(buffer);
    }
    return this->driver_pid;
}

void device_client::secure_network() {
    if (!ENABLE_NETWORK_ISOLATION) return;

    // Enforce network isolation and process isolation working properly
    // iptables network isolation
    // Firewall setup: allow communication between LD and DP in firewall
    execute_cmd("sudo iptables -A FORWARD -i wlan1.%d -o wlan1.%d -j ACCEPT", vlan_id, vlan_id);

}

void device_client::secure_process() {
    if (!ENABLE_PROCESS_ISOLATION) return;

    // TOMOYO linux process isolation
    execute_cmd("echo 'initialize_domain %s from any' | sudo tomoyo-loadpolicy -e", get_driver_path().c_str());
    execute_cmd("echo 'keep_domain any from <kernel> %s' | sudo tomoyo-loadpolicy -e", get_driver_path().c_str());

    execute_cmd("( echo '<kernel> %s'; echo 'use_group %d'; ) | sudo tomoyo-loadpolicy -d",
            get_driver_path().c_str(), DEFAULT_TOMOYO_GROUP_NUMBER);

    // enable socket to driver
    execute_cmd("( echo '<kernel> %s'; "
                "echo 'network inet stream bind 192.168.%d.1 %d'; "
                "echo 'network inet stream listen 192.168.%d.1 %d'; "
                "echo 'network inet stream listen 192.168.%d.1 %d';) | sudo tomoyo-loadpolicy -d",
                get_driver_path().c_str(),
                vlan_id, driver_port,
                vlan_id, driver_port,
                vlan_id, driver_port);}

string device_client::get_driver_path() {
    string s;
    if (driver_path.empty()) {
        s = DEFAULT_DRIVER;
    } else {
        s = driver_path;
    }
    return s;
}

void device_client::set_driver_path(const string&  driver_path_in) {
    if (driver_path_in == payload_eap_request_t::USE_DEFAULT_DRIVER) {
        // TODO: Fork a copy of default driver for this specific device
        driver_path = DEFAULT_DRIVER;
    } else if (driver_path_in == payload_eap_request_t::USE_DEFAULT_CAMERA_DRIVER) {
        // TODO: Same as above
        driver_path = DEFAULT_CAMERA_DRIVER;
    } else {
        // use custom driver path
        driver_path = driver_path_in;
    }
}
