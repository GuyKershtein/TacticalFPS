#include "GameApplication.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace {

// Splits "host:port" into its parts; port is left at its current value if
// no ":port" suffix is present.
void ParseAddressPort(const std::string& text, std::string& outHost, uint16_t& outPort) {
    const size_t colon = text.find(':');
    if (colon == std::string::npos) {
        outHost = text;
        return;
    }
    outHost = text.substr(0, colon);
    outPort = static_cast<uint16_t>(std::atoi(text.c_str() + colon + 1));
}

} // namespace

int main(int argc, char** argv) {
    // Command-line role selection (Milestone 10):
    //   (no args)                 -> Offline, exactly every prior milestone's behavior
    //   --host[=port]             -> authoritative host, listening for remote players
    //   --connect=host[:port]     -> networked client
    // No menu-driven equivalent exists yet — picking a role happens before
    // a window is even open, so there is nothing yet to click through.
    Game::LaunchOptions options;
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (std::strncmp(arg, "--host", 6) == 0) {
            options.role = Game::NetRole::Host;
            if (arg[6] == '=') {
                options.port = static_cast<uint16_t>(std::atoi(arg + 7));
            }
        } else if (std::strncmp(arg, "--connect=", 10) == 0) {
            options.role = Game::NetRole::Client;
            ParseAddressPort(arg + 10, options.connectAddress, options.port);
        }
    }

    Game::GameApplication app;
    if (!app.Init(options)) {
        return -1;
    }
    app.Run();
    app.Shutdown();
    return 0;
}
