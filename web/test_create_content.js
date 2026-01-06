#!/usr/bin/env node

/**
 * MCP create_content 功能测试脚本
 * 测试创建新内容的功能
 */

import fetch from 'node-fetch';

const SERVER_URL = 'http://localhost:8086';
const MCP_API_URL = `${SERVER_URL}/api/mcp`;

/**
 * 测试 create_content 功能
 * @param {string} title - 内容标题
 * @param {string} content - 内容正文
 * @param {string} contentType - 内容类型
 * @param {string} tags - 标签（逗号分隔）
 */
async function testCreateContent(title, content, contentType = 'document', tags = '') {
    console.log(`📝 创建内容: "${title}"`);
    
    try {
        const response = await fetch(MCP_API_URL, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                method: 'tools/call',
                params: {
                    name: 'create_content',
                    arguments: {
                        title: title,
                        content: content,
                        content_type: contentType,
                        tags: tags
                    }
                },
                id: 1
            })
        });
        
        const result = await response.json();
        
        if (result.success && result.result) {
            if (result.result.success && result.result.data) {
                const createdContent = result.result.data;
                console.log(`✅ 内容创建成功`);
                console.log(`   🆔 ID: ${createdContent.id}`);
                console.log(`   📝 标题: ${createdContent.title}`);
                console.log(`   📂 类型: ${createdContent.content_type}`);
                console.log(`   🏷️  标签: ${createdContent.tags || '无'}`);
                console.log(`   📅 创建时间: ${createdContent.created_at}`);
                console.log(`   📄 内容长度: ${createdContent.content ? createdContent.content.length : 0} 字符`);
                return createdContent.id;
            } else {
                console.log(`❌ 内容创建失败`);
                console.log('   错误信息:', result.result.error || '创建结果格式异常');
                return null;
            }
        } else {
            console.log(`❌ create_content 功能失败`);
            console.log('   错误信息:', result.error || '未知错误');
            return null;
        }
        
    } catch (error) {
        console.log(`❌ 创建请求失败:`, error.message);
        return null;
    }
}

/**
 * 验证创建的内容
 * @param {number} contentId - 内容 ID
 */
async function verifyCreatedContent(contentId) {
    if (!contentId) return;
    
    console.log(`🔍 验证创建的内容 (ID: ${contentId})...`);
    
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
        
        if (result.success && result.result && result.result.success && result.result.data) {
            console.log(`✅ 验证成功 - 内容确实已创建`);
        } else {
            console.log(`❌ 验证失败 - 无法找到创建的内容`);
        }
        
    } catch (error) {
        console.log(`❌ 验证请求失败:`, error.message);
    }
}

/**
 * 运行创建内容测试用例
 */
async function runCreateTests() {
    console.log('📝 测试 create_content 功能...');
    console.log('服务器地址:', SERVER_URL);
    console.log('=' .repeat(60));
    
    // 测试用例
    const testCases = [
        {
            title: 'Node.js 基础教程',
            content: 'Node.js 是一个基于 Chrome V8 引擎的 JavaScript 运行环境。它使用事件驱动、非阻塞 I/O 模型，使其轻量且高效。',
            contentType: 'markdown',
            tags: 'nodejs,javascript,tutorial',
            description: '创建 Node.js 教程文档'
        },
        {
            title: '今日工作笔记',
            content: '今天完成了 MCP 服务器的测试脚本编写，包括各个功能模块的单元测试。明天需要继续优化错误处理机制。',
            contentType: 'markdown',
            tags: 'work,daily,mcp',
            description: '创建工作笔记'
        },
        {
            title: 'API 设计规范',
            content: 'RESTful API 设计应遵循以下原则：\n1. 使用名词而非动词\n2. 使用复数形式\n3. 使用 HTTP 状态码\n4. 提供清晰的错误信息',
            contentType: 'markdown',
            tags: 'api,design,rest',
            description: '创建 API 设计文档'
        },
        {
            title: '测试空标签',
            content: '这是一个没有标签的测试内容，用于验证系统对空标签的处理。',
            contentType: 'markdown',
            tags: '',
            description: '创建无标签内容'
        },
        {
            title: '',
            content: '测试空标题',
            contentType: 'markdown',
            tags: 'test',
            description: '创建空标题内容（应该失败）'
        }
    ];
    
    const createdIds = [];
    
    for (const testCase of testCases) {
        console.log(`\n📝 ${testCase.description}`);
        const createdId = await testCreateContent(
            testCase.title,
            testCase.content,
            testCase.contentType,
            testCase.tags
        );
        
        if (createdId) {
            createdIds.push(createdId);
            await verifyCreatedContent(createdId);
        }
        
        console.log('-' .repeat(40));
    }
    
    // 显示创建的内容 ID 列表
    if (createdIds.length > 0) {
        console.log('\n📋 本次测试创建的内容 ID 列表:');
        createdIds.forEach((id, index) => {
            console.log(`   ${index + 1}. ID: ${id}`);
        });
    }
    
    console.log('=' .repeat(60));
}

// 运行测试
runCreateTests().then(() => {
    console.log('\n✅ create_content 功能测试完成!');
}).catch(error => {
    console.error('❌ 测试过程中发生错误:', error);
    process.exit(1);
});