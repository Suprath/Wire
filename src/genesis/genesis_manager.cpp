/**
 * @file genesis_manager.cpp
 * @brief Implementation of Genesis Provisioning Manager
 * @project Project Wire
 */

#include "genesis_manager.hpp"
#include <cstring>
#include <sstream>
#include <algorithm>

namespace wire::genesis {

static const char BASE64_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string GenesisManager::base64_encode(const uint8_t* data, size_t len) noexcept {
    std::string ret;
    ret.reserve(((len + 2) / 3) * 4);

    uint32_t val = 0;
    int valb = -6;
    for (size_t i = 0; i < len; ++i) {
        val = (val << 8) + data[i];
        valb += 8;
        while (valb >= 0) {
            ret.push_back(BASE64_CHARS[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        ret.push_back(BASE64_CHARS[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (ret.size() % 4 != 0) {
        ret.push_back('=');
    }
    return ret;
}

std::vector<uint8_t> GenesisManager::base64_decode(const std::string& input) noexcept {
    std::vector<uint8_t> out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; ++i) T[static_cast<size_t>(BASE64_CHARS[i])] = i;

    uint32_t val = 0;
    int valb = -8;
    for (char c : input) {
        if (c == '=') break;
        if (T[static_cast<unsigned char>(c)] == -1) continue;
        val = (val << 6) + static_cast<uint32_t>(T[static_cast<unsigned char>(c)]);
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

std::vector<uint8_t> GenesisPayload::serialize_binary() const noexcept {
    std::vector<uint8_t> buf(80);
    std::copy(master_seed.begin(), master_seed.end(), buf.begin());
    std::copy(peer_identity_pubkey.begin(), peer_identity_pubkey.end(), buf.begin() + 32);

    for (size_t i = 0; i < 8; ++i) {
        buf[64 + i] = static_cast<uint8_t>((t0_timestamp >> (i * 8)) & 0xFF);
        buf[72 + i] = static_cast<uint8_t>((primary_ipv6_prefix >> (i * 8)) & 0xFF);
    }
    return buf;
}

std::optional<GenesisPayload> GenesisPayload::deserialize_binary(const uint8_t* data, size_t len) noexcept {
    if (len < 80) return std::nullopt;

    GenesisPayload payload{};
    std::copy(data, data + 32, payload.master_seed.begin());
    std::copy(data + 32, data + 64, payload.peer_identity_pubkey.begin());

    payload.t0_timestamp = 0;
    payload.primary_ipv6_prefix = 0;

    for (size_t i = 0; i < 8; ++i) {
        payload.t0_timestamp |= (static_cast<uint64_t>(data[64 + i]) << (i * 8));
        payload.primary_ipv6_prefix |= (static_cast<uint64_t>(data[72 + i]) << (i * 8));
    }
    return payload;
}

GenesisPayload GenesisManager::create_genesis(
    const Key256& master_seed,
    const Key256& peer_pubkey,
    uint64_t t0_timestamp,
    uint64_t ipv6_prefix) noexcept {

    GenesisPayload p{};
    p.master_seed = master_seed;
    p.peer_identity_pubkey = peer_pubkey;
    p.t0_timestamp = t0_timestamp;
    p.primary_ipv6_prefix = ipv6_prefix;
    return p;
}

std::string GenesisManager::export_armored_key_file(const GenesisPayload& payload) noexcept {
    auto bin = payload.serialize_binary();
    std::string b64 = base64_encode(bin.data(), bin.size());

    std::ostringstream ss;
    ss << "-----BEGIN PROJECT WIRE GENESIS KEY-----\n";
    ss << "Version: Project Wire 1.0\n";
    ss << "Comment: Absolute Privacy Physical Entanglement Key\n\n";

    // Split base64 into 64-character lines
    for (size_t i = 0; i < b64.size(); i += 64) {
        ss << b64.substr(i, 64) << "\n";
    }

    ss << "-----END PROJECT WIRE GENESIS KEY-----\n";
    return ss.str();
}

std::optional<GenesisPayload> GenesisManager::import_armored_key_file(const std::string& armored_text) noexcept {
    size_t start = armored_text.find("-----BEGIN PROJECT WIRE GENESIS KEY-----");
    size_t end = armored_text.find("-----END PROJECT WIRE GENESIS KEY-----");

    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return std::nullopt;
    }

    std::string content = armored_text.substr(start, end - start);
    std::istringstream ss(content);
    std::string line;
    std::string b64_accum;

    bool in_body = false;
    while (std::getline(ss, line)) {
        if (line.empty() || line == "\r") {
            in_body = true;
            continue;
        }
        if (line.find("-----") != std::string::npos || line.find(":") != std::string::npos) {
            continue;
        }
        if (in_body) {
            // Trim whitespace
            line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
            b64_accum += line;
        }
    }

    auto decoded = base64_decode(b64_accum);
    return GenesisPayload::deserialize_binary(decoded.data(), decoded.size());
}

std::string GenesisManager::export_qr_payload(const GenesisPayload& payload) noexcept {
    auto bin = payload.serialize_binary();
    return "wire://" + base64_encode(bin.data(), bin.size());
}

} // namespace wire::genesis
