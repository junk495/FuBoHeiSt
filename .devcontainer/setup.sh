#!/usr/bin/env bash
# =============================================================================
# FuBoHeiSt – Devcontainer-Setup
# =============================================================================
# Wird als "postCreateCommand" nach jedem Container-Rebuild ausgefuehrt.
# Das Skript ist IDEMPOTENT: bereits vorhandene Komponenten werden uebersprungen.
#
# Installiert/verifiziert:
#   1. uv (liefert Python 3.13 fuer das penv)
#   2. Built-in-Core der pioarduino-Extension -> ~/.platformio/penv (Python 3.13)
#   3. PlatformIO-Core (pioarduino) fuer das Terminal -> /usr/local/py-utils/bin/pio
#   4. espressif32-Plattform + Toolchain + Arduino-Framework
#   5. cpptools-IntelliSense-Konfiguration fuer pioarduino
#
# Voraussetzung: Das Volume "platformio-core-<projekt>" ist auf
# /home/vscode/.platformio gemountet (siehe devcontainer.json). Dadurch bleiben
# Core, Toolchain (~5 GB) und Arduino-Framework ueber Rebuilds erhalten.
# =============================================================================

set -euo pipefail

PIO_VERSION="${PIO_VERSION:-6.2.0.post1}"
PIO_BIN_DIR="/usr/local/py-utils/bin"
PIO_HOME="${PLATFORMIO_CORE_DIR:-${HOME}/.platformio}"
PENV_DIR="${PIO_HOME}/penv"
# pioarduino verlangt Python 3.13 im penv (nicht das System-Python 3.14).
PYTHON_VERSION="${PYTHON_VERSION:-3.13}"

# Projektverzeichnis bestimmen. postCreateCommand laeuft zwar im Workspace-Root,
# darauf verlassen wir uns aber nicht: das Skript sucht selbst nach platformio.ini.
PROJECT_DIR="${WORKSPACE_FOLDER:-}"
if [ -z "${PROJECT_DIR}" ] || [ ! -f "${PROJECT_DIR}/platformio.ini" ]; then
	PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
fi

log()  { echo "[setup] $*"; }
fail() { echo "[setup] FEHLER: $*" >&2; exit 1; }

# ---------------------------------------------------------------------------
# 1) uv bereitstellen (uv liefert Python 3.13 fuer das penv)
#    Installation via pipx in das systemweite, bereits im PATH liegende
#    Verzeichnis /usr/local/py-utils/bin (PIPX_BIN_DIR aus der Container-Image).
# ---------------------------------------------------------------------------
install_uv() {
	if [ -x "${PIO_BIN_DIR}/uv" ]; then
		log "uv vorhanden: $("${PIO_BIN_DIR}/uv" --version)"
		return 0
	fi

	log "Installiere uv (Python-Manager fuer das penv) via pipx ..."
	if ! command -v pipx >/dev/null 2>&1; then
		fail "pipx nicht gefunden - kann uv nicht installieren"
	fi

	export PIPX_HOME="${PIPX_HOME:-/usr/local/py-utils}"
	export PIPX_BIN_DIR="${PIO_BIN_DIR}"
	pipx install uv --force || fail "Installation von uv fehlgeschlagen"

	log "uv installiert: $("${PIO_BIN_DIR}/uv" --version)"
}

# ---------------------------------------------------------------------------
# 2) Built-in-Core der pioarduino-Extension erzeugen
#    Die Extension erwartet den Core unter <core_dir>/penv mit:
#      - penv/bin/python3  (Python 3.13)
#      - penv/bin/platformio bzw. penv/bin/pio
#    <core_dir> ist PLATFORMIO_CORE_DIR bzw. ~/.platformio.
#
#    WICHTIG: Die Extension prueft im Built-in-Modus die penv-Python-Version
#    und verlangt exakt 3.13 - das System-Python (3.14) wird NICHT akzeptiert.
#    Mit "useBuiltinPIOCore: false" sucht sie nur nach Python 3.13 im System
#    und faellt sonst mit "Could not find compatible pioarduino Core" aus.
# ---------------------------------------------------------------------------
install_builtin_core() {
	local penv_python="${PENV_DIR}/bin/python3"

	# penv existiert bereits und hat die richtige Python-Version?
	if [ -x "${penv_python}" ] && "${penv_python}" -c 'import sys; sys.exit(0 if sys.version_info[:2]==(3,13) else 1)' 2>/dev/null; then
		if [ -x "${PENV_DIR}/bin/platformio" ]; then
			log "Built-in-Core vorhanden: $("${PENV_DIR}/bin/platformio" --version 2>/dev/null)"
			return 0
		fi
	fi

	# Veraltetes penv (falsche Python-Version / unvollstaendig) -> neu aufbauen
	if [ -e "${PENV_DIR}" ]; then
		log "penv ist veraltet oder unvollstaendig - wird neu erstellt ..."
		rm -rf "${PENV_DIR}"
	fi

	log "Erzeuge Built-in-Core in ${PENV_DIR} (Python ${PYTHON_VERSION}) ..."
	mkdir -p "${PIO_HOME}"
	"${PIO_BIN_DIR}/uv" venv --python "${PYTHON_VERSION}" "${PENV_DIR}" \
		|| fail "uv venv (Python ${PYTHON_VERSION}) fehlgeschlagen"

	"${PIO_BIN_DIR}/uv" pip install --python "${penv_python}" "pioarduino==${PIO_VERSION}" \
		|| fail "Installation von pioarduino==${PIO_VERSION} in penv fehlgeschlagen"

	log "Built-in-Core erstellt: $("${PENV_DIR}/bin/platformio" --version)"
}

