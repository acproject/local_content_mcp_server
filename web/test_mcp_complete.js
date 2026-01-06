#!/usr/bin/env node

import fetch from 'node-fetch';

// 测试所有 MCP 功能
async function testMCPFunctions() {
  const serverUrl = process.env.MCP_SERVER_URL || 'http://localhost:8086';
  
  console.log('🚀 开始测试 MCP 功能...');
  console.log(`服务器地址: ${serverUrl}`);
  console.log('=' .repeat(50));
  
  try {
    // 1. 测试获取工具列表
    console.log('\n📋 测试 1: 获取工具列表');
    const toolsResponse = await fetch(`${serverUrl}/api/mcp`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        method: 'tools/list',
        params: {},
        id: 1
      })
    });
    
    const toolsData = await toolsResponse.json();
    if (toolsData.success && toolsData.result && toolsData.result.tools) {
      console.log(`✅ 成功获取 ${toolsData.result.tools.length} 个工具:`);
      toolsData.result.tools.forEach(tool => {
        console.log(`   - ${tool.name}: ${tool.description}`);
      });
    } else {
      console.log('❌ 获取工具列表失败');
    }

    // 2. 测试创建内容
    console.log('\n📝 测试 2: 创建测试内容');
    const createResponse = await fetch(`${serverUrl}/api/mcp`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        method: 'tools/call',
        params: {
          name: 'create_content',
          arguments: {
            title: 'MCP 测试内容',
            content: '这是一个用于测试 MCP list_content 功能的示例内容。',
            content_type: 'test',
            tags: 'mcp,test,demo'
          }
        },
        id: 2
      })
    });
    
    const createData = await createResponse.json();
    if (createData.success) {
      console.log('✅ 成功创建测试内容');
      console.log(`   内容ID: ${createData.result?.id || 'N/A'}`);
    } else {
      console.log('❌ 创建内容失败:', createData.error || '未知错误');
    }

    // 3. 测试 list_content
    console.log('\n📄 测试 3: 列出所有内容 (list_content)');
    const listResponse = await fetch(`${serverUrl}/api/mcp`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        method: 'tools/call',
        params: {
          name: 'list_content',
          arguments: {
            page: 1,
            page_size: 10
          }
        },
        id: 3
      })
    });

    const listData = await listResponse.json();
    if (listData.success) {
      console.log('✅ list_content 功能正常');
      if (listData.result && listData.result.content) {
        console.log(`   找到 ${listData.result.content.length} 条内容`);
        console.log(`   总计: ${listData.result.total || 'N/A'} 条`);
        console.log(`   当前页: ${listData.result.page || 1}`);
        console.log(`   每页大小: ${listData.result.page_size || 'N/A'}`);
        
        // 显示前几条内容
        if (listData.result.content.length > 0) {
          console.log('\n   📋 内容列表:');
          listData.result.content.slice(0, 3).forEach((item, index) => {
            console.log(`   ${index + 1}. [ID: ${item.id}] ${item.title}`);
            console.log(`      类型: ${item.content_type || 'N/A'} | 标签: ${item.tags || 'N/A'}`);
            console.log(`      创建时间: ${item.created_at || 'N/A'}`);
          });
          if (listData.result.content.length > 3) {
            console.log(`   ... 还有 ${listData.result.content.length - 3} 条内容`);
          }
        }
      } else {
        console.log('   📭 暂无内容');
      }
    } else {
      console.log('❌ list_content 功能失败:', listData.error || '未知错误');
    }

    // 4. 测试获取统计信息
    console.log('\n📊 测试 4: 获取统计信息');
    const statsResponse = await fetch(`${serverUrl}/api/mcp`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        method: 'tools/call',
        params: {
          name: 'get_statistics',
          arguments: {}
        },
        id: 4
      })
    });

    const statsData = await statsResponse.json();
    if (statsData.success && statsData.result) {
      console.log('✅ 统计信息获取成功:');
      console.log(`   总内容数: ${statsData.result.total_content || 'N/A'}`);
      console.log(`   总标签数: ${statsData.result.total_tags || 'N/A'}`);
    } else {
      console.log('❌ 获取统计信息失败');
    }

    // 5. 测试获取标签
    console.log('\n🏷️  测试 5: 获取所有标签');
    const tagsResponse = await fetch(`${serverUrl}/api/mcp`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        method: 'tools/call',
        params: {
          name: 'get_tags',
          arguments: {}
        },
        id: 5
      })
    });

    const tagsData = await tagsResponse.json();
    if (tagsData.success && tagsData.result) {
      console.log('✅ 标签获取成功:');
      if (tagsData.result.tags && tagsData.result.tags.length > 0) {
        console.log(`   可用标签: ${tagsData.result.tags.join(', ')}`);
      } else {
        console.log('   暂无标签');
      }
    } else {
      console.log('❌ 获取标签失败');
    }

    console.log('\n' + '='.repeat(50));
    console.log('🎉 MCP 功能测试完成!');
    console.log('\n💡 现在你可以在 Claude Desktop 中使用这些功能了:');
    console.log('   - 说 "帮我列出所有内容" 来测试 list_content');
    console.log('   - 说 "创建一个新笔记" 来测试 create_content');
    console.log('   - 说 "搜索关于XXX的内容" 来测试 search_content');
    
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
testMCPFunctions();