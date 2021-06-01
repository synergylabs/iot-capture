//
// Created by Han Zhang on 2019-08-20.
//

#include <unistd.h>
#include <sys/socket.h>
#include <cstdlib>
#include <netinet/in.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <pthread.h>
#include <csignal>
#include <memory>
#include <libconfig.h++>
#include <iostream>

#include "monitor_main.hpp"
#include "crypto.hpp"
#include "config.h"
#include "utils.hpp"
#include "device_client.hpp"
#include "message.hpp"

using std::string;
using std::unique_ptr;
using std::move;

using libconfig::Config;
using libconfig::FileIOException;
using libconfig::ParseException;
using libconfig::SettingNotFoundException;

const char TAG[] = "MonitorMain";

const char *CAPTURE_RUNTIME_CONFIG_FILE = "/home/pi/capture_monitor.cfg";

bool ENABLE_NETWORK_ISOLATION = true;
bool ENABLE_PROCESS_ISOLATION = true;
bool SKIP_DRIVER_CREATION = false;
bool CREATE_DRIVER_WITH_NO_CREDENTIAL = false;

string DEFAULT_DRIVER = "/home/pi/capture_default_driver.py";
string DEFAULT_CAMERA_DRIVER = "/home/pi/capture_default_camera_driver.py";


/*
 * Workflow:
 * - Listening for LD discovery request
 * - Respond to LD discovery request
 * - Fork new thread to process target device request
 */
int main(int argc, char **argv) {
    pthread_t broadcast_thread_id, process_request_thread_id;
    int err;

    LOGV("Monitor Main");
    LOGV("Version: %s", PROJECT_VER);

    LOGV("Setting up quit signal handler...");
    struct sigaction sigIntHandler{};

    sigIntHandler.sa_handler = exit_signal_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, nullptr);

    load_config_file();

    // Initialize system
    monitor_system_init();


    err = start_eap_ap();
    if (err != 0) {
        LOGE("Error creating EAP access point");
    }

    err = monitor_server_create(&(server_data.server_fd));
    if (err != 0) {
        LOGE("Error creating server socket");
    }

    // Create discovery broadcast thread
    pthread_create(&broadcast_thread_id, nullptr, discovery_broadcast_handler, nullptr);

    // Create main listener thread to handle new requests
    pthread_create(&process_request_thread_id, nullptr, main_listener,
                   nullptr);

    // Wait for all threads
    pthread_join(broadcast_thread_id, nullptr);
    pthread_join(process_request_thread_id, nullptr);

    return 0;
}

int start_eap_ap() {
    execute_cmd("sudo systemctl restart hostapd");
    return 0;
}

