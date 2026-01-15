import React, { useState, useEffect } from 'react';
import { useParams, useNavigate, Link } from 'react-router-dom';
import {
  Paper,
  Typography,
  Box,
  Chip,
  Button,
  Alert,
  Snackbar,
  Divider,
  IconButton,
} from '@mui/material';
import type { AlertColor } from '@mui/material';
import {
  Edit as EditIcon,
  Delete as DeleteIcon,
  ArrowBack as ArrowBackIcon,
  FileDownload as FileDownloadIcon,
} from '@mui/icons-material';

import { ContentAPI, ContentItem } from '../services/api';

const ContentDetail: React.FC = () => {
  const { id } = useParams<{ id: string }>();
  const navigate = useNavigate();
  const [content, setContent] = useState<ContentItem | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [exporting, setExporting] = useState(false);
  const [snackbarOpen, setSnackbarOpen] = useState(false);
  const [snackbarMessage, setSnackbarMessage] = useState('');
  const [snackbarSeverity, setSnackbarSeverity] = useState<AlertColor>('success');

  useEffect(() => {
    if (id) {
      fetchContent(parseInt(id));
    }
  }, [id]);

  const fetchContent = async (contentId: number) => {
    try {
      setLoading(true);
      const data = await ContentAPI.getContent(contentId);
      setContent(data);
      setError(null);
    } catch (err) {
      setError('获取内容失败');
      console.error('Error fetching content:', err);
    } finally {
      setLoading(false);
    }
  };

  const handleDelete = async () => {
    if (content && window.confirm(`确定要删除内容 "${content.title}" 吗？`)) {
      try {
        await ContentAPI.deleteContent(content.id);
        navigate('/');
      } catch (err) {
        setError('删除失败');
        console.error('Error deleting content:', err);
      }
    }
  };

  const handleExport = async () => {
    if (!content) return;
    try {
      setExporting(true);
      const { blob, filename } = await ContentAPI.exportContentFile(content.id);
      const url = window.URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = filename;
      document.body.appendChild(a);
      a.click();
      a.remove();
      window.URL.revokeObjectURL(url);

      setSnackbarSeverity('success');
      setSnackbarMessage('已开始下载');
      setSnackbarOpen(true);
    } catch (err) {
      console.error('Error exporting content:', err);
      setSnackbarSeverity('error');
      setSnackbarMessage('导出失败');
      setSnackbarOpen(true);
    } finally {
      setExporting(false);
    }
  };

  const formatDate = (dateString: string) => {
    return new Date(dateString).toLocaleString('zh-CN');
  };

  const formatTags = (tags: string) => {
    return tags.split(',').filter(tag => tag.trim());
  };

  const renderContent = (content: string, contentType: string) => {
    switch (contentType) {
      case 'markdown':
        return (
          <Typography
            component="pre"
            sx={{
              whiteSpace: 'pre-wrap',
              fontFamily: 'monospace',
              backgroundColor: '#f5f5f5',
              p: 2,
              borderRadius: 1,
            }}
          >
            {content}
          </Typography>
        );
      case 'code':
        return (
          <Typography
            component="pre"
            sx={{
              whiteSpace: 'pre-wrap',
              fontFamily: 'monospace',
              backgroundColor: '#1e1e1e',
              color: '#d4d4d4',
              p: 2,
              borderRadius: 1,
              overflow: 'auto',
            }}
          >
            {content}
          </Typography>
        );
      case 'html':
        return (
          <Box
            dangerouslySetInnerHTML={{ __html: content }}
            sx={{
              '& *': {
                maxWidth: '100%',
              },
            }}
          />
        );
      default:
        return (
          <Typography
            sx={{
              whiteSpace: 'pre-wrap',
              lineHeight: 1.6,
            }}
          >
            {content}
          </Typography>
        );
    }
  };

  if (loading) {
    return (
      <Box display="flex" justifyContent="center" mt={4}>
        <Typography>加载中...</Typography>
      </Box>
    );
  }

  if (error) {
    return (
      <Box>
        <Button
          startIcon={<ArrowBackIcon />}
          onClick={() => navigate('/')}
          sx={{ mb: 2 }}
        >
          返回列表
        </Button>
        <Alert severity="error">{error}</Alert>
      </Box>
    );
  }

  if (!content) {
    return (
      <Box>
        <Button
          startIcon={<ArrowBackIcon />}
          onClick={() => navigate('/')}
          sx={{ mb: 2 }}
        >
          返回列表
        </Button>
        <Alert severity="info">内容不存在</Alert>
      </Box>
    );
  }

  return (
    <Box>
      <Box display="flex" justifyContent="space-between" alignItems="center" mb={3}>
        <Button
          startIcon={<ArrowBackIcon />}
          onClick={() => navigate('/')}
        >
          返回列表
        </Button>
        <Box>
          <IconButton
            onClick={handleExport}
            color="primary"
            title="导出"
            disabled={exporting}
          >
            <FileDownloadIcon />
          </IconButton>
          <IconButton
            component={Link}
            to={`/edit/${content.id}`}
            color="primary"
            title="编辑"
          >
            <EditIcon />
          </IconButton>
          <IconButton
            onClick={handleDelete}
            color="error"
            title="删除"
          >
            <DeleteIcon />
          </IconButton>
        </Box>
      </Box>

      <Paper elevation={3} sx={{ p: 3 }}>
        <Typography variant="h4" component="h1" gutterBottom>
          {content.title}
        </Typography>

        <Box mb={3}>
          <Typography variant="body2" color="textSecondary" gutterBottom>
            内容类型: {content.content_type}
          </Typography>
          <Typography variant="body2" color="textSecondary" gutterBottom>
            创建时间: {formatDate(content.created_at)}
          </Typography>
          {content.updated_at !== content.created_at && (
            <Typography variant="body2" color="textSecondary" gutterBottom>
              更新时间: {formatDate(content.updated_at)}
            </Typography>
          )}
        </Box>

        {content.tags && (
          <Box mb={3}>
            <Typography variant="subtitle2" gutterBottom>
              标签:
            </Typography>
            <Box>
              {formatTags(content.tags).map((tag, index) => (
                <Chip
                  key={index}
                  label={tag}
                  size="small"
                  sx={{ mr: 1, mb: 1 }}
                />
              ))}
            </Box>
          </Box>
        )}

        <Divider sx={{ my: 3 }} />

        <Typography variant="subtitle2" gutterBottom>
          内容:
        </Typography>
        <Box mt={2}>
          {renderContent(content.content, content.content_type)}
        </Box>
      </Paper>

      <Snackbar
        open={snackbarOpen}
        autoHideDuration={3000}
        onClose={() => setSnackbarOpen(false)}
      >
        <Alert severity={snackbarSeverity}>{snackbarMessage}</Alert>
      </Snackbar>
    </Box>
  );
};

export default ContentDetail;
