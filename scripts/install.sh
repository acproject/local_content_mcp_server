#!/bin/bash

# Local Content MCP Server Installation Script
# This script installs the MCP server as a system service

set -e

# Configuration
SERVICE_NAME="local-content-mcp"
SERVICE_USER="mcp"
INSTALL_DIR="/opt/local-content-mcp"
SERVICE_FILE="/etc/systemd/system/${SERVICE_NAME}.service"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        log_error "This script must be run as root (use sudo)"
        exit 1
    fi
}

# Check system requirements
check_requirements() {
    log_info "Checking system requirements..."
    
    # Check OS
    if [[ ! -f /etc/os-release ]]; then
        log_error "Cannot determine OS version"
        exit 1
    fi
    
    # Check systemd
    if ! command -v systemctl &> /dev/null; then
        log_error "systemd is required but not found"
        exit 1
    fi
    
    # Check SQLite3
    if ! command -v sqlite3 &> /dev/null; then
        log_warning "SQLite3 not found, attempting to install..."
        install_dependencies
    fi
    
    log_success "System requirements check passed"
}

# Install system dependencies
install_dependencies() {
    log_info "Installing system dependencies..."
    
    if command -v apt-get &> /dev/null; then
        apt-get update
        apt-get install -y sqlite3 libsqlite3-0 curl
    elif command -v yum &> /dev/null; then
        yum install -y sqlite sqlite-libs curl
    elif command -v dnf &> /dev/null; then
        dnf install -y sqlite sqlite-libs curl
    elif command -v pacman &> /dev/null; then
        pacman -S --noconfirm sqlite curl
    else
        log_error "Unsupported package manager. Please install SQLite3 manually."
        exit 1
    fi
    
    log_success "Dependencies installed"
}

# Create service user
create_user() {
    log_info "Creating service user: $SERVICE_USER"
    
    if id "$SERVICE_USER" &>/dev/null; then
        log_warning "User $SERVICE_USER already exists"
    else
        useradd -r -s /bin/false -m -d "$INSTALL_DIR" "$SERVICE_USER"
        log_success "User $SERVICE_USER created"
    fi
}

# Build the project
build_project() {
    log_info "Building the project..."
    
    cd "$PROJECT_DIR"
    
    # Check if build directory exists
    if [[ -d "build" ]]; then
        log_warning "Build directory exists, cleaning..."
        rm -rf build
    fi
    
    # Build
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_CLIENT=ON
    make -j$(nproc)
    
    log_success "Project built successfully"
}

# Install files
install_files() {
    log_info "Installing files to $INSTALL_DIR..."
    
    # Create installation directory
    mkdir -p "$INSTALL_DIR"/{bin,config,data,logs}
    
    # Copy binaries
    cp "$PROJECT_DIR/build/server/mcp_server" "$INSTALL_DIR/bin/"
    cp "$PROJECT_DIR/build/client/mcp_client" "$INSTALL_DIR/bin/"
    
    # Copy configuration files
    cp "$PROJECT_DIR/config/"*.json "$INSTALL_DIR/config/"
    
    # Set permissions
    chown -R "$SERVICE_USER:$SERVICE_USER" "$INSTALL_DIR"
    chmod +x "$INSTALL_DIR/bin/"*
    chmod 755 "$INSTALL_DIR/bin"
    chmod 644 "$INSTALL_DIR/config/"*.json
    chmod 755 "$INSTALL_DIR/data" "$INSTALL_DIR/logs"
    
    log_success "Files installed"
}

# Install systemd service
install_service() {
    log_info "Installing systemd service..."
    
    # Copy service file
    cp "$SCRIPT_DIR/local-content-mcp.service" "$SERVICE_FILE"
    
    # Reload systemd
    systemctl daemon-reload
    
    # Enable service
    systemctl enable "$SERVICE_NAME"
    
    log_success "Service installed and enabled"
}

