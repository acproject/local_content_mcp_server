#!/bin/bash

# Local Content MCP Client 测试脚本

set -e

# 脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# 配置
BUILD_DIR="${PROJECT_DIR}/build"
CLIENT_BINARY="${BUILD_DIR}/client/mcp_client"
SERVER_URL="http://localhost:8080"
CONFIG_FILE="${PROJECT_DIR}/config/client.json"

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

# 检查客户端二进制文件
check_client() {
    if [ ! -f "${CLIENT_BINARY}" ]; then
        log_error "客户端二进制文件不存在: ${CLIENT_BINARY}"
        log_info "请先运行构建脚本: ./scripts/start_server.sh build"
        exit 1
    fi
}

# 检查服务器连接
check_server() {
    log_info "检查服务器连接..."
    
    if ! curl -s "${SERVER_URL}/health" > /dev/null 2>&1; then
        log_error "无法连接到服务器: ${SERVER_URL}"
        log_info "请确保服务器正在运行: ./scripts/start_server.sh start"
        exit 1
    fi
    
    log_success "服务器连接正常"
}

# 运行客户端命令
run_client() {
    local args="$@"
    log_info "执行: ${CLIENT_BINARY} ${args}"
    "${CLIENT_BINARY}" --server "${SERVER_URL}" $args
}

# 测试基本功能
test_basic_functions() {
    log_info "开始基本功能测试..."
    
    # 测试连接
    log_info "测试连接..."
    run_client test
    
    # 获取统计信息
    log_info "获取统计信息..."
    run_client stats
    
    # 获取标签
    log_info "获取标签列表..."
    run_client tags
    
    # 列出内容
    log_info "列出内容..."
    run_client list 1 5
    
    log_success "基本功能测试完成"
}

# 测试内容管理
test_content_management() {
    log_info "开始内容管理测试..."
    
    # 创建测试内容
    log_info "创建测试内容..."
    local create_output=$(run_client create "测试标题" "这是测试内容" "测试" "demo" 2>&1)
    echo "$create_output"
    
    # 从输出中提取ID（简化处理）
    local content_id=$(echo "$create_output" | grep "ID:" | head -1 | awk '{print $2}' || echo "")
    
    if [ -n "$content_id" ] && [ "$content_id" != "" ]; then
        log_success "内容创建成功，ID: $content_id"
        
        # 获取内容
        log_info "获取内容..."
        run_client get "$content_id"
        
        # 搜索内容
        log_info "搜索内容..."
        run_client search "测试"
        
        # 删除内容
        log_info "删除测试内容..."
        run_client delete "$content_id"
        
        log_success "内容管理测试完成"
    else
        log_warn "无法获取创建的内容ID，跳过后续测试"
    fi
}

# 测试REST API
test_rest_api() {
    log_info "开始REST API测试..."
    
    # 使用REST API创建内容
    log_info "使用REST API创建内容..."
    run_client --rest create "REST测试标题" "这是REST API测试内容" "rest" "api"
    
    # 使用REST API列出内容
    log_info "使用REST API列出内容..."
    run_client --rest list 1 3
    
    log_success "REST API测试完成"
}

# 性能测试
test_performance() {
    log_info "开始性能测试..."
    
    local start_time=$(date +%s)
    
    # 批量创建内容
    for i in {1..10}; do
        run_client create "性能测试 $i" "这是性能测试内容 $i" "performance" "test" > /dev/null 2>&1 || true
    done
    
    # 批量搜索
    for i in {1..5}; do
        run_client search "性能测试" > /dev/null 2>&1 || true
    done
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    log_success "性能测试完成，耗时: ${duration}秒"
}

# 交互式测试
test_interactive() {
    log_info "启动交互式测试模式..."
    log_info "在交互模式中，您可以手动测试各种命令"
    log_info "输入 'help' 查看可用命令，输入 'quit' 退出"
    
    run_client interactive
}

# 清理测试数据
cleanup_test_data() {
    log_info "清理测试数据..."
    
    # 搜索并删除测试内容
    local test_items=$(run_client search "测试" 2>/dev/null | grep "ID:" | awk '{print $2}' || true)
    
    if [ -n "$test_items" ]; then
        echo "$test_items" | while read -r id; do
            if [ -n "$id" ]; then
                log_info "删除测试内容 ID: $id"
                run_client delete "$id" > /dev/null 2>&1 || true
            fi
        done
    fi
    
    # 搜索并删除性能测试内容
    local perf_items=$(run_client search "性能测试" 2>/dev/null | grep "ID:" | awk '{print $2}' || true)
    
    if [ -n "$perf_items" ]; then
        echo "$perf_items" | while read -r id; do
            if [ -n "$id" ]; then
                log_info "删除性能测试内容 ID: $id"
                run_client delete "$id" > /dev/null 2>&1 || true
            fi
        done
    fi
    
    log_success "测试数据清理完成"
}

# 显示帮助
show_help() {
    echo "Local Content MCP Client 测试脚本"
    echo ""
    echo "用法: $0 [命令] [选项]"
    echo ""
    echo "命令:"
    echo "  basic       运行基本功能测试"
    echo "  content     运行内容管理测试"
    echo "  rest        运行REST API测试"
    echo "  performance 运行性能测试"
    echo "  interactive 启动交互式测试"
    echo "  all         运行所有测试"
    echo "  cleanup     清理测试数据"
    echo "  help        显示帮助信息"
    echo ""
    echo "选项:"
    echo "  --server <url>  指定服务器URL (默认: http://localhost:8080)"
    echo "  --verbose       详细输出"
    echo "  --no-cleanup    测试后不清理数据"
    echo ""
    echo "示例:"
    echo "  $0 basic"
    echo "  $0 all --verbose"
    echo "  $0 interactive --server http://localhost:9090"
}

# 主函数
main() {
    local command=${1:-help}
    local verbose=false
    local no_cleanup=false
    
    # 解析参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            --server)
                SERVER_URL="$2"
                shift 2
                ;;
            --verbose)
                verbose=true
                shift
                ;;
            --no-cleanup)
                no_cleanup=true
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
    
    # 设置详细输出
    if [ "$verbose" = true ]; then
        set -x
    fi
    
    # 检查前置条件
    check_client
    
    # 执行命令
    case $command in
        basic)
            check_server
            test_basic_functions
            ;;
        content)
            check_server
            test_content_management
            if [ "$no_cleanup" != true ]; then
                cleanup_test_data
            fi
            ;;
        rest)
            check_server
            test_rest_api
            if [ "$no_cleanup" != true ]; then
                cleanup_test_data
            fi
            ;;
        performance)
            check_server
            test_performance
            if [ "$no_cleanup" != true ]; then
                cleanup_test_data
            fi
            ;;
        interactive)
            check_server
            test_interactive
            ;;
        all)
            check_server
            test_basic_functions
            echo ""
            test_content_management
            echo ""
            test_rest_api
            echo ""
            test_performance
            if [ "$no_cleanup" != true ]; then
                echo ""
                cleanup_test_data
            fi
            log_success "所有测试完成！"
            ;;
        cleanup)
            check_server
            cleanup_test_data
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