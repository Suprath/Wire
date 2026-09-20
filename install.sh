#!/usr/bin/env bash
# Project Wire — One-Line Installer for macOS & Linux
# Usage: curl -fsSL https://raw.githubusercontent.com/Suprath/Wire/main/install.sh | bash

set -e

REPO="Suprath/Wire"
INSTALL_DIR="$HOME/.wire/bin"
API_URL="https://api.github.com/repos/$REPO/releases/latest"

# ── Colors ────────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'

echo ""
echo -e "${BOLD}${CYAN}  ╔══════════════════════════════════════════╗${NC}"
echo -e "${BOLD}${CYAN}  ║     Project Wire — Installer             ║${NC}"
echo -e "${BOLD}${CYAN}  ╚══════════════════════════════════════════╝${NC}"
echo ""

# ── Detect Platform ───────────────────────────────────────────────────────────
OS="$(uname -s)"
ARCH="$(uname -m)"

if [[ "$OS" == "Darwin" ]]; then
    ASSET="wire-macos-arm64.tar.gz"
    PLATFORM="macOS"
elif [[ "$OS" == "Linux" ]]; then
    ASSET="wire-linux-x86_64.tar.gz"
    PLATFORM="Linux"
else
    echo -e "${RED}  [!] Unsupported OS: $OS. Use install.ps1 on Windows.${NC}"
    exit 1
fi

echo -e "  Platform detected: ${BOLD}$PLATFORM ($ARCH)${NC}"

# ── Get Latest Release Tag ────────────────────────────────────────────────────
echo -e "  Fetching latest release from GitHub..."
LATEST_TAG=$(curl -fsSL "$API_URL" | grep '"tag_name"' | sed -E 's/.*"([^"]+)".*/\1/')

if [[ -z "$LATEST_TAG" ]]; then
    echo -e "${RED}  [!] Could not fetch release info. Check your internet connection.${NC}"
    exit 1
fi

echo -e "  Latest version: ${BOLD}$LATEST_TAG${NC}"

# ── Download ──────────────────────────────────────────────────────────────────
DOWNLOAD_URL="https://github.com/$REPO/releases/download/$LATEST_TAG/$ASSET"
TMP_DIR="$(mktemp -d)"
TMP_FILE="$TMP_DIR/$ASSET"

echo -e "  Downloading $ASSET..."
curl -fL --progress-bar -o "$TMP_FILE" "$DOWNLOAD_URL"

# ── Extract ───────────────────────────────────────────────────────────────────
echo -e "\n  Extracting..."
tar -xzf "$TMP_FILE" -C "$TMP_DIR"

# ── Install ───────────────────────────────────────────────────────────────────
mkdir -p "$INSTALL_DIR"
cp "$TMP_DIR/wire_tui" "$INSTALL_DIR/wire_tui" 2>/dev/null || true
cp "$TMP_DIR/wire_tui_interactive" "$INSTALL_DIR/wire_tui_interactive"
chmod +x "$INSTALL_DIR/wire_tui" "$INSTALL_DIR/wire_tui_interactive"

# ── macOS: Remove quarantine flag ─────────────────────────────────────────────
if [[ "$OS" == "Darwin" ]]; then
    xattr -dr com.apple.quarantine "$INSTALL_DIR/wire_tui" 2>/dev/null || true
    xattr -dr com.apple.quarantine "$INSTALL_DIR/wire_tui_interactive" 2>/dev/null || true
fi

# ── Add to PATH ───────────────────────────────────────────────────────────────
SHELL_RC=""
if [[ "$SHELL" == *"zsh"* ]]; then
    SHELL_RC="$HOME/.zshrc"
elif [[ "$SHELL" == *"bash"* ]]; then
    SHELL_RC="$HOME/.bashrc"
fi

PATH_LINE="export PATH=\"\$HOME/.wire/bin:\$PATH\""

if [[ -n "$SHELL_RC" ]] && ! grep -q ".wire/bin" "$SHELL_RC" 2>/dev/null; then
    echo "" >> "$SHELL_RC"
    echo "# Project Wire" >> "$SHELL_RC"
    echo "$PATH_LINE" >> "$SHELL_RC"
    echo -e "  Added Wire to PATH in ${SHELL_RC}"
fi

# ── Cleanup ───────────────────────────────────────────────────────────────────
rm -rf "$TMP_DIR"

# ── Done ──────────────────────────────────────────────────────────────────────
echo ""
echo -e "${GREEN}${BOLD}  ✓ Wire $LATEST_TAG installed successfully!${NC}"
echo -e "  Location: ${BOLD}$INSTALL_DIR/wire_tui_interactive${NC}"
echo ""
echo -e "  ${BOLD}To start Wire:${NC}"
echo -e "    Restart your terminal, then run:"
echo -e "      ${CYAN}wire_tui_interactive${NC}"
echo ""
echo -e "  ${BOLD}Or run it directly right now:${NC}"
echo -e "      ${CYAN}PEER_NAME=Peer_A $INSTALL_DIR/wire_tui_interactive${NC}"
echo ""