int monitor_server_create(int *ptr_sock) {
    struct sockaddr_in address{};
    int opt = 1;

    // Creating socket file descriptor
    if ((*ptr_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        LOGE("socket failed");
    }

    // Set socket options
    if (setsockopt(*ptr_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt))) {
        LOGE("setsocktopt failed");
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Attach socket to the port
    if (bind(*ptr_sock, (struct sockaddr *) &address, sizeof(address)) < 0) {
        LOGE("bind failed");
    }
    if (listen(*ptr_sock, MAX_CONN) < 0) {
        LOGE("listen");
    }
    return 0;
}


void monitor_system_init() {
    int err = 0;

    err = generate_key_pair(MONITOR_PUBLIC_KEY, MONITOR_PRIVATE_KEY);
    LOGV("Creating RSA key pair returns: %d", err);

    // Initialize server data structure
    server_data.server_fd = 0;
    server_data.available_vlan_id = VLAN_LOWER;
    server_data.available_driver_port = DRIVER_PORT_INIT;
    server_data.active_drivers.clear();

    clean_up_radius_file();

    // Load backup files in dhcp configurations and dhcpd.conf
    execute_cmd("sudo cp %s.backup %s", DHCP_CONFIG_FILE, DHCP_CONFIG_FILE);
    execute_cmd("sudo cp %s.backup %s", DHCP_SERVICE_FILE, DHCP_SERVICE_FILE);

    // Deleting all old testing users in previous run
    execute_cmd("sudo %s", DELETE_HUB_USER_SCRIPT);

    // Flush iptables rulesets
    execute_cmd("sudo iptables --flush");

    if (ENABLE_NETWORK_ISOLATION) {
        execute_cmd("sudo iptables -D FORWARD DROP"); // disbale cross interface forward
        execute_cmd("sudo iptables -A FORWARD -i wlan2 -o eth0 -j ACCEPT");
        execute_cmd("sudo iptables -A FORWARD -i eth0 -o wlan2 -j ACCEPT");
        execute_cmd("sudo iptables -A FORWARD -i wlan2 -o wlan1 -j ACCEPT");
        execute_cmd("sudo iptables -A FORWARD -i wlan1 -o wlan2 -j ACCEPT");
        execute_cmd("sudo iptables -A FORWARD -i wlan2 -o lo -j ACCEPT");
        execute_cmd("sudo iptables -A FORWARD -i lo -o wlan2 -j ACCEPT");
    }

    // Load default TOMOYO policy
    if (ENABLE_PROCESS_ISOLATION) {
        execute_cmd("cat %s | sudo tomoyo-loadpolicy -e", DEFAULT_TOMOYO_GROUP);
    }
}

void clean_up_radius_file() {
    FILE *original, *temp;
    char *line = nullptr;
    size_t len = 0;
    ssize_t read;
    bool skip = false;
    bool inside_user = false;
    bool skip_user = false;
    string temp_filename = "/tmp/temp_radius_users";
    const char *DEFAULT_PREFIX = "DEFAULT";

    change_dir_owner(RADIUS_FOLDER, "pi");

    original = fopen(RADIUS_USERS_FILE, "r");
    temp = fopen(temp_filename.c_str(), "w");
    if (original == nullptr || temp == nullptr) {
        change_dir_owner(RADIUS_FOLDER, "freerad");
        LOGE("radius user files open error");
    }

    while ((read = getline(&line, &len, original)) != -1) {
        if (skip) {
            fputs(line, temp);
        } else {
            // If line is empty line, continue
            if (read == 0) continue;

            // If line starts with DEFAULT, skip and copy the rest of the file
            if (startsWith(DEFAULT_PREFIX, line)) {
                skip = true;
            }

            if (inside_user) {
                // break if not \t
                if (!startsWith("\t", line)) {
                    inside_user = false;
                    skip_user = false;
                }
            } else {
                if (strstr(line, "Cleartext-Password")) {
                    inside_user = true;
                    if (startsWith("testing", line)) {
                        skip_user = true;
                        continue;
                    }
                }
            }
            if (!(inside_user && skip_user)) {
                fputs(line, temp);
            }
        }
    }

    fclose(original);
    fclose(temp);

    // Copy temp file to original
    execute_cmd("sudo cp %s %s", temp_filename.c_str(), RADIUS_USERS_FILE);

    change_dir_owner(RADIUS_FOLDER, "freerad");

    execute_cmd("sudo systemctl restart freeradius");
}

void assign_ip_and_dhcp_to_interface(int vlan_id) {
    string vlan_if;
    int err = 0;
    char buf[MAX_BUFFER_SIZE] = {0};

    vlan_if = construct_vlan_if_name(vlan_id);

    // Update IP address for the VLAN interface
    execute_cmd("sudo ifconfig %s 192.168.%d.1/24", vlan_if.c_str(), vlan_id);

    // Update DHCP server (isc-dhcp-server)
    err = snprintf(buf, MAX_BUFFER_SIZE, "subnet 192.168.%d.0 netmask 255.255.255.0 {\\n"
                                         "\\trange 192.168.%d.100 192.168.%d.200;\\n"
                                         "\\toption routers 192.168.%d.1;\\n"
                                         "\\toption subnet-mask 255.255.255.0;\\n"
                                         "\\toption domain-search \"capture-pi.andrew.cmu.edu\";\\n"
                                         "\\toption domain-name-servers 8.8.8.8;\\n"
                                         "}\\n", vlan_id, vlan_id, vlan_id, vlan_id);
    if (err < 0) {
        LOGE("truncate dhcp entry");
    }
    execute_cmd("sudo sed -i '$s/$/\\n%s/' %s", buf, DHCP_CONFIG_FILE);

    // Add new wlan interface to the list of listening interfaces
    execute_cmd("sudo sed -i '/INTERFACES=.*/s/\"$/ %s\"/' %s", vlan_if.c_str(), DHCP_SERVICE_FILE);

    // restart DHCP server
    execute_cmd("sudo systemctl restart isc-dhcp-server");
}

void remove_brvlan(int vlan_id) {
    string vlan_if = construct_vlan_if_name(vlan_id);

    // Update IP address for the VLAN interface
    execute_cmd("sudo brctl delif brvlan%d %s", vlan_id, vlan_if.c_str());
    execute_cmd("sudo ifconfig brvlan%d down", vlan_id);
    execute_cmd("sudo brctl delbr brvlan%d", vlan_id);
}

int assign_new_vlan() {
    // FIXME: manage list of free / used VLANs
    if (server_data.available_vlan_id >= VLAN_UPPER) {
        LOGE("Running out of VLAN IDs");
    }

    return ++server_data.available_vlan_id;
}


/*
 * Broadcast discovery message in the network
 */
void *discovery_broadcast_handler(void *vargp) {
    // TODO: Create discovery broadcast server and receive incoming commands (if necessary)
    // TODO: Use TLS for server authentication
//    while (true) {
//        sleep(10);
//    }
    return nullptr;
}

void *main_listener(void *vargp) {
    int new_socket;
    struct sockaddr_in address{};
    int addrlen = sizeof(address);
    int server_fd = server_data.server_fd;

    LOGD("Server waiting for connection...");

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
            LOGE("accept");
        }
        // Fork into a separate thread to process new request
        new_request_handler_arg_t new_request_handler_arg = {
                new_socket
        };
        pthread_t pthread;
        pthread_create(&pthread, nullptr, new_request_handler,
                       (void *) &new_request_handler_arg);
    }
}