# ---------------------------------------------------------------------------
# 3) PlatformIO-Core (pioarduino) fuer das Terminal installieren
#    Installation via pipx in das systemweite, bereits im PATH liegende
#    Verzeichnis /usr/local/py-utils/bin (PIPX_BIN_DIR aus der Container-Image).
# ---------------------------------------------------------------------------
install_terminal_core() {
	if [ -x "${PIO_BIN_DIR}/pio" ] && "${PIO_BIN_DIR}/pio" --version 2>/dev/null | grep -q "version"; then
		log "Terminal-Core vorhanden: $("${PIO_BIN_DIR}/pio" --version)"
		return 0
	fi

	log "Installiere PlatformIO-Core (pioarduino ${PIO_VERSION}) fuer das Terminal ..."
	if ! command -v pipx >/dev/null 2>&1; then
		fail "pipx nicht gefunden - kann PlatformIO-Core nicht installieren"
	fi

	export PIPX_HOME="${PIPX_HOME:-/usr/local/py-utils}"
	export PIPX_BIN_DIR="${PIO_BIN_DIR}"
	pipx install "pioarduino==${PIO_VERSION}" --force \
		|| fail "Installation von pioarduino==${PIO_VERSION} fehlgeschlagen"

	log "Terminal-Core installiert: $("${PIO_BIN_DIR}/pio" --version)"
}

# ---------------------------------------------------------------------------
# 4) Espressif32-Plattform, Toolchain und Arduino-Framework installieren
#    "pio pkg install" ist idempotent und installiert nur Fehlendes.
# ---------------------------------------------------------------------------
install_platform_packages() {
	if [ ! -f "${PROJECT_DIR}/platformio.ini" ]; then
		log "platformio.ini nicht gefunden - ueberspringe Paketinstallation"
		return 0
	fi

	log "Installiere espressif32-Plattform, Toolchain und Libraries ..."
	( cd "${PROJECT_DIR}" && "${PENV_DIR}/bin/platformio" pkg install -e esp32-s3-devkitc-1 ) \
		|| fail "Installation der PlatformIO-Pakete fehlgeschlagen"
}

# ---------------------------------------------------------------------------
# 5) cpptools-IntelliSense-Konfiguration fuer pioarduino erzeugen
#    pioarduino >= 1.4 installiert kein IntelliSense-Backend mehr automatisch.
#    "pio project init" erzeugt die c_cpp_properties.json fuer das cpptools-Backend.
# ---------------------------------------------------------------------------
setup_intellisense() {
	local props="${PROJECT_DIR}/.vscode/c_cpp_properties.json"

	if [ -f "${props}" ]; then
		log "IntelliSense-Konfiguration vorhanden - ueberspringe"
		return 0
	fi

	log "Erzeuge IntelliSense-Konfiguration (cpptools) ..."
	mkdir -p "${PROJECT_DIR}/.vscode"
	( cd "${PROJECT_DIR}" && "${PENV_DIR}/bin/platformio" project init --ide vscode ) \
		|| log "WARNUNG: 'pio project init' fehlgeschlagen - IntelliSense ggf. manuell pruefen"
}

# ---------------------------------------------------------------------------
# 6) PATH fuer das Terminal sicherstellen
#    Das Image setzt PIPX_BIN_DIR bereits in /etc/zsh/zshrc und /etc/bash.bashrc,
#    dies wird hier nur abgesichert (z. B. fuer nicht-interaktive Shells).
# ---------------------------------------------------------------------------
setup_path() {
	local profile="/etc/profile.d/10-pio-path.sh"
	if [ ! -f "${profile}" ]; then
		sudo tee "${profile}" >/dev/null 2>&1 <<-'EOF' || true
			# PlatformIO-Core (pioarduino) fuer alle Shells verfuegbar machen
			case ":${PATH}:" in
				*":/usr/local/py-utils/bin:"*) ;;
				*) export PATH="/usr/local/py-utils/bin:${PATH}" ;;
			esac
		EOF
		log "PATH-Profil angelegt: ${profile}"
	fi
}

log "=============================================="
log " FuBoHeiSt Devcontainer-Setup"
log "=============================================="

install_uv
install_builtin_core
install_terminal_core
install_platform_packages
setup_intellisense
setup_path

log ""
log "Setup abgeschlossen."
log "  Built-in-Core: $("${PENV_DIR}/bin/platformio" --version 2>/dev/null || echo 'FEHLT')"
log "  Terminal-Core: $("${PIO_BIN_DIR}/pio" --version 2>/dev/null || echo 'FEHLT')"
log "  PIO_HOME:      ${PIO_HOME}"
log "=============================================="
