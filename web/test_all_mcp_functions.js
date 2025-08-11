#!/usr/bin/env node

/**
 * MCP 所有功能综合测试脚本
 * 依次运行所有单独的测试文件
 */

import { spawn } from 'child_process';
import path from 'path';
import { fileURLToPath } from 'url';
import { dirname } from 'path';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// 测试文件列表
const testFiles = [
    'test_get_tags.js',
    'test_get_statistics.js', 
    'test_list_content.js',
    'test_get_content.js',
    'test_search_content.js',
    'test_create_content.js'
];

/**
 * 运行单个测试文件
 * @param {string} testFile - 测试文件名
 * @returns {Promise<boolean>} - 测试是否成功
 */
function runTestFile(testFile) {
    return new Promise((resolve) => {
        console.log(`\n🚀 运行测试: ${testFile}`);
        console.log('=' .repeat(80));
        
        const testPath = path.join(__dirname, testFile);
        const child = spawn('node', [testPath], {
            stdio: 'inherit',
            cwd: __dirname
        });
        
        child.on('close', (code) => {
            if (code === 0) {
                console.log(`✅ ${testFile} 测试通过`);
                resolve(true);
            } else {
                console.log(`❌ ${testFile} 测试失败 (退出码: ${code})`);
                resolve(false);
            }
        });
        
        child.on('error', (error) => {
            console.log(`❌ ${testFile} 运行出错:`, error.message);
            resolve(false);
        });
    });
}

/**
 * 检查服务器是否运行
 */
async function checkServer() {
    const fetch = (await import('node-fetch')).default;
    const SERVER_URL = 'http://localhost:8080';
    
    try {
        const response = await fetch(`${SERVER_URL}/api/mcp`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                method: 'tools/list',
                id: 1
            })
        });
        
        if (response.ok) {
            const result = await response.json();
            return result.success || true;
        }
        return false;
    } catch (error) {
        console.log('   服务器检查错误:', error.message);
        return false;
    }
}

/**
 * 运行所有测试
 */
async function runAllTests() {
    console.log('🧪 MCP 功能综合测试开始');
    console.log('测试时间:', new Date().toLocaleString());
    console.log('=' .repeat(80));
    
    // 检查服务器状态
    console.log('🔍 检查服务器状态...');
    console.log('⚠️  跳过服务器检查，直接运行测试');
    console.log('   如果测试失败，请确保 MCP 服务器正在 http://localhost:8080 上运行');
    
    // 运行所有测试文件
    const results = [];
    
    for (const testFile of testFiles) {
        const success = await runTestFile(testFile);
        results.push({ file: testFile, success });
        
        // 在测试之间添加短暂延迟
        await new Promise(resolve => setTimeout(resolve, 1000));
    }
    
    // 显示测试总结
    console.log('\n' + '=' .repeat(80));
    console.log('📊 测试总结');
    console.log('=' .repeat(80));
    
    const passedTests = results.filter(r => r.success);
    const failedTests = results.filter(r => !r.success);
    
    console.log(`总测试数: ${results.length}`);
    console.log(`通过: ${passedTests.length}`);
    console.log(`失败: ${failedTests.length}`);
    console.log(`成功率: ${((passedTests.length / results.length) * 100).toFixed(1)}%`);
    
    if (passedTests.length > 0) {
        console.log('\n✅ 通过的测试:');
        passedTests.forEach(test => {
            console.log(`   - ${test.file}`);
        });
    }
    
    if (failedTests.length > 0) {
        console.log('\n❌ 失败的测试:');
        failedTests.forEach(test => {
            console.log(`   - ${test.file}`);
        });
    }
    
    console.log('\n🏁 所有测试完成!');
    console.log('完成时间:', new Date().toLocaleString());
    
    // 如果有失败的测试，以非零状态码退出
    if (failedTests.length > 0) {
        process.exit(1);
    }
}

// 处理命令行参数
if (process.argv.includes('--help') || process.argv.includes('-h')) {
    console.log('MCP 功能综合测试脚本');
    console.log('');
    console.log('用法:');
    console.log('  node test_all_mcp_functions.js     # 运行所有测试');
    console.log('  node test_all_mcp_functions.js -h  # 显示帮助');
    console.log('');
    console.log('单独测试文件:');
    testFiles.forEach(file => {
        console.log(`  node ${file}`);
    });
    process.exit(0);
}

// 运行测试
runAllTests().catch(error => {
    console.error('❌ 测试运行过程中发生错误:', error);
    process.exit(1);
});