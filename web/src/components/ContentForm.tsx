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
} from '@mui/material';
import { Save as SaveIcon, Cancel as CancelIcon } from '@mui/icons-material';

import { ContentAPI, CreateContentRequest, UpdateContentRequest } from '../services/api';

const ContentForm: React.FC = (): JSX.Element => {
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
        <Box component="form" onSubmit={handleSubmit}>
          <TextField
            fullWidth
            label="标题"
            name="title"
            value={formData.title}
            onChange={handleInputChange}
            margin="normal"
            required
            disabled={loading}
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
            disabled={loading}
          />

          <FormControl fullWidth margin="normal">
            <InputLabel>内容类型</InputLabel>
            <Select
              value={formData.content_type}
              onChange={handleSelectChange}
              label="内容类型"
              disabled={loading}
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
            disabled={loading}
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
              disabled={loading}
            >
              {loading ? '保存中...' : (isEdit ? '更新' : '创建')}
            </Button>
            <Button
              variant="outlined"
              onClick={handleCancel}
              startIcon={<CancelIcon />}
              disabled={loading}
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