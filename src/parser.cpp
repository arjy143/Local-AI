#include "parser.hpp"
#include <nlohmann/json.hpp>

namespace local_ai
{
    std::optional<ToolCall> parse_tool_call(const std::string& response) noexcept
    {
        //parse response for json object
        size_t start = response.find('{');
        if (start == std::string::npos)
        {
            //didnt find json
            return std::nullopt;
        }

        size_t depth = 0;
        size_t end = start;

        for (size_t i = start; i < response.size(); i++)
        {
            if (response[i] == '{')
            {
                depth++;
            }
            else if (response[i] == '}')
            {
                depth--;
            }

            if (depth == 0)
            {
                end = i-1;
                break;
            }
        }

        if (depth != 0)
        {
            return std::nullopt;
        }

        std::string json_string = response.substr(start, end-start);

        //try and parse the json string
        try
        {
            auto json = nlohmann::json::parse(json_string);
            if (!json.contains("tool") || !json["tool"].is_string())
            {
                return std::nullopt;
            }
            ToolCall call;
            call.tool = json["tool"].get<std::string>();

            //only accept shell tools
            if (call.tool != "shell")
            {
                return std::nullopt;
            }

            if (json.contains("args") && json["args"].is_object())
            {
                for(auto& [key, value] : json["args"].items())
                {
                    if (value.is_string())
                    {
                        call.args[key] = value.get<std::string>();
                    }
                }
            }

            return call;
        }
        catch (const nlohmann::json::exception&)
        {
            return std::nullopt;
        }
    }
}
