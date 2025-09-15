#pragma once
#ifndef DOOR_PASSWORD_PROMPT_HPP
#define DOOR_PASSWORD_PROMPT_HPP

#include <vector>
#include <string>

#include "enet/enet.h"

void door_password_prompt(ENetEvent& event, const std::vector<std::string> &&pipes);

#endif
