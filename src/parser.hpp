#pragma once
#include <optional>
#include <string>
#include <map>

namespace local_ai
{
    struct ToolCall
    {
        std::string tool;
        std::map<std::string, std::string> args;
    };

    //each tool call is like a json object like {name : tool, args...}

    std::optional<ToolCall> parse_tool_call(const std::string& response) noexcept;

}
