#!/usr/bin/env node

import { Server } from '@modelcontextprotocol/sdk/server/index.js';
import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
import { CallToolRequestSchema, ListToolsRequestSchema } from '@modelcontextprotocol/sdk/types.js';
import fetch from 'node-fetch';

class LocalContentMCPServer {
  constructor() {
    this.server = new Server(
      {
        name: 'local-content-server',
        version: '1.0.0',
      },
      {
        capabilities: {
          tools: {},
          resources: {},
        },
      }
    );

    this.mcpServerUrl = process.env.MCP_SERVER_URL || 'http://localhost:8080';
    this.setupToolHandlers();
    this.setupResourceHandlers();
  }

  setupToolHandlers() {
    this.server.setRequestHandler(ListToolsRequestSchema, async () => {
      try {
        const response = await fetch(`${this.mcpServerUrl}/api/mcp`, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify({
            method: 'tools/list',
            params: {},
            id: 1
          })
        });

        const data = await response.json();
        
        if (data.success && data.result && data.result.tools) {
          return {
            tools: data.result.tools
          };
        }

        // 如果没有返回工具列表，返回默认工具
        return {
          tools: [
            {
              name: 'create_content',
              description: 'Create new content in the local content management system',
              inputSchema: {
                type: 'object',
                properties: {
                  title: {
                    type: 'string',
                    description: 'Title of the content'
                  },
                  content: {
                    type: 'string',
                    description: 'Content body'
                  },
                  content_type: {
                    type: 'string',
                    description: 'Type of content (document, note, etc.)'
                  },
                  tags: {
                    type: 'string',
                    description: 'Comma-separated tags'
                  }
                },
                required: ['title', 'content']
              }
            },
            {
              name: 'search_content',
              description: 'Search for content in the local content management system',
              inputSchema: {
                type: 'object',
                properties: {
                  query: {
                    type: 'string',
                    description: 'Search query'
                  },
                  page: {
                    type: 'number',
                    description: 'Page number (default: 1)'
                  },
                  page_size: {
                    type: 'number',
                    description: 'Number of results per page (default: 20)'
                  }
                },
                required: ['query']
              }
            },
            {
              name: 'get_content',
              description: 'Get specific content by ID',
              inputSchema: {
                type: 'object',
                properties: {
                  id: {
                    type: 'number',
                    description: 'Content ID'
                  }
                },
                required: ['id']
              }
            },
            {
              name: 'list_content',
              description: 'List all content with pagination',
              inputSchema: {
                type: 'object',
                properties: {
                  page: {
                    type: 'number',
                    description: 'Page number (default: 1)'
                  },
                  page_size: {
                    type: 'number',
                    description: 'Number of results per page (default: 20)'
                  }
                }
              }
            },
            {
              name: 'get_tags',
              description: 'Get all available tags',
              inputSchema: {
                type: 'object',
                properties: {}
              }
            },
            {
              name: 'get_statistics',
              description: 'Get content statistics',
              inputSchema: {
                type: 'object',
                properties: {}
              }
            }
          ]
        };
      } catch (error) {
        console.error('Error fetching tools:', error);
        return { tools: [] };
      }
    });

    this.server.setRequestHandler(CallToolRequestSchema, async (request) => {
      try {
        const { name, arguments: args } = request.params;

        const response = await fetch(`${this.mcpServerUrl}/api/mcp`, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify({
            method: 'tools/call',
            params: {
              name: name,
              arguments: args || {}
            },
            id: Date.now()
          })
        });

        const data = await response.json();

        if (data.success) {
          return {
            content: [
              {
                type: 'text',
                text: JSON.stringify(data.result, null, 2)
              }
            ]
          };
        } else {
          throw new Error(data.error?.message || 'Tool call failed');
        }
      } catch (error) {
        console.error('Error calling tool:', error);
        return {
          content: [
            {
              type: 'text',
              text: `Error: ${error.message}`
            }
          ],
          isError: true
        };
      }
    });
  }

  setupResourceHandlers() {
    // 可以在这里添加资源处理器
  }

  async run() {
    const transport = new StdioServerTransport();
    await this.server.connect(transport);
    console.error('Local Content MCP Server running on stdio');
  }
}

const server = new LocalContentMCPServer();
server.run().catch(console.error);