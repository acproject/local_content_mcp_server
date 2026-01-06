#!/usr/bin/env node

import fetch from 'node-fetch';

// 测试 list_content 功能
async function testListContent() {
  const serverUrl = process.env.MCP_SERVER_URL || 'http://localhost:8086';
  
  console.log('正在测试 list_content 功能...');
  console.log(`服务器地址: ${serverUrl}`);
  
  try {
    // 测试 list_content 工具调用
    const response = await fetch(`${serverUrl}/api/mcp`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        method: 'tools/call',
        params: {
          name: 'list_content',
          arguments: {
            page: 1,
            page_size: 10
          }
        },
        id: 1
      })
    });

    if (!response.ok) {
      throw new Error(`HTTP 错误! 状态: ${response.status}`);
    }

    const data = await response.json();
    
    console.log('\n=== list_content 测试结果 ===');
    console.log(JSON.stringify(data, null, 2));
    
    if (data.success && data.result) {
      console.log('\n✅ list_content 功能测试成功!');
      if (data.result.content && Array.isArray(data.result.content)) {
        console.log(`📄 找到 ${data.result.content.length} 条内容`);
        console.log(`📊 总计: ${data.result.total || 'N/A'} 条`);
        console.log(`📖 当前页: ${data.result.page || 1}`);
        console.log(`📑 每页大小: ${data.result.page_size || 'N/A'}`);
      }
    } else {
      console.log('❌ list_content 功能测试失败');
      console.log('错误信息:', data.error || '未知错误');
    }
    
  } catch (error) {
    console.error('❌ 测试过程中发生错误:');
    console.error(error.message);
    console.error('\n请确保:');
    console.error('1. 本地内容服务器正在运行 (http://localhost:8086)');
    console.error('2. 服务器配置正确');
    console.error('3. 网络连接正常');
  }
}

// 运行测试
testListContent();