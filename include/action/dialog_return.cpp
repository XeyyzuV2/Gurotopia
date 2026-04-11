#include "pch.hpp"

#include "dialog_return/__dialog_return.hpp"

#include "action/dialog_return.hpp"

void action::dialog_return(ENetEvent& event, const std::string& header) 
{
    ::hPipe hPipe{ header };

    // @note found at ./action/dialog_return/
    if (const auto it = dialog_return_pool.find(hPipe["dialog_name"]); it != dialog_return_pool.end()) 
        it->second(std::ref(event), std::move(hPipe));
}