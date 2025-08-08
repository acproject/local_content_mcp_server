import React, { useState, useEffect } from 'react';
import {
  Paper,
  TextField,
  Button,
  Typography,
  Box,
  Alert,
  Snackbar,
  Card,
  CardContent,
  Divider,
  FormControlLabel,
  Switch,
  Select,
  MenuItem,
  FormControl,
  InputLabel,
  Grid,
  Accordion,
  AccordionSummary,
  AccordionDetails,
} from '@mui/material';
import {
  Save as SaveIcon,
  Refresh as RefreshIcon,
  ExpandMore as ExpandMoreIcon,
} from '@mui/icons-material';

import { ConfigAPI, ServerConfig, configService } from '../services/api';

const ConfigView: React.FC = () => {
  const [config, setConfig] = useState<ServerConfig>({
    host: '',
    port: 0,
    database_path: '',
    log_level: 'info',
    log_file: '',
    max_content_size: 1048576,
    default_page_size: 20,
    max_page_size: 100,
    enable_cors: true,
    cors_origin: '*',
    static_files_path: './web',
    enable_static_files: true,
    upload_path: './uploads',
    max_file_size: 10485760,
    allowed_file_types: [],
    enable_file_upload: true,
    llama_model_path: '',
    llama_executable_path: './llama.cpp/main',
    llama_context_size: 2048,
    llama_threads: 4,
    llama_temperature: 0.7,
    llama_max_tokens: 512,
    enable_llama: false,
    ollama_host: 'localhost',
    ollama_port: 11434,
    ollama_model: 'llama2',
    ollama_temperature: 0.7,
    ollama_max_tokens: 512,
    ollama_timeout: 30,
    enable_ollama: false,
  });
  const [loading, setLoading] = useState(true);
  const [saving, setSaving] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [snackbarOpen, setSnackbarOpen] = useState(false);
  const [snackbarMessage, setSnackbarMessage] = useState('');
  const [snackbarSeverity, setSnackbarSeverity] = useState<'success' | 'error'>('success');

  useEffect(() => {
    fetchConfig();
  }, []);

  const fetchConfig = async () => {
    try {
      setLoading(true);
      const data = await ConfigAPI.getConfig();
      setConfig(data);
      setError(null);
    } catch (err) {
      setError('获取配置失败');
      console.error('Error fetching config:', err);
    } finally {
      setLoading(false);
    }
  };

  const handleInputChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    const { name, value, type, checked } = event.target;
    setConfig(prev => ({
      ...prev,
      [name]: type === 'checkbox' ? checked : 
              ['port', 'max_content_size', 'default_page_size', 'max_page_size', 'max_file_size', 'llama_context_size', 'llama_threads', 'llama_max_tokens', 'ollama_port', 'ollama_max_tokens', 'ollama_timeout'].includes(name) ? parseInt(value) || 0 :
              ['llama_temperature', 'ollama_temperature'].includes(name) ? Math.round((parseFloat(value) || 0) * 100) / 100 :
              value,
    }));
  };

  const handleSelectChange = (event: any) => {
    const { name, value } = event.target;
    setConfig(prev => ({
      ...prev,
      [name]: value,
    }));
  };

  const handleFileTypesChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    const value = event.target.value;
    const fileTypes = value.split(',').map(type => type.trim()).filter(type => type);
    setConfig(prev => ({
      ...prev,
      allowed_file_types: fileTypes,
    }));
  };

  const handleSubmit = async (event: React.FormEvent) => {
    event.preventDefault();
    
    if (!config.host.trim()) {
      setSnackbarMessage('主机地址不能为空');
      setSnackbarSeverity('error');
      setSnackbarOpen(true);
      return;
    }
    
    if (config.port <= 0 || config.port > 65535) {
      setSnackbarMessage('端口号必须在1-65535之间');
      setSnackbarSeverity('error');
      setSnackbarOpen(true);
      return;
    }

    try {
      setSaving(true);
      await ConfigAPI.updateConfig(config);
      setSnackbarMessage('配置保存成功！服务器需要重启才能生效。');
      setSnackbarSeverity('success');
      setSnackbarOpen(true);
      setError(null);
    } catch (err: any) {
      console.error('❌ Config save failed:', err);
      console.error('❌ Error details:', {
        message: err?.message,
        response: err?.response?.data,
        status: err?.response?.status,
        config: err?.config
      });
      
      let errorMessage = '保存配置失败';
      if (err?.response?.status) {
        errorMessage += ` (HTTP ${err.response.status})`;
      }
      if (err?.message) {
        errorMessage += `: ${err.message}`;
      }
      
      setSnackbarMessage(errorMessage);
      setSnackbarSeverity('error');
      setSnackbarOpen(true);
    } finally {
      setSaving(false);
    }
  };

  const handleRefresh = () => {
    fetchConfig();
  };

  if (loading) {
    return (
      <Box display="flex" justifyContent="center" mt={4}>
        <Typography>加载中...</Typography>
      </Box>
    );
  }

  return (
    <Box>
      <Typography variant="h4" component="h1" gutterBottom>
        服务器配置
      </Typography>

      <Card sx={{ mb: 3 }}>
        <CardContent>
          <Typography variant="h6" gutterBottom>
            配置说明
          </Typography>
          <Typography variant="body2" color="textSecondary" paragraph>
            这里可以配置服务器的基本参数。修改配置后需要重启服务器才能生效。
          </Typography>
          <Typography variant="body2" color="textSecondary" paragraph>
            当前连接的服务器: {configService.getApiBaseUrl().replace('/api', '')}
          </Typography>
          <Typography variant="body2" color="textSecondary">
            配置通过服务器 API 动态获取和保存
          </Typography>
        </CardContent>
      </Card>

      <Paper elevation={3} sx={{ p: 3 }}>
        <Box display="flex" justifyContent="space-between" alignItems="center" mb={3}>
          <Typography variant="h6">
            服务器设置
          </Typography>
          <Button
            variant="outlined"
            startIcon={<RefreshIcon />}
            onClick={handleRefresh}
            disabled={loading || saving}
          >
            刷新
          </Button>
        </Box>

        <Divider sx={{ mb: 3 }} />

        <Box component="form" onSubmit={handleSubmit}>
          {/* 基本服务器配置 */}
          <Accordion defaultExpanded>
            <AccordionSummary expandIcon={<ExpandMoreIcon />}>
              <Typography variant="h6">基本服务器配置</Typography>
            </AccordionSummary>
            <AccordionDetails>
              <Grid container spacing={2}>
                <Grid item xs={12} md={6}>
                  <TextField
                    fullWidth
                    label="主机地址"
                    name="host"
                    value={config.host}
                    onChange={handleInputChange}
                    margin="normal"
                    required
                    disabled={saving}
                    helperText="服务器绑定的IP地址，0.0.0.0表示绑定所有网络接口"
                  />
                </Grid>
                <Grid item xs={12} md={6}>
                  <TextField
                    fullWidth
                    label="端口号"
                    name="port"
                    type="number"
                    value={config.port}
                    onChange={handleInputChange}
                    margin="normal"
                    required
                    disabled={saving}
                    inputProps={{ min: 1, max: 65535 }}
                    helperText="服务器监听的端口号，范围：1-65535"
                  />
                </Grid>
              </Grid>
            </AccordionDetails>
          </Accordion>

          {/* 数据库配置 */}
          <Accordion>
            <AccordionSummary expandIcon={<ExpandMoreIcon />}>
              <Typography variant="h6">数据库配置</Typography>
            </AccordionSummary>
            <AccordionDetails>
              <TextField
                fullWidth
                label="数据库路径"
                name="database_path"
                value={config.database_path}
                onChange={handleInputChange}
                margin="normal"
                disabled={saving}
                helperText="SQLite数据库文件的路径"
              />
            </AccordionDetails>
          </Accordion>

          {/* 日志配置 */}
          <Accordion>
            <AccordionSummary expandIcon={<ExpandMoreIcon />}>
              <Typography variant="h6">日志配置</Typography>
            </AccordionSummary>
            <AccordionDetails>
              <Grid container spacing={2}>
                <Grid item xs={12} md={6}>
                  <FormControl fullWidth margin="normal">
                    <InputLabel>日志级别</InputLabel>
                    <Select
                      name="log_level"
                      value={config.log_level}
                      onChange={handleSelectChange}
                      disabled={saving}
                    >
                      <MenuItem value="debug">Debug</MenuItem>
                      <MenuItem value="info">Info</MenuItem>
                      <MenuItem value="warning">Warning</MenuItem>
                      <MenuItem value="error">Error</MenuItem>
                    </Select>
                  </FormControl>
                </Grid>
                <Grid item xs={12} md={6}>
                  <TextField
                    fullWidth
                    label="日志文件路径"
                    name="log_file"
                    value={config.log_file}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving}
                    helperText="日志文件的保存路径，留空则输出到控制台"
                  />
                </Grid>
              </Grid>
            </AccordionDetails>
          </Accordion>

          {/* 内容管理配置 */}
          <Accordion>
            <AccordionSummary expandIcon={<ExpandMoreIcon />}>
              <Typography variant="h6">内容管理配置</Typography>
            </AccordionSummary>
            <AccordionDetails>
              <Grid container spacing={2}>
                <Grid item xs={12} md={4}>
                  <TextField
                    fullWidth
                    label="最大内容大小 (字节)"
                    name="max_content_size"
                    type="number"
                    value={config.max_content_size}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving}
                    inputProps={{ min: 1024 }}
                    helperText="单个内容项的最大大小"
                  />
                </Grid>
                <Grid item xs={12} md={4}>
                  <TextField
                    fullWidth
                    label="默认页面大小"
                    name="default_page_size"
                    type="number"
                    value={config.default_page_size}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving}
                    inputProps={{ min: 1, max: 100 }}
                    helperText="分页查询的默认页面大小"
                  />
                </Grid>
                <Grid item xs={12} md={4}>
                  <TextField
                    fullWidth
                    label="最大页面大小"
                    name="max_page_size"
                    type="number"
                    value={config.max_page_size}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving}
                    inputProps={{ min: 1, max: 1000 }}
                    helperText="分页查询的最大页面大小"
                  />
                </Grid>
              </Grid>
            </AccordionDetails>
          </Accordion>

          {/* CORS配置 */}
          <Accordion>
            <AccordionSummary expandIcon={<ExpandMoreIcon />}>
              <Typography variant="h6">CORS配置</Typography>
            </AccordionSummary>
            <AccordionDetails>
              <Grid container spacing={2}>
                <Grid item xs={12}>
                  <FormControlLabel
                    control={
                      <Switch
                        checked={config.enable_cors}
                        onChange={handleInputChange}
                        name="enable_cors"
                        disabled={saving}
                      />
                    }
                    label="启用CORS"
                  />
                </Grid>
                <Grid item xs={12}>
                  <TextField
                    fullWidth
                    label="CORS允许的源"
                    name="cors_origin"
                    value={config.cors_origin}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving || !config.enable_cors}
                    helperText="允许跨域访问的源，*表示允许所有源"
                  />
                </Grid>
              </Grid>
            </AccordionDetails>
          </Accordion>

          {/* 静态文件配置 */}
          <Accordion>
            <AccordionSummary expandIcon={<ExpandMoreIcon />}>
              <Typography variant="h6">静态文件配置</Typography>
            </AccordionSummary>
            <AccordionDetails>
              <Grid container spacing={2}>
                <Grid item xs={12}>
                  <FormControlLabel
                    control={
                      <Switch
                        checked={config.enable_static_files}
                        onChange={handleInputChange}
                        name="enable_static_files"
                        disabled={saving}
                      />
                    }
                    label="启用静态文件服务"
                  />
                </Grid>
                <Grid item xs={12}>
                  <TextField
                    fullWidth
                    label="静态文件路径"
                    name="static_files_path"
                    value={config.static_files_path}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving || !config.enable_static_files}
                    helperText="静态文件的根目录路径"
                  />
                </Grid>
              </Grid>
            </AccordionDetails>
          </Accordion>

          {/* 文件上传配置 */}
          <Accordion>
            <AccordionSummary expandIcon={<ExpandMoreIcon />}>
              <Typography variant="h6">文件上传配置</Typography>
            </AccordionSummary>
            <AccordionDetails>
              <Grid container spacing={2}>
                <Grid item xs={12}>
                  <FormControlLabel
                    control={
                      <Switch
                        checked={config.enable_file_upload}
                        onChange={handleInputChange}
                        name="enable_file_upload"
                        disabled={saving}
                      />
                    }
                    label="启用文件上传"
                  />
                </Grid>
                <Grid item xs={12} md={6}>
                  <TextField
                    fullWidth
                    label="上传文件路径"
                    name="upload_path"
                    value={config.upload_path}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving || !config.enable_file_upload}
                    helperText="上传文件的保存目录"
                  />
                </Grid>
                <Grid item xs={12} md={6}>
                  <TextField
                    fullWidth
                    label="最大文件大小 (字节)"
                    name="max_file_size"
                    type="number"
                    value={config.max_file_size}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving || !config.enable_file_upload}
                    inputProps={{ min: 1024 }}
                    helperText="单个文件的最大大小"
                  />
                </Grid>
                <Grid item xs={12}>
                  <TextField
                    fullWidth
                    label="允许的文件类型"
                    name="allowed_file_types"
                    value={config.allowed_file_types.join(', ')}
                    onChange={handleFileTypesChange}
                    margin="normal"
                    disabled={saving || !config.enable_file_upload}
                    helperText="允许上传的文件扩展名，用逗号分隔，如：.txt, .pdf, .jpg"
                  />
                </Grid>
              </Grid>
            </AccordionDetails>
          </Accordion>

          {/* LLaMA配置 */}
          <Accordion>
            <AccordionSummary expandIcon={<ExpandMoreIcon />}>
              <Typography variant="h6">LLaMA AI配置</Typography>
            </AccordionSummary>
            <AccordionDetails>
              <Grid container spacing={2}>
                <Grid item xs={12}>
                  <FormControlLabel
                    control={
                      <Switch
                        checked={config.enable_llama}
                        onChange={handleInputChange}
                        name="enable_llama"
                        disabled={saving}
                      />
                    }
                    label="启用LLaMA AI功能"
                  />
                </Grid>
                <Grid item xs={12} md={6}>
                  <TextField
                    fullWidth
                    label="LLaMA模型路径"
                    name="llama_model_path"
                    value={config.llama_model_path}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving || !config.enable_llama}
                    helperText="LLaMA模型文件的路径"
                  />
                </Grid>
                <Grid item xs={12} md={6}>
                  <TextField
                    fullWidth
                    label="LLaMA可执行文件路径"
                    name="llama_executable_path"
                    value={config.llama_executable_path}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving || !config.enable_llama}
                    helperText="LLaMA可执行文件的路径"
                  />
                </Grid>
                <Grid item xs={12} md={3}>
                  <TextField
                    fullWidth
                    label="上下文大小"
                    name="llama_context_size"
                    type="number"
                    value={config.llama_context_size}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving || !config.enable_llama}
                    inputProps={{ min: 512, max: 8192 }}
                    helperText="LLaMA的上下文窗口大小"
                  />
                </Grid>
                <Grid item xs={12} md={3}>
                  <TextField
                    fullWidth
                    label="线程数"
                    name="llama_threads"
                    type="number"
                    value={config.llama_threads}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving || !config.enable_llama}
                    inputProps={{ min: 1, max: 32 }}
                    helperText="LLaMA使用的线程数"
                  />
                </Grid>
                <Grid item xs={12} md={3}>
                  <TextField
                    fullWidth
                    label="温度"
                    name="llama_temperature"
                    type="number"
                    value={Math.round(config.llama_temperature * 100) / 100}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving || !config.enable_llama}
                    inputProps={{ min: 0, max: 2, step: 0.1 }}
                    helperText="生成文本的随机性，0-2"
                  />
                </Grid>
                <Grid item xs={12} md={3}>
                  <TextField
                    fullWidth
                    label="最大令牌数"
                    name="llama_max_tokens"
                    type="number"
                    value={config.llama_max_tokens}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving || !config.enable_llama}
                    inputProps={{ min: 1, max: 2048 }}
                    helperText="生成文本的最大令牌数"
                  />
                </Grid>
              </Grid>
            </AccordionDetails>
          </Accordion>

          {/* Ollama配置 */}
          <Accordion>
            <AccordionSummary expandIcon={<ExpandMoreIcon />}>
              <Typography variant="h6">Ollama AI配置</Typography>
            </AccordionSummary>
            <AccordionDetails>
              <Grid container spacing={2}>
                <Grid item xs={12}>
                  <FormControlLabel
                    control={
                      <Switch
                        checked={config.enable_ollama}
                        onChange={handleInputChange}
                        name="enable_ollama"
                        disabled={saving}
                      />
                    }
                    label="启用Ollama AI功能"
                  />
                </Grid>
                <Grid item xs={12} md={6}>
                  <TextField
                    fullWidth
                    label="Ollama主机地址"
                    name="ollama_host"
                    value={config.ollama_host}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving || !config.enable_ollama}
                    helperText="Ollama服务器的主机地址"
                  />
                </Grid>
                <Grid item xs={12} md={6}>
                  <TextField
                    fullWidth
                    label="Ollama端口号"
                    name="ollama_port"
                    type="number"
                    value={config.ollama_port}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving || !config.enable_ollama}
                    inputProps={{ min: 1, max: 65535 }}
                    helperText="Ollama服务器的端口号，默认11434"
                  />
                </Grid>
                <Grid item xs={12} md={6}>
                  <TextField
                    fullWidth
                    label="Ollama模型名称"
                    name="ollama_model"
                    value={config.ollama_model}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving || !config.enable_ollama}
                    helperText="要使用的Ollama模型名称，如llama2、codellama等"
                  />
                </Grid>
                <Grid item xs={12} md={6}>
                  <TextField
                    fullWidth
                    label="超时时间 (秒)"
                    name="ollama_timeout"
                    type="number"
                    value={config.ollama_timeout}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving || !config.enable_ollama}
                    inputProps={{ min: 5, max: 300 }}
                    helperText="请求超时时间，5-300秒"
                  />
                </Grid>
                <Grid item xs={12} md={6}>
                  <TextField
                    fullWidth
                    label="温度"
                    name="ollama_temperature"
                    type="number"
                    value={Math.round(config.ollama_temperature * 100) / 100}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving || !config.enable_ollama}
                    inputProps={{ min: 0, max: 2, step: 0.1 }}
                    helperText="生成文本的随机性，0-2"
                  />
                </Grid>
                <Grid item xs={12} md={6}>
                  <TextField
                    fullWidth
                    label="最大令牌数"
                    name="ollama_max_tokens"
                    type="number"
                    value={config.ollama_max_tokens}
                    onChange={handleInputChange}
                    margin="normal"
                    disabled={saving || !config.enable_ollama}
                    inputProps={{ min: 1, max: 4096 }}
                    helperText="生成文本的最大令牌数"
                  />
                </Grid>
              </Grid>
            </AccordionDetails>
          </Accordion>

          {error && (
            <Alert severity="error" sx={{ mt: 2 }}>
              {error}
            </Alert>
          )}

          <Box display="flex" gap={2} mt={3}>
            <Button
              type="submit"
              variant="contained"
              color="primary"
              startIcon={<SaveIcon />}
              disabled={saving}
            >
              {saving ? '保存中...' : '保存配置'}
            </Button>
          </Box>
        </Box>
      </Paper>

      <Snackbar
        open={snackbarOpen}
        autoHideDuration={6000}
        onClose={() => setSnackbarOpen(false)}
      >
        <Alert severity={snackbarSeverity}>{snackbarMessage}</Alert>
      </Snackbar>
    </Box>
  );
};

export default ConfigView;