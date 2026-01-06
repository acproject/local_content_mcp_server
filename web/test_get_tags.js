#!/usr/bin/env node

/**
 * MCP get_tags 功能测试脚本
 * 测试获取所有标签的功能
 */

import fetch from 'node-fetch';

const SERVER_URL = 'http://localhost:8086';
const MCP_API_URL = `${SERVER_URL}/api/mcp`;

/**
 * 测试 get_tags 功能
 */
async function testGetTags() {
    console.log('🏷️  测试 get_tags 功能...');
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
                    name: 'get_tags',
                    arguments: {}
                },
                id: 1
            })
        });
        
        const result = await response.json();
        
        if (result.success && result.result) {
            console.log('✅ get_tags 功能正常');
            
            if (result.result.success && result.result.data) {
                const tags = result.result.data;
                console.log(`   📊 标签统计:`);
                console.log(`   - 总标签数: ${tags.length}`);
                
                if (tags.length > 0) {
                    console.log('   📋 标签列表:');
                    tags.forEach((tag, index) => {
                        console.log(`   ${index + 1}. ${tag.name} (使用次数: ${tag.count})`);
                    });
                } else {
                    console.log('   📭 暂无标签');
                }
            } else {
                console.log('   ⚠️  返回数据格式异常:', result.result);
            }
        } else {
            console.log('❌ get_tags 功能失败');
            console.log('   错误信息:', result.error || '未知错误');
        }
        
    } catch (error) {
        console.log('❌ 请求失败:', error.message);
        console.log('   请确保服务器正在运行在', SERVER_URL);
    }
    
    console.log('=' .repeat(50));
}

// 运行测试
testGetTags().then(() => {
    console.log('\n✅ get_tags 功能测试完成!');
}).catch(error => {
    console.error('❌ 测试过程中发生错误:', error);
    process.exit(1);
});