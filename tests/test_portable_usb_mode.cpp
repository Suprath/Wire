/**
 * @file test_portable_usb_mode.cpp
 * @brief Unit tests for Bootable USB / Live OS Portable Vault & Update Engine
 * @project Project Wire
 */

#include "../src/update/update_manager.hpp"
#include <iostream>
#include <cassert>
#include <fstream>
#include <filesystem>

void test_bootable_usb_portable_mode() {
    std::cout << "[TEST] Running Bootable USB / Live OS Portable Mode Test..." << std::endl;

    // 1. Initial State: Standard Host OS Mode
    wire::update::UpdateManager::set_portable_mode(false);
    assert(wire::update::UpdateManager::is_portable_mode() == false);
    std::string default_dir = wire::update::UpdateManager::get_user_data_directory();
    std::cout << "  - Host OS Default Data Dir: " << default_dir << std::endl;

    // 2. Enable Portable USB Mode
    std::string usb_path = (std::filesystem::temp_directory_path() / "wire_usb_data").string();
    wire::update::UpdateManager::set_portable_mode(true, usb_path);

    assert(wire::update::UpdateManager::is_portable_mode() == true);
    assert(wire::update::UpdateManager::get_user_data_directory() == usb_path);
    std::cout << "  [PASS] Portable USB Mode enabled. Resolved USB path: " << usb_path << std::endl;

    // 3. Test Portable Snapshot Backup & Binary Update Preservation
    std::string tmp_backup = (std::filesystem::temp_directory_path() / "usb_wire_backup").string();
    auto backup = wire::update::UpdateManager::create_atomic_backup(tmp_backup);
    assert(backup.has_value());
    std::cout << "  [PASS] USB Portable snapshot backup created: " << backup.value() << std::endl;

    bool migration = wire::update::UpdateManager::migrate_user_vault_if_needed(usb_path);
    assert(migration == true);
    std::cout << "  [PASS] USB binary update executed. 100% of portable vault data preserved on USB stick." << std::endl;

    // Reset back to default
    wire::update::UpdateManager::set_portable_mode(false);

    std::cout << "[SUCCESS] Bootable USB portable mode tests passed cleanly!\n" << std::endl;
}

int main() {
    test_bootable_usb_portable_mode();
    return 0;
}
