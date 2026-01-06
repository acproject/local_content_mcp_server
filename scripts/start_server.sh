#!/bin/bash

# Local Content MCP Server 启动脚本

set -e

# 脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# 配置
CONFIG_FILE="${PROJECT_DIR}/config/server.json"
BUILD_DIR="${PROJECT_DIR}/build"
SERVER_BINARY="${BUILD_DIR}/server/mcp_server"
PID_FILE="${PROJECT_DIR}/data/server.pid"
LOG_DIR="${PROJECT_DIR}/logs"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# 检查依赖
check_dependencies() {
    log_info "检查依赖..."
    
    # 检查CMake
    if ! command -v cmake &> /dev/null; then
        log_error "CMake 未安装"
        exit 1
    fi
    
    # 检查编译器
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        log_error "C++ 编译器未安装"
        exit 1
    fi
    
    # 检查SQLite3
    if ! command -v sqlite3 &> /dev/null; then
        log_warn "SQLite3 命令行工具未安装，但可能不影响编译"
    fi
    
    log_success "依赖检查完成"
}

# 创建必要目录
create_directories() {
    log_info "创建必要目录..."
    
    mkdir -p "${PROJECT_DIR}/data"
    mkdir -p "${PROJECT_DIR}/logs"
    mkdir -p "${PROJECT_DIR}/static"
    mkdir -p "${BUILD_DIR}"
    
    log_success "目录创建完成"
}

# 构建项目
build_project() {
    log_info "构建项目..."
    
    cd "${BUILD_DIR}"
    
    # 配置CMake
    cmake .. -DCMAKE_BUILD_TYPE=Release
    
    # 编译
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    if [ ! -f "${SERVER_BINARY}" ]; then
        log_error "服务器二进制文件构建失败"
        exit 1
    fi
    
    log_success "项目构建完成"
}

# 检查服务器是否运行
is_server_running() {
    if [ -f "${PID_FILE}" ]; then
        local pid=$(cat "${PID_FILE}")
        if ps -p "$pid" > /dev/null 2>&1; then
            return 0
        else
            rm -f "${PID_FILE}"
            return 1
        fi
    fi
    return 1
}

# 停止服务器
stop_server() {
    if is_server_running; then
        local pid=$(cat "${PID_FILE}")
        log_info "停止服务器 (PID: $pid)..."
        
        kill "$pid"
        
        # 等待进程结束
        local count=0
        while ps -p "$pid" > /dev/null 2>&1 && [ $count -lt 30 ]; do
            sleep 1
            count=$((count + 1))
        done
        
        if ps -p "$pid" > /dev/null 2>&1; then
            log_warn "强制终止服务器..."
            kill -9 "$pid"
        fi
        
        rm -f "${PID_FILE}"
        log_success "服务器已停止"
    else
        log_info "服务器未运行"
    fi
}

# 启动服务器
start_server() {
    if is_server_running; then
        log_warn "服务器已在运行中"
        return 0
    fi
    
    log_info "启动服务器..."
    
    # 检查配置文件
    if [ ! -f "${CONFIG_FILE}" ]; then
        log_error "配置文件不存在: ${CONFIG_FILE}"
        exit 1
    fi
    
    # 启动服务器
    cd "${PROJECT_DIR}"
    nohup "${SERVER_BINARY}" --config "${CONFIG_FILE}" > "${LOG_DIR}/server_output.log" 2>&1 &
    local pid=$!
    
    # 保存PID
    echo "$pid" > "${PID_FILE}"
    
    # 等待服务器启动
    sleep 2
    
    if ps -p "$pid" > /dev/null 2>&1; then
        log_success "服务器启动成功 (PID: $pid)"
        log_info "服务器日志: ${LOG_DIR}/server_output.log"
        
        # 检查服务器是否响应
        local count=0
        while [ $count -lt 10 ]; do
            if curl -s http://localhost:8086/health > /dev/null 2>&1; then
                log_success "服务器健康检查通过"
                return 0
            fi
            sleep 1
            count=$((count + 1))
        done
        
        log_warn "服务器启动但健康检查失败，请检查日志"
    else
        log_error "服务器启动失败"
        rm -f "${PID_FILE}"
        exit 1
    fi
}

# 重启服务器
restart_server() {
    log_info "重启服务器..."
    stop_server
    sleep 1
    start_server
}

# 显示服务器状态
show_status() {
    if is_server_running; then
        local pid=$(cat "${PID_FILE}")
        log_success "服务器正在运行 (PID: $pid)"
        
        # 显示端口信息
        if command -v lsof &> /dev/null; then
            local port_info=$(lsof -p "$pid" -i TCP -a -P 2>/dev/null | grep LISTEN || true)
            if [ -n "$port_info" ]; then
                echo "监听端口:"
                echo "$port_info"
            fi
        fi
        
        # 检查健康状态
        if curl -s http://localhost:8086/health > /dev/null 2>&1; then
            log_success "服务器健康状态: 正常"
        else
            log_warn "服务器健康状态: 异常"
        fi
    else
        log_info "服务器未运行"
    fi
}

# 显示日志
show_logs() {
    local lines=${1:-50}
    
    if [ -f "${LOG_DIR}/server_output.log" ]; then
        log_info "显示最近 $lines 行日志:"
        tail -n "$lines" "${LOG_DIR}/server_output.log"
    else
        log_warn "日志文件不存在"
    fi
}

# 显示帮助
show_help() {
    echo "Local Content MCP Server 管理脚本"
    echo ""
    echo "用法: $0 [命令] [选项]"
    echo ""
    echo "命令:"
    echo "  start     启动服务器"
    echo "  stop      停止服务器"
    echo "  restart   重启服务器"
    echo "  status    显示服务器状态"
    echo "  build     构建项目"
    echo "  logs      显示日志 [行数]"
    echo "  help      显示帮助信息"
    echo ""
    echo "选项:"
    echo "  --force   强制执行操作"
    echo "  --debug   启用调试模式"
    echo ""
    echo "示例:"
    echo "  $0 start"
    echo "  $0 logs 100"
    echo "  $0 restart --force"
}

# 主函数
main() {
    local command=${1:-help}
    local force=false
    local debug=false
    
    # 解析参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            --force)
                force=true
                shift
                ;;
            --debug)
                debug=true
                set -x
                shift
                ;;
            -*)
                log_error "未知选项: $1"
                show_help
                exit 1
                ;;
            *)
                if [ -z "$command" ] || [ "$command" = "help" ]; then
                    command=$1
                fi
                shift
                ;;
        esac
    done
    
    # 执行命令
    case $command in
        start)
            check_dependencies
            create_directories
            if [ ! -f "${SERVER_BINARY}" ] || [ "$force" = true ]; then
                build_project
            fi
            start_server
            ;;
        stop)
            stop_server
            ;;
        restart)
            restart_server
            ;;
        status)
            show_status
            ;;
        build)
            check_dependencies
            create_directories
            build_project
            ;;
        logs)
            show_logs "${2:-50}"
            ;;
        help|--help|-h)
            show_help
            ;;
        *)
            log_error "未知命令: $command"
            show_help
            exit 1
            ;;
    esac
}

# 运行主函数
main "$@"