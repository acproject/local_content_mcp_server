#!/usr/bin/env node

/**
 * MCP get_statistics 功能测试脚本
 * 测试获取内容统计信息的功能
 */

import fetch from 'node-fetch';

const SERVER_URL = 'http://localhost:8080';
const MCP_API_URL = `${SERVER_URL}/api/mcp`;

/**
 * 测试 get_statistics 功能
 */
async function testGetStatistics() {
    console.log('📊 测试 get_statistics 功能...');
    console.log('服务器地址:', SERVER_URL);
    console.log('=' .repeat(50));
    
    try {
        const response = await fetch(MCP_API_URL, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                method: 'tools/call',
                params: {
                    name: 'get_statistics',
                    arguments: {}
                },
                id: 1
            })
        });
        
        const result = await response.json();
        
        if (result.success && result.result) {
            console.log('✅ get_statistics 功能正常');
            
            if (result.result.success && result.result.data) {
                const stats = result.result.data;
                console.log('   📈 统计信息:');
                console.log(`   - 总内容数: ${stats.total_content || 'N/A'}`);
                console.log(`   - 总标签数: ${stats.total_tags || 'N/A'}`);
                console.log(`   - 文档类型数: ${stats.document_count || 'N/A'}`);
                console.log(`   - 笔记类型数: ${stats.note_count || 'N/A'}`);
                
                if (stats.content_by_type) {
                    console.log('   📋 按类型分组:');
                    Object.entries(stats.content_by_type).forEach(([type, count]) => {
                        console.log(`     - ${type}: ${count}`);
                    });
                }
                
                if (stats.recent_activity) {
                    console.log('   🕒 最近活动:');
                    console.log(`     - 今日新增: ${stats.recent_activity.today || 0}`);
                    console.log(`     - 本周新增: ${stats.recent_activity.this_week || 0}`);
                    console.log(`     - 本月新增: ${stats.recent_activity.this_month || 0}`);
                }
            } else {
                console.log('   ⚠️  返回数据格式异常:', result.result);
            }
        } else {
            console.log('❌ get_statistics 功能失败');
            console.log('   错误信息:', result.error || '未知错误');
        }
        
    } catch (error) {
        console.log('❌ 请求失败:', error.message);
        console.log('   请确保服务器正在运行在', SERVER_URL);
    }
    
    console.log('=' .repeat(50));
}

// 运行测试
testGetStatistics().then(() => {
    console.log('\n✅ get_statistics 功能测试完成!');
}).catch(error => {
    console.error('❌ 测试过程中发生错误:', error);
    process.exit(1);
});