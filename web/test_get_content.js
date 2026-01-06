#!/usr/bin/env node

/**
 * MCP get_content 功能测试脚本
 * 测试根据 ID 获取特定内容的功能
 */

import fetch from 'node-fetch';

const SERVER_URL = 'http://localhost:8086';
const MCP_API_URL = `${SERVER_URL}/api/mcp`;

/**
 * 测试 get_content 功能
 * @param {number} contentId - 要获取的内容 ID
 */
async function testGetContent(contentId) {
    console.log(`📄 测试 get_content 功能 (ID: ${contentId})...`);
    
    try {
        const response = await fetch(MCP_API_URL, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                method: 'tools/call',
                params: {
                    name: 'get_content',
                    arguments: {
                        id: contentId
                    }
                },
                id: 1
            })
        });
        
        const result = await response.json();
        
        if (result.success && result.result) {
            if (result.result.success && result.result.data) {
                const content = result.result.data;
                console.log(`✅ 成功获取内容 (ID: ${contentId})`);
                console.log(`   📝 标题: ${content.title}`);
                console.log(`   📂 类型: ${content.content_type}`);
                console.log(`   🏷️  标签: ${content.tags || '无'}`);
                console.log(`   📅 创建时间: ${content.created_at}`);
                console.log(`   📅 更新时间: ${content.updated_at}`);
                console.log(`   📄 内容预览: ${content.content ? content.content.substring(0, 100) + '...' : '无内容'}`);
            } else {
                console.log(`❌ 获取内容失败 (ID: ${contentId})`);
                console.log('   错误信息:', result.result.error || '内容不存在或格式异常');
            }
        } else {
            console.log(`❌ get_content 功能失败 (ID: ${contentId})`);
            console.log('   错误信息:', result.error || '未知错误');
        }
        
    } catch (error) {
        console.log(`❌ 请求失败 (ID: ${contentId}):`, error.message);
    }
}

/**
 * 运行多个测试用例
 */
async function runTests() {
    console.log('📄 测试 get_content 功能...');
    console.log('服务器地址:', SERVER_URL);
    console.log('=' .repeat(50));
    
    // 测试存在的内容 ID
    const testIds = [1, 2, 3];
    
    for (const id of testIds) {
        await testGetContent(id);
        console.log('-' .repeat(30));
    }
    
    // 测试不存在的内容 ID
    console.log('🔍 测试不存在的内容 ID...');
    await testGetContent(999);
    
    console.log('=' .repeat(50));
}

// 运行测试
runTests().then(() => {
    console.log('\n✅ get_content 功能测试完成!');
}).catch(error => {
    console.error('❌ 测试过程中发生错误:', error);
    process.exit(1);
});