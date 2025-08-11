#!/usr/bin/env node

/**
 * MCP search_content 功能测试脚本
 * 测试内容搜索功能
 */

import fetch from 'node-fetch';

const SERVER_URL = 'http://localhost:8080';
const MCP_API_URL = `${SERVER_URL}/api/mcp`;

/**
 * 测试 search_content 功能
 * @param {string} query - 搜索查询
 * @param {number} page - 页码
 * @param {number} pageSize - 每页大小
 */
async function testSearchContent(query, page = 1, pageSize = 10) {
    console.log(`🔍 搜索内容: "${query}" (页码: ${page}, 每页: ${pageSize})`);
    
    try {
        const response = await fetch(MCP_API_URL, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                method: 'tools/call',
                params: {
                    name: 'search_content',
                    arguments: {
                        query: query,
                        page: page,
                        page_size: pageSize
                    }
                },
                id: 1
            })
        });
        
        const result = await response.json();
        
        if (result.success && result.result) {
            if (result.result.success && result.result.data) {
                const searchResult = result.result.data;
                console.log(`✅ 搜索成功`);
                console.log(`   📊 搜索统计:`);
                console.log(`   - 总结果数: ${searchResult.total || 0}`);
                console.log(`   - 当前页: ${searchResult.page || page}`);
                console.log(`   - 每页大小: ${searchResult.page_size || pageSize}`);
                console.log(`   - 总页数: ${searchResult.total_pages || 0}`);
                
                if (searchResult.items && searchResult.items.length > 0) {
                    console.log(`   📋 搜索结果:`);
                    searchResult.items.forEach((item, index) => {
                        console.log(`   ${index + 1}. [ID: ${item.id}] ${item.title}`);
                        console.log(`      类型: ${item.content_type} | 标签: ${item.tags || '无'}`);
                        console.log(`      创建时间: ${item.created_at}`);
                        if (item.content) {
                            const preview = item.content.length > 80 
                                ? item.content.substring(0, 80) + '...' 
                                : item.content;
                            console.log(`      内容预览: ${preview}`);
                        }
                        console.log('');
                    });
                } else {
                    console.log(`   📭 未找到匹配的内容`);
                }
            } else {
                console.log(`❌ 搜索失败`);
                console.log('   错误信息:', result.result.error || '搜索结果格式异常');
            }
        } else {
            console.log(`❌ search_content 功能失败`);
            console.log('   错误信息:', result.error || '未知错误');
        }
        
    } catch (error) {
        console.log(`❌ 搜索请求失败:`, error.message);
    }
}

/**
 * 运行多个搜索测试用例
 */
async function runSearchTests() {
    console.log('🔍 测试 search_content 功能...');
    console.log('服务器地址:', SERVER_URL);
    console.log('=' .repeat(60));
    
    // 测试用例
    const testCases = [
        { query: 'CMake', description: '搜索 CMake 相关内容' },
        { query: 'documentation', description: '搜索文档相关内容' },
        { query: 'test', description: '搜索测试相关内容' },
        { query: 'nonexistent', description: '搜索不存在的内容' },
        { query: '', description: '空搜索查询' }
    ];
    
    for (const testCase of testCases) {
        console.log(`\n📝 ${testCase.description}`);
        await testSearchContent(testCase.query);
        console.log('-' .repeat(40));
    }
    
    // 测试分页功能
    console.log('\n📄 测试分页功能...');
    await testSearchContent('CMake', 1, 5);  // 第1页，每页5条
    console.log('-' .repeat(40));
    await testSearchContent('CMake', 2, 5);  // 第2页，每页5条
    
    console.log('=' .repeat(60));
}

// 运行测试
runSearchTests().then(() => {
    console.log('\n✅ search_content 功能测试完成!');
}).catch(error => {
    console.error('❌ 测试过程中发生错误:', error);
    process.exit(1);
});