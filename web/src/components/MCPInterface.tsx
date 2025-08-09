import React, { useState, useEffect } from 'react';
import {
  Box,
  Typography,
  TextField,
  Button,
  Paper,
  Alert,
  Accordion,
  AccordionSummary,
  AccordionDetails,
  Card,
  CardContent,
  Chip,
  Grid,
  Divider,
  IconButton,
  Tooltip,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
} from '@mui/material';
import {
  ExpandMore as ExpandMoreIcon,
  Send as SendIcon,
  Clear as ClearIcon,
  ContentCopy as CopyIcon,
  Info as InfoIcon,
  PlayArrow as PlayIcon,
} from '@mui/icons-material';
import { Prism as SyntaxHighlighter } from 'react-syntax-highlighter';
import { tomorrow } from 'react-syntax-highlighter/dist/esm/styles/prism';

interface MCPTool {
  name: string;
  description: string;
  inputSchema: any;
}

interface MCPRequest {
  jsonrpc: string;
  id: string | number;
  method: string;
  params?: any;
}

interface MCPResponse {
  jsonrpc?: string;
  id?: string | number;
  result?: any;
  error?: {
    code: number;
    message: string;
  };
}

const MCPInterface = () => {
  const [tools, setTools] = useState<MCPTool[]>([]);
  const [request, setRequest] = useState<string>('');
  const [response, setResponse] = useState<string>('');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [serverInfo, setServerInfo] = useState<any>(null);
  const [selectedTool, setSelectedTool] = useState<MCPTool | null>(null);
  const [toolParams, setToolParams] = useState<string>('{}');
  const [showToolDialog, setShowToolDialog] = useState(false);

  useEffect(() => {
    initializeMCP();
  }, []);

  const initializeMCP = async () => {
    try {
      // 初始化MCP连接
      const initRequest: MCPRequest = {
        jsonrpc: '2.0',
        id: 1,
        method: 'initialize',
        params: {
          protocolVersion: '2024-11-05',
          capabilities: {},
          clientInfo: {
            name: 'Web MCP Client',
            version: '1.0.0'
          }
        }
      };

      const initResponse = await sendMCPRequest(initRequest);
      if (initResponse.result) {
        setServerInfo(initResponse.result.serverInfo);
      }

      // 获取工具列表
      const toolsRequest: MCPRequest = {
        jsonrpc: '2.0',
        id: 2,
        method: 'tools/list'
      };

      const toolsResponse = await sendMCPRequest(toolsRequest);
      if (toolsResponse.result && toolsResponse.result.tools) {
        setTools(toolsResponse.result.tools);
      }
    } catch (err) {
      setError('初始化MCP连接失败: ' + (err as Error).message);
    }
  };

  const sendMCPRequest = async (mcpRequest: MCPRequest): Promise<MCPResponse> => {
    const response = await fetch('/mcp', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(mcpRequest),
    });

    if (!response.ok) {
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }

    return await response.json();
  };

  const handleSendRequest = async () => {
    if (!request.trim()) {
      setError('请输入MCP请求');
      return;
    }

    setLoading(true);
    setError(null);

    try {
      const mcpRequest: MCPRequest = JSON.parse(request);
      const mcpResponse = await sendMCPRequest(mcpRequest);
      setResponse(JSON.stringify(mcpResponse, null, 2));
    } catch (err) {
      setError('请求失败: ' + (err as Error).message);
    } finally {
      setLoading(false);
    }
  };

  const handleToolCall = async (tool: MCPTool) => {
    setSelectedTool(tool);
    setShowToolDialog(true);
  };

  const executeToolCall = async () => {
    if (!selectedTool) return;

    setLoading(true);
    setError(null);

    try {
      const params = JSON.parse(toolParams);
      const toolRequest: MCPRequest = {
        jsonrpc: '2.0',
        id: Date.now(),
        method: 'tools/call',
        params: {
          name: selectedTool.name,
          arguments: params
        }
      };

      const toolResponse = await sendMCPRequest(toolRequest);
      setResponse(JSON.stringify(toolResponse, null, 2));
      setShowToolDialog(false);
    } catch (err) {
      setError('工具调用失败: ' + (err as Error).message);
    } finally {
      setLoading(false);
    }
  };

  const copyToClipboard = (text: string) => {
    navigator.clipboard.writeText(text);
  };

  const generateExampleRequest = (tool: MCPTool) => {
    const exampleParams: any = {};
    
    if (tool.inputSchema && tool.inputSchema.properties) {
      Object.keys(tool.inputSchema.properties).forEach(key => {
        const prop = tool.inputSchema.properties[key];
        if (prop.type === 'string') {
          exampleParams[key] = prop.description || 'example_value';
        } else if (prop.type === 'number') {
          exampleParams[key] = 123;
        } else if (prop.type === 'boolean') {
          exampleParams[key] = true;
        } else if (prop.type === 'object') {
          exampleParams[key] = {};
        }
      });
    }

    const exampleRequest: MCPRequest = {
      jsonrpc: '2.0',
      id: Date.now(),
      method: 'tools/call',
      params: {
        name: tool.name,
        arguments: exampleParams
      }
    };

    setRequest(JSON.stringify(exampleRequest, null, 2));
  };

  return (
    <Box sx={{ p: 3 }}>
      <Typography variant="h4" component="h1" gutterBottom>
        MCP 服务接口
      </Typography>
      
      <Typography variant="body1" color="textSecondary" paragraph>
        这个界面允许大语言模型通过 Model Context Protocol (MCP) 与本地内容管理服务进行交互。
      </Typography>

      {error && (
        <Alert severity="error" sx={{ mb: 2 }}>
          {error}
        </Alert>
      )}

      {/* 服务器信息 */}
      {serverInfo && (
        <Card sx={{ mb: 3 }}>
          <CardContent>
            <Typography variant="h6" gutterBottom>
              服务器信息
            </Typography>
            <Typography variant="body2">
              名称: {serverInfo.name}
            </Typography>
            <Typography variant="body2">
              版本: {serverInfo.version}
            </Typography>
          </CardContent>
        </Card>
      )}

      <Grid container spacing={3}>
        {/* 左侧：工具列表 */}
        <Grid item xs={12} md={6}>
          <Paper sx={{ p: 2 }}>
            <Typography variant="h6" gutterBottom>
              可用工具
            </Typography>
            
            {tools.map((tool, index) => (
              <Accordion key={index}>
                <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                  <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                    <Typography variant="subtitle1">{tool.name}</Typography>
                    <Chip label="工具" size="small" color="primary" />
                  </Box>
                </AccordionSummary>
                <AccordionDetails>
                  <Typography variant="body2" paragraph>
                    {tool.description}
                  </Typography>
                  
                  {tool.inputSchema && (
                    <Box sx={{ mb: 2 }}>
                      <Typography variant="subtitle2" gutterBottom>
                        输入参数:
                      </Typography>
                      <SyntaxHighlighter
                        language="json"
                        style={tomorrow}
                        customStyle={{ fontSize: '12px', maxHeight: '200px' }}
                      >
                        {JSON.stringify(tool.inputSchema, null, 2)}
                      </SyntaxHighlighter>
                    </Box>
                  )}
                  
                  <Box sx={{ display: 'flex', gap: 1 }}>
                    <Button
                      size="small"
                      variant="contained"
                      startIcon={<PlayIcon />}
                      onClick={() => handleToolCall(tool)}
                    >
                      调用工具
                    </Button>
                    <Button
                      size="small"
                      variant="outlined"
                      startIcon={<CopyIcon />}
                      onClick={() => generateExampleRequest(tool)}
                    >
                      生成示例
                    </Button>
                  </Box>
                </AccordionDetails>
              </Accordion>
            ))}
          </Paper>
        </Grid>

        {/* 右侧：请求/响应 */}
        <Grid item xs={12} md={6}>
          <Paper sx={{ p: 2 }}>
            <Typography variant="h6" gutterBottom>
              MCP 请求测试
            </Typography>
            
            {/* 请求输入 */}
            <Box sx={{ mb: 2 }}>
              <Typography variant="subtitle2" gutterBottom>
                请求 (JSON):
              </Typography>
              <TextField
                fullWidth
                multiline
                rows={8}
                value={request}
                onChange={(e) => setRequest(e.target.value)}
                placeholder='{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "tools/list"
}'
                variant="outlined"
                sx={{ fontFamily: 'monospace' }}
              />
            </Box>
            
            {/* 操作按钮 */}
            <Box sx={{ display: 'flex', gap: 1, mb: 2 }}>
              <Button
                variant="contained"
                startIcon={<SendIcon />}
                onClick={handleSendRequest}
                disabled={loading}
              >
                {loading ? '发送中...' : '发送请求'}
              </Button>
              <Button
                variant="outlined"
                startIcon={<ClearIcon />}
                onClick={() => {
                  setRequest('');
                  setResponse('');
                  setError(null);
                }}
              >
                清空
              </Button>
            </Box>
            
            {/* 响应显示 */}
            {response && (
              <Box>
                <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 1 }}>
                  <Typography variant="subtitle2">
                    响应:
                  </Typography>
                  <Tooltip title="复制响应">
                    <IconButton
                      size="small"
                      onClick={() => copyToClipboard(response)}
                    >
                      <CopyIcon fontSize="small" />
                    </IconButton>
                  </Tooltip>
                </Box>
                <SyntaxHighlighter
                  language="json"
                  style={tomorrow}
                  customStyle={{ maxHeight: '300px' }}
                >
                  {response}
                </SyntaxHighlighter>
              </Box>
            )}
          </Paper>
        </Grid>
      </Grid>

      {/* 工具调用对话框 */}
      <Dialog
        open={showToolDialog}
        onClose={() => setShowToolDialog(false)}
        maxWidth="md"
        fullWidth
      >
        <DialogTitle>
          调用工具: {selectedTool?.name}
        </DialogTitle>
        <DialogContent>
          <Typography variant="body2" paragraph>
            {selectedTool?.description}
          </Typography>
          
          <Typography variant="subtitle2" gutterBottom>
            参数 (JSON):
          </Typography>
          <TextField
            fullWidth
            multiline
            rows={6}
            value={toolParams}
            onChange={(e) => setToolParams(e.target.value)}
            placeholder='{}'
            variant="outlined"
            sx={{ fontFamily: 'monospace' }}
          />
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setShowToolDialog(false)}>
            取消
          </Button>
          <Button
            variant="contained"
            onClick={executeToolCall}
            disabled={loading}
          >
            {loading ? '执行中...' : '执行'}
          </Button>
        </DialogActions>
      </Dialog>
    </Box>
  );
};

export default MCPInterface;