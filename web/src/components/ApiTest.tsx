import React, { useState } from 'react';
import { Box, Button, Typography, Paper } from '@mui/material';
import { ContentAPI } from '../services/api';

const ApiTest = () => {
  const [result, setResult] = useState<any>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const testApi = async () => {
    console.log('=== API Test Started ===');
    setLoading(true);
    setError(null);
    setResult(null);

    try {
      console.log('Calling ContentAPI.listContent(1, 10)...');
      const data = await ContentAPI.listContent(1, 10);
      console.log('API Response:', data);
      setResult(data);
    } catch (err: any) {
      console.error('API Error:', err);
      setError(err.message || 'API调用失败');
    } finally {
      setLoading(false);
    }
  };

  return (
    <Box sx={{ p: 3 }}>
      <Typography variant="h4" gutterBottom>
        API 测试页面
      </Typography>
      
      <Button 
        variant="contained" 
        onClick={testApi} 
        disabled={loading}
        sx={{ mb: 2 }}
      >
        {loading ? '测试中...' : '测试 API'}
      </Button>

      {error && (
        <Paper sx={{ p: 2, mb: 2, bgcolor: 'error.light' }}>
          <Typography color="error">
            错误: {error}
          </Typography>
        </Paper>
      )}

      {result && (
        <Paper sx={{ p: 2 }}>
          <Typography variant="h6" gutterBottom>
            API 响应结果:
          </Typography>
          <pre style={{ whiteSpace: 'pre-wrap', fontSize: '12px' }}>
            {JSON.stringify(result, null, 2)}
          </pre>
        </Paper>
      )}
    </Box>
  );
};

export default ApiTest;