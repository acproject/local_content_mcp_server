import React, { useState, useEffect } from 'react';
import { useNavigate, useParams } from 'react-router-dom';
import {
  Paper,
  TextField,
  Button,
  Typography,
  Box,
  FormControl,
  InputLabel,
  Select,
  MenuItem,
  Alert,
  Snackbar,
  Divider,
  CircularProgress,
  LinearProgress,
} from '@mui/material';
import { 
  Save as SaveIcon, 
  Cancel as CancelIcon, 
  Upload as UploadIcon,
  Description as DocumentIcon 
} from '@mui/icons-material';

import { ContentAPI, CreateContentRequest, UpdateContentRequest } from '../services/api';

const ContentForm = () => {
  const { id } = useParams<{ id: string }>();
  const navigate = useNavigate();
  const isEdit = Boolean(id);

  const [formData, setFormData] = useState({
    title: '',
    content: '',
    content_type: 'text',
    tags: '',
  });
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [snackbarOpen, setSnackbarOpen] = useState(false);
  const [snackbarMessage, setSnackbarMessage] = useState('');
  
  // 文件上传相关状态
  const [selectedFile, setSelectedFile] = useState<File | null>(null);
  const [uploading, setUploading] = useState(false);
  const [parsing, setParsing] = useState(false);
  const [uploadProgress, setUploadProgress] = useState(0);

  useEffect(() => {
    if (isEdit && id) {
      fetchContent(parseInt(id));
    }
  }, [isEdit, id]);

  const fetchContent = async (contentId: number) => {
    try {
      setLoading(true);
      const content = await ContentAPI.getContent(contentId);
      setFormData({
        title: content.title,
        content: content.content,
        content_type: content.content_type,
        tags: content.tags,
      });
      setError(null);
    } catch (err) {
      setError('获取内容失败');
      console.error('Error fetching content:', err);
    } finally {
      setLoading(false);
    }
  };

  const handleInputChange = (event: React.ChangeEvent<HTMLInputElement | HTMLTextAreaElement>) => {
    const { name, value } = event.target;
    setFormData(prev => ({
      ...prev,
      [name]: value,
    }));
  };

  const handleSelectChange = (event: any) => {
    setFormData(prev => ({
      ...prev,
      content_type: event.target.value,
    }));
  };

  const handleSubmit = async (event: React.FormEvent) => {
    event.preventDefault();
    
    if (!formData.title.trim() || !formData.content.trim()) {
      setError('标题和内容不能为空');
      return;
    }

    try {
      setLoading(true);
      setError(null);

      if (isEdit && id) {
        const updateData: UpdateContentRequest = {
          title: formData.title.trim(),
          content: formData.content.trim(),
          content_type: formData.content_type,
          tags: formData.tags.trim(),
        };
        await ContentAPI.updateContent(parseInt(id), updateData);
        setSnackbarMessage('内容更新成功');
      } else {
        const createData: CreateContentRequest = {
          title: formData.title.trim(),
          content: formData.content.trim(),
          content_type: formData.content_type,
          tags: formData.tags.trim(),
        };
        await ContentAPI.createContent(createData);
        setSnackbarMessage('内容创建成功');
      }
      
      setSnackbarOpen(true);
      setTimeout(() => {
        navigate('/');
      }, 1000);
    } catch (err) {
      setError(isEdit ? '更新内容失败' : '创建内容失败');
      console.error('Error saving content:', err);
    } finally {
      setLoading(false);
    }
  };

  const handleCancel = () => {
    navigate('/');
  };

  // 文件处理函数
  const handleFileSelect = (event: React.ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    if (file) {
      setSelectedFile(file);
      setError(null);
    }
  };

  const handleFileUpload = async () => {
    if (!selectedFile) {
      setError('请先选择文件');
      return;
    }

    try {
      setUploading(true);
      setUploadProgress(0);
      setError(null);

      // 模拟上传进度
      const progressInterval = setInterval(() => {
        setUploadProgress(prev => {
          if (prev >= 90) {
            clearInterval(progressInterval);
            return 90;
          }
          return prev + 10;
        });
      }, 200);

      // 上传文件
      const uploadResult = await ContentAPI.uploadFile(selectedFile);
      setUploadProgress(100);
      clearInterval(progressInterval);
      
      setSnackbarMessage('文件上传成功，正在解析...');
      setSnackbarOpen(true);
      
      // 解析文档
      setParsing(true);
      const parseResult = await ContentAPI.parseDocument(uploadResult.file_path);
      
      // 填充表单数据
      setFormData({
        title: parseResult.title || selectedFile.name,
        content: parseResult.content || '',
        content_type: parseResult.content_type || 'text',
        tags: parseResult.tags || '',
      });
      
      setSnackbarMessage('文档解析完成，内容已自动填充');
      setSnackbarOpen(true);
      
    } catch (err) {
      setError('文件上传或解析失败');
      console.error('Error uploading/parsing file:', err);
    } finally {
      setUploading(false);
      setParsing(false);
      setUploadProgress(0);
    }
  };

  const clearFile = () => {
    setSelectedFile(null);
    setUploadProgress(0);
  };

  if (loading && isEdit) {
    return (
      <Box display="flex" justifyContent="center" mt={4}>
        <Typography>加载中...</Typography>
      </Box>
    );
  }

  return (
    <Box>
      <Typography variant="h4" component="h1" gutterBottom>
        {isEdit ? '编辑内容' : '创建新内容'}
      </Typography>

      <Paper elevation={3} sx={{ p: 3, mt: 3 }}>
        {/* 文件上传区域 */}
        {!isEdit && (
          <Box sx={{ mb: 3 }}>
            <Typography variant="h6" gutterBottom>
              文档上传
            </Typography>
            <Typography variant="body2" color="text.secondary" sx={{ mb: 2 }}>
              上传文档文件，系统将自动解析内容并填充表单
            </Typography>
            
            <Box sx={{ display: 'flex', alignItems: 'center', gap: 2, mb: 2 }}>
               <input
                 accept=".txt,.md,.pdf,.doc,.docx"
                 style={{ display: 'none' }}
                 id="file-upload"
                 type="file"
                 onChange={handleFileSelect}
                 disabled={uploading || parsing}
               />
               <Box component="label" htmlFor="file-upload">
                <Button
                  variant="outlined"
                  component="span"
                  startIcon={<DocumentIcon />}
                  disabled={uploading || parsing}
                >
                  选择文件
                </Button>
              </Box>
              
              {selectedFile && (
                <Typography variant="body2">
                  {selectedFile.name} ({(selectedFile.size / 1024 / 1024).toFixed(2)} MB)
                </Typography>
              )}
              
              {selectedFile && (
                <Button
                  variant="contained"
                  onClick={handleFileUpload}
                  startIcon={uploading || parsing ? <CircularProgress size={16} /> : <UploadIcon />}
                  disabled={uploading || parsing}
                >
                  {uploading ? '上传中...' : parsing ? '解析中...' : '上传并解析'}
                </Button>
              )}
              
              {selectedFile && !uploading && !parsing && (
                <Button
                  variant="text"
                  onClick={clearFile}
                  size="small"
                >
                  清除
                </Button>
              )}
            </Box>
            
            {uploading && (
              <Box sx={{ mb: 2 }}>
                <Typography variant="body2" sx={{ mb: 1 }}>上传进度: {uploadProgress}%</Typography>
                <LinearProgress variant="determinate" value={uploadProgress} />
              </Box>
            )}
            
            {parsing && (
              <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 2 }}>
                <CircularProgress size={16} />
                <Typography variant="body2">正在使用AI解析文档内容...</Typography>
              </Box>
            )}
            
            <Divider sx={{ my: 3 }} />
          </Box>
        )}
        
        <Box component="form" onSubmit={handleSubmit}>
          <TextField
            fullWidth
            label="标题"
            name="title"
            value={formData.title}
            onChange={handleInputChange}
            margin="normal"
            required
            disabled={loading || uploading || parsing}
          />

          <TextField
            fullWidth
            label="内容"
            name="content"
            value={formData.content}
            onChange={handleInputChange}
            margin="normal"
            multiline
            rows={8}
            required
            disabled={loading || uploading || parsing}
          />

          <FormControl fullWidth margin="normal">
            <InputLabel>内容类型</InputLabel>
            <Select
              value={formData.content_type}
              onChange={handleSelectChange}
              label="内容类型"
              disabled={loading || uploading || parsing}
            >
              <MenuItem value="text">文本</MenuItem>
              <MenuItem value="markdown">Markdown</MenuItem>
              <MenuItem value="html">HTML</MenuItem>
              <MenuItem value="code">代码</MenuItem>
            </Select>
          </FormControl>

          <TextField
            fullWidth
            label="标签（用逗号分隔）"
            name="tags"
            value={formData.tags}
            onChange={handleInputChange}
            margin="normal"
            placeholder="例如：技术,编程,笔记"
            disabled={loading || uploading || parsing}
          />

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
              disabled={loading || uploading || parsing}
            >
              {loading ? '保存中...' : (isEdit ? '更新' : '创建')}
            </Button>
            <Button
              variant="outlined"
              onClick={handleCancel}
              startIcon={<CancelIcon />}
              disabled={loading || uploading || parsing}
            >
              取消
            </Button>
          </Box>
        </Box>
      </Paper>

      <Snackbar
        open={snackbarOpen}
        autoHideDuration={3000}
        onClose={() => setSnackbarOpen(false)}
      >
        <Alert severity="success">{snackbarMessage}</Alert>
      </Snackbar>
    </Box>
  );
};

export default ContentForm;