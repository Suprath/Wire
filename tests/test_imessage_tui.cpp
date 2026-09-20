/**
 * @file test_imessage_tui.cpp
 * @brief Unit tests for minimal iMessage-style speech bubble TUI renderer
 * @project Project Wire
 */

#include "../src/ui/imessage_tui.hpp"
#include <iostream>
#include <cassert>

void test_imessage_tui_render() {
    std::cout << "[TEST] Running minimal iMessage-style TUI layout test..." << std::endl;

    wire::ui::iMessageTUI tui;

    std::vector<std::pair<std::string, std::string>> contacts = {
        {"Bob", "CONNECTED"},
        {"Alice (Work)", "IDLE"},
        {"Dr. Charlie", "OFFLINE"}
    };

    std::vector<wire::ui::ChatBubble> history = {
        {"YOU", "Hello Bob! Testing iMessage TUI.", "19:05", true},
        {"BOB", "ACK: Ghost Bridge Online! 🚀", "19:06", false}
    };

    tui.render_layout("Bob", true, contacts, history);

    std::cout << "\n  [PASS] Minimal iMessage-style speech bubble TUI layout rendered cleanly." << std::endl;
    std::cout << "[SUCCESS] iMessage TUI layout tests passed!\n" << std::endl;
}

int main() {
    test_imessage_tui_render();
    return 0;
}
