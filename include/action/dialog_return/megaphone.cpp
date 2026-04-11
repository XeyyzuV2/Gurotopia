#include "pch.hpp"

#include "commands/sb.hpp"

#include "megaphone.hpp"

void megaphone(ENetEvent& event, const ::hPipe &hPipe)
{
    sb(event, hPipe["message"]); // @todo handle this when /sb requires gems @todo handle trim
}