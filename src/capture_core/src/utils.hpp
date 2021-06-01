//
// Created by Han Zhang on 1/13/20.
//

#ifndef CAPTURE_CORE_UTILS_HPP
#define CAPTURE_CORE_UTILS_HPP

#include <string>

bool startsWith(const char *pre, const char *str);

void change_dir_owner(const std::string &file_path, const std::string &username);

/*
 * Check the linux system whether the given *port* already has another process listening
 * Return: true if the given port is free
 */
bool check_port_available(int port);

std::string construct_vlan_if_name(int vlan_id);

bool is_interface_online(char const *interface);

pid_t system2(char const * command, bool wait_p = true, int * infp = nullptr, int * outfp = nullptr);

/*
 * Template functions. Variadic functions
 */
template<typename ... Args>
std::string string_format(const std::string &format, Args ... args);

template<typename ... Args>
pid_t execute_cmd(const std::string &fmt_cmd, Args ... args);

//template<typename ... Args>
//pid_t execute_cmd_no_wait(const std::string &fmt_cmd, Args ... args);

#include "utils.tpp"

#endif //CAPTURE_CORE_UTILS_HPP
