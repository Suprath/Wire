/**
 * @file update_manager.hpp
 * @brief Zero-Loss App Update & Data Vault Migration Engine for Project Wire
 * @project Project Wire
 * 
 * @details
 * Manages cross-platform persistent data vault paths, atomic vault backups,
 * zero-loss data migration across release patches, and GitHub Release update checks.
 * 
 * Architecture & State Isolation Rationale:
 * 1. Application binaries (QEMU micro-VM images, TUI executables) are decoupled from user state.
 * 2. All user data (Master Seeds, Double Ratchet states, Merkle-DAG ledgers, contacts) is stored
 *    in user-space data directories (~/.wire/data/ or %APPDATA%\Wire\data\).
 * 3. Updating app binaries from GitHub Releases overwrites executable files ONLY, preserving 100%
 *    of user chat data and cryptographic keys without replication or data loss.
 * 4. Automatic in-place schema migration upgrades vault structures atomically when new patches land.
 */

#ifndef WIRE_UPDATE_MANAGER_HPP
#define WIRE_UPDATE_MANAGER_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <optional>

namespace wire::update {

static constexpr uint32_t CURRENT_VAULT_SCHEMA_VERSION = 1;
static constexpr const char* CURRENT_APP_VERSION = "v1.0.0";

/**
 * @struct UpdateCheckResult
 * @brief Result structure returned by GitHub release version check.
 */
struct UpdateCheckResult {
    bool update_available;         /**< true if newer version posted on GitHub Releases */
    std::string latest_version;    /**< Version tag string (e.g. "v1.1.0") */
    std::string release_url;       /**< Direct GitHub release download URL */
};

/**
 * @class UpdateManager
 * @brief Handles zero-loss data vault persistence, schema migrations, and update notifications.
 */
class UpdateManager {
public:
    /**
     * @brief Resolves the cross-platform user data directory path (~/.wire/data/ or Portable USB path).
     * @return std::string Absolute path to persistent user vault directory.
     */
    [[nodiscard]] static std::string get_user_data_directory() noexcept;

    /**
     * @brief Enables Portable USB / Live OS mode, directing data storage to the USB drive path.
     * @param enable true to enable portable mode.
     * @param custom_path Optional custom USB directory path (e.g. "/media/usb/wire_data" or "./wire_data").
     */
    static void set_portable_mode(bool enable, const std::string& custom_path = "") noexcept;

    /**
     * @brief Checks if Portable USB mode is active.
     * @return bool true if in Portable USB mode.
     */
    [[nodiscard]] static bool is_portable_mode() noexcept;

    /**
     * @brief Checks if the user data vault requires schema migration to match current app version.
     * @param vault_dir Path to user data directory.
     * @return bool true if data migration was applied or vault is current, false if migration error.
     */
    [[nodiscard]] static bool migrate_user_vault_if_needed(const std::string& vault_dir) noexcept;

    /**
     * @brief Creates an atomic snapshot backup of user chat data before applying patch migrations.
     * @param vault_dir User data directory.
     * @return std::optional<std::string> Backup filepath if successful, nullopt if failed.
     */
    [[nodiscard]] static std::optional<std::string> create_atomic_backup(const std::string& vault_dir) noexcept;

    /**
     * @brief Checks GitHub Releases API for new application update patches.
     * @param github_repo GitHub repository path (default "Suprath/Wire").
     * @return UpdateCheckResult Result containing update status and download link.
     */
    [[nodiscard]] static UpdateCheckResult check_github_updates(const std::string& github_repo = "Suprath/Wire") noexcept;
};

} // namespace wire::update

#endif // WIRE_UPDATE_MANAGER_HPP