void *new_request_handler(void *vargp) {
    int vlan_id = 0, err = 0;
    auto *argp = (new_request_handler_arg_t *) vargp;
    int new_socket = argp->new_socket;

    if (CREATE_DRIVER_WITH_NO_CREDENTIAL) {
        // create VLAN ID (X)
        vlan_id = assign_new_vlan();

        // create Radius entry (user, pass, VLAN-ID = X)
        unique_ptr<device_client> cli(new device_client(vlan_id, new_socket, ""));

        // Allocate driver information to device.
        // We can't start driver process here, because the corresponding network interface has not been created
        cli->create_linux_user();
        // Allocate network ports to driver
        int driver_port = assign_new_driver_port();
        cli->set_driver_port(driver_port);

//        // monitor interface status. Add ip address 192.168.X.1/24 to wlan1.X if appeared
//        // add DHCP config to the VLAN subnet and wireless interface
//        // This has to be completed after we respond new credentials to target device
//        assign_ip_and_dhcp_to_interface(vlan_id);
//        remove_brvlan(vlan_id);

        return nullptr;
    }

    // receive Hello message --> change to wait for connection request message
    auto request_msg = new_empty_msg();
    err = read_eap_request_msg(request_msg, new_socket);
    if (err < 0) {
        return nullptr;
    }

    // create VLAN ID (X)
    vlan_id = assign_new_vlan();
    // create Radius entry (user, pass, VLAN-ID = X)
    unique_ptr<device_client> cli(new device_client(vlan_id, new_socket,
                                                    dynamic_cast<payload_eap_request_t *>(request_msg->payload)->driver_name));

    // Allocate driver information to device.
    // We can't start driver process here, because the corresponding network interface has not been created
    cli->create_linux_user();
    // Allocate network ports to driver
    int driver_port = assign_new_driver_port();
    cli->set_driver_port(driver_port);

    // respond credential information
    cli->respond_eap_credentials();

    // monitor interface status. Add ip address 192.168.X.1/24 to wlan1.X if appeared
    // add DHCP config to the VLAN subnet and wireless interface
    // This has to be completed after we respond new credentials to target device
    assign_ip_and_dhcp_to_interface(vlan_id);
    remove_brvlan(vlan_id);

    if (SKIP_DRIVER_CREATION) {
        return nullptr;
    }

    // Start driver process
    cli->start_driver(driver_port);

    cli->secure_network();

    // TODO: * - 3-party handshake for keys

    // TODO: Clean up to avoid memory leaks

    server_data.active_drivers.push_back(move(cli));

    return nullptr;
}

int assign_new_driver_port() {
    int port = 0;

    do {
        port = ++server_data.available_driver_port;

        if (port > DRIVER_PORT_UPPER) {
            return 0;
        }
    } while (!check_port_available(port));

    return port;
}

void exit_signal_handler(int s) {
    LOGV("Caught signal %d\nClean up all pending drivers...\n", s);

    // Clean up all connected client drivers
    for (auto &it : server_data.active_drivers) {
        if (it->get_driver_pid() > 0) {
            execute_cmd("sudo kill -9 %d", it->get_driver_pid());
        }
    }

    exit(1);
}

void load_config_file() {
    LOGV("Loading Capture monitor configuration file...");

    Config cfg;

    try {
        cfg.readFile(CAPTURE_RUNTIME_CONFIG_FILE);
    }
    catch (const FileIOException &fioex) {
        std::cerr << "I/O error while reading file." << std::endl;
        exit(EXIT_FAILURE);
    }
    catch (const ParseException &pex) {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
                  << " - " << pex.getError() << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        cfg.lookupValue("ENABLE_NETWORK_ISOLATION", ENABLE_NETWORK_ISOLATION);
        cfg.lookupValue("ENABLE_PROCESS_ISOLATION", ENABLE_PROCESS_ISOLATION);
        cfg.lookupValue("SKIP_DRIVER_CREATION", SKIP_DRIVER_CREATION);
        cfg.lookupValue("CREATE_DRIVER_WITH_NO_CREDENTIAL", CREATE_DRIVER_WITH_NO_CREDENTIAL);

        cfg.lookupValue("DEFAULT_DRIVER", DEFAULT_DRIVER);
        cfg.lookupValue("DEFAULT_CAMERA_DRIVER", DEFAULT_CAMERA_DRIVER);
    }
    catch (const SettingNotFoundException &nfex) {}


}