# Start service
start_service() {
    log_info "Starting service..."
    
    systemctl start "$SERVICE_NAME"
    
    # Wait a moment for service to start
    sleep 2
    
    # Check status
    if systemctl is-active --quiet "$SERVICE_NAME"; then
        log_success "Service started successfully"
        log_info "Service status:"
        systemctl status "$SERVICE_NAME" --no-pager -l
    else
        log_error "Service failed to start"
        log_info "Service logs:"
        journalctl -u "$SERVICE_NAME" --no-pager -l
        exit 1
    fi
}

# Test installation
test_installation() {
    log_info "Testing installation..."
    
    # Test health endpoint
    local max_attempts=30
    local attempt=1
    
    while [[ $attempt -le $max_attempts ]]; do
        if curl -f -s http://localhost:8086/health > /dev/null 2>&1; then
            log_success "Health check passed"
            break
        fi
        
        if [[ $attempt -eq $max_attempts ]]; then
            log_error "Health check failed after $max_attempts attempts"
            exit 1
        fi
        
        log_info "Waiting for service to be ready... (attempt $attempt/$max_attempts)"
        sleep 2
        ((attempt++))
    done
    
    # Test client
    if "$INSTALL_DIR/bin/mcp_client" --help > /dev/null 2>&1; then
        log_success "Client test passed"
    else
        log_warning "Client test failed, but service is running"
    fi
}

# Show installation summary
show_summary() {
    log_success "Installation completed successfully!"
    echo
    echo "Service Information:"
    echo "  Name: $SERVICE_NAME"
    echo "  User: $SERVICE_USER"
    echo "  Install Directory: $INSTALL_DIR"
    echo "  Service File: $SERVICE_FILE"
    echo
    echo "Service Management:"
    echo "  Start:   sudo systemctl start $SERVICE_NAME"
    echo "  Stop:    sudo systemctl stop $SERVICE_NAME"
    echo "  Restart: sudo systemctl restart $SERVICE_NAME"
    echo "  Status:  sudo systemctl status $SERVICE_NAME"
    echo "  Logs:    sudo journalctl -u $SERVICE_NAME -f"
    echo
    echo "API Endpoints:"
    echo "  Health:  http://localhost:8086/health"
    echo "  Info:    http://localhost:8086/info"
    echo "  MCP:     http://localhost:8086/mcp"
    echo "  REST:    http://localhost:8086/api/"
    echo
    echo "Client Usage:"
    echo "  $INSTALL_DIR/bin/mcp_client --help"
    echo
}

# Uninstall function
uninstall() {
    log_info "Uninstalling $SERVICE_NAME..."
    
    # Stop and disable service
    if systemctl is-active --quiet "$SERVICE_NAME"; then
        systemctl stop "$SERVICE_NAME"
    fi
    
    if systemctl is-enabled --quiet "$SERVICE_NAME"; then
        systemctl disable "$SERVICE_NAME"
    fi
    
    # Remove service file
    if [[ -f "$SERVICE_FILE" ]]; then
        rm "$SERVICE_FILE"
        systemctl daemon-reload
    fi
    
    # Remove installation directory
    if [[ -d "$INSTALL_DIR" ]]; then
        rm -rf "$INSTALL_DIR"
    fi
    
    # Remove user (optional)
    read -p "Remove user $SERVICE_USER? [y/N]: " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        userdel "$SERVICE_USER" 2>/dev/null || true
        log_info "User $SERVICE_USER removed"
    fi
    
    log_success "Uninstallation completed"
}

# Main function
main() {
    case "${1:-install}" in
        install)
            log_info "Starting installation of $SERVICE_NAME..."
            check_root
            check_requirements
            create_user
            build_project
            install_files
            install_service
            start_service
            test_installation
            show_summary
            ;;
        uninstall)
            check_root
            uninstall
            ;;
        *)
            echo "Usage: $0 [install|uninstall]"
            echo
            echo "Commands:"
            echo "  install    - Install the MCP server as a system service (default)"
            echo "  uninstall  - Remove the MCP server and all related files"
            exit 1
            ;;
    esac
}

# Run main function
main "$@"