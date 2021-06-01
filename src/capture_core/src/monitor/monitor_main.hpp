//
// Created by Han Zhang on 2019-08-20.
//

#ifndef CAPTURE_CORE_MONITOR_MAIN_HPP
#define CAPTURE_CORE_MONITOR_MAIN_HPP

#include <memory>
#include <vector>

#include "credential.h"
#include "device_client.hpp"

struct server_data_t {
    int server_fd{};
    int available_vlan_id{};
    int available_driver_port{};
    std::vector<std::unique_ptr<device_client>> active_drivers;
};

struct new_request_handler_arg_t {
    int new_socket;
};

/*
 * Monitor server initial setup. Should only run one-time.
 * Including:
 *  - create key pairs and config files
 *  - reset RADIUS configuration
 *  - reset DHCP configuration
 *  - delete old users in testing mode
 */
void monitor_system_init();

/*
 * Load global configuration files
 */
void load_config_file();

/*
 * Restart hostapd AP
 */
int start_eap_ap();

/*
 * Load Radius user file and remove test users (previously created)
 * FIXME: Not super robust. Not reliable to handle spaces and new users.
 */
void clean_up_radius_file();

/*
 * Create new monitor server and update server fd in ptr_sock.
 * Return 0 if success.
 */
int monitor_server_create(int *ptr_sock);

/*
 * Workflow for handling new request:
 * - receive connection request
 * - create VLAN ID, Radius entry for device
 * - monitor new connection status in hostapd
 *   - once connected, add bridge information to firewall
 * - create driver process (DP) for device
 * - allow communication between LD and DP in firewall
 * - respond DP information to LD
 * - 3-party handshake for keys
 */
void *main_listener(void *vargp);

void *discovery_broadcast_handler(void *vargp);

void *new_request_handler(void *vargp);

int assign_new_vlan();

int assign_new_driver_port();

void assign_ip_and_dhcp_to_interface(int vlan_id);

void remove_brvlan(int vlan_id);

void exit_signal_handler(int s);

// Create global data for server information
server_data_t server_data;

#endif //CAPTURE_CORE_MONITOR_MAIN_HPP
