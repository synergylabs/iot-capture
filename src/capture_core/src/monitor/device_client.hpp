//
// Created by Han Zhang on 1/13/20.
//

#ifndef CAPTURE_CORE_DEVICE_CLIENT_HPP
#define CAPTURE_CORE_DEVICE_CLIENT_HPP

#include <credential.h>
#include <string>

class device_client {
private:
    int vlan_id = 0;
    int client_sock;
    int driver_port = 0;
    std::string driver_path;
    credential_t creds;
    pid_t driver_pid = 0;

    int generate_eap_credential();

public:
    device_client(int vlan_id, int client_sock);
    device_client(int vlan_id, int client_sock, int driver_port);
    device_client(int vlan_id, int client_sock, const std::string& driver_path);
    device_client(int vlan_id, int client_sock, int driver_port, const std::string& driver_path_in);
    ~device_client();

    int respond_eap_credentials();
    int create_linux_user();
    int start_driver(int port = 0);

    int set_driver_port(int port);
    pid_t  get_driver_pid();

    void set_driver_path(const std::string&  driver_path_in);
    std::string get_driver_path();

    void secure_network();
    void secure_process();

};


#endif //CAPTURE_CORE_DEVICE_CLIENT_HPP
