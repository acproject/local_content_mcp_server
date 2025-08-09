import React, { useState, useEffect } from 'react';
import { Link, useNavigate } from 'react-router-dom';
import {
  Card,
  CardContent,
  Typography,
  Grid,
  Chip,
  Box,
  Pagination,
  Button,
  IconButton,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Alert,
  Snackbar,
} from '@mui/material';
import {
  Edit as EditIcon,
  Delete as DeleteIcon,
  Visibility as ViewIcon,
} from '@mui/icons-material';

import { ContentAPI, ContentItem, ContentListResponse } from '../services/api';

const ContentList = () => {
  const [contentList, setContentList] = useState<ContentListResponse>({ 
    content: [], 
    page: 1, 
    page_size: 10, 
    total_count: 0, 
    total_pages: 0 
  });
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const [page, setPage] = useState(1);
  const [deleteDialogOpen, setDeleteDialogOpen] = useState(false);
  const [itemToDelete, setItemToDelete] = useState<ContentItem | null>(null);
  const [snackbarOpen, setSnackbarOpen] = useState(false);
  const [snackbarMessage, setSnackbarMessage] = useState('');
  
  const navigate = useNavigate();
  const pageSize = 6;

  const fetchContent = async (currentPage: number) => {
    console.log('=== fetchContent called ===');
    console.log('Page:', currentPage);
    console.log('PageSize:', pageSize);
    
    try {
      setLoading(true);
      setError(null);
      
      console.log('Making API call to ContentAPI.listContent...');
      const data = await ContentAPI.listContent(currentPage, pageSize);
      
      console.log('=== API Response ===');
      console.log('Full response:', JSON.stringify(data, null, 2));
      console.log('Response type:', typeof data);
      console.log('Has content property:', 'content' in data);
      console.log('Content array:', data?.content);
      console.log('Content array type:', typeof data?.content);
      console.log('Content array length:', data?.content?.length);
      console.log('Total pages:', data?.total_pages);
      console.log('Total count:', data?.total_count);
      
      setContentList(data);
      
      console.log('=== State Updated ===');
      console.log('ContentList state will be set to:', data);
      
    } catch (err: any) {
      console.error('=== Error in fetchContent ===');
      console.error('Error object:', err);
      console.error('Error message:', err.message);
      console.error('Error stack:', err.stack);
      if (err.response) {
        console.error('Response status:', err.response.status);
        console.error('Response data:', err.response.data);
      }
      setError(err.message || '获取内容失败');
    } finally {
      setLoading(false);
      console.log('=== fetchContent completed ===');
    }
  };

  useEffect(() => {
    console.log('ContentList useEffect triggered, page:', page);
    fetchContent(page);
  }, [page]);

  // 添加渲染时的调试信息
  console.log('ContentList render - loading:', loading, 'error:', error, 'contentList:', contentList);
  
  // 将API暴露到全局，方便调试
  React.useEffect(() => {
    (window as any).testAPI = async () => {
      try {
        console.log('=== Manual API Test ===');
        const result = await ContentAPI.listContent(1, 10);
        console.log('Manual API result:', result);
        return result;
      } catch (err) {
        console.error('Manual API error:', err);
        throw err;
      }
    };
  }, []);



  const handlePageChange = (event: React.ChangeEvent<unknown>, value: number) => {
    setPage(value);
  };

  const handleDeleteClick = (item: ContentItem) => {
    setItemToDelete(item);
    setDeleteDialogOpen(true);
  };

  const handleDeleteConfirm = async () => {
    if (itemToDelete) {
      try {
        await ContentAPI.deleteContent(itemToDelete.id);
        setSnackbarMessage('内容删除成功');
        setSnackbarOpen(true);
        fetchContent(page);
      } catch (err) {
        setSnackbarMessage('删除失败');
        setSnackbarOpen(true);
        console.error('Error deleting content:', err);
      }
    }
    setDeleteDialogOpen(false);
    setItemToDelete(null);
  };

  const formatDate = (dateString: string) => {
    // 后端返回的是Unix时间戳，需要转换为毫秒
    const timestamp = parseInt(dateString) * 1000;
    return new Date(timestamp).toLocaleString('zh-CN');
  };

  const formatTags = (tags: string) => {
    return tags.split(',').filter(tag => tag.trim());
  };





  if (loading) {
    return (
      <Box display="flex" justifyContent="center" alignItems="center" minHeight="200px">
        <Typography>加载中...</Typography>
      </Box>
    );
  }

  if (error) {
    return (
      <Box display="flex" justifyContent="center" alignItems="center" minHeight="200px">
        <Alert severity="error">{error}</Alert>
      </Box>
    );
  }

  return (
    <Box>
      <Box display="flex" justifyContent="space-between" alignItems="center" mb={3}>
        <Typography variant="h4" component="h1">
          内容列表
        </Typography>
        <Box sx={{ display: 'flex', gap: 2 }}>
          <Button
            variant="contained"
            color="primary"
            component={Link}
            to="/add"
            startIcon={<EditIcon />}
          >
            创建新内容
          </Button>
          <Button
            variant="outlined"
            color="secondary"
            onClick={() => {
              console.log('Manual refresh clicked');
              fetchContent(page);
            }}
          >
            刷新数据
          </Button>
        </Box>
      </Box>

      {(!contentList?.content || contentList.content.length === 0) ? (
        <Box display="flex" justifyContent="center" alignItems="center" minHeight="200px">
          <Typography variant="h6" color="textSecondary">
            暂无内容
          </Typography>
        </Box>
      ) : (
        <>
          <Grid container spacing={3}>
            {contentList.content.map((item) => (
              <Grid item xs={12} md={6} key={item.id}>
                <Card sx={{ height: '100%', display: 'flex', flexDirection: 'column' }}>
                  <CardContent sx={{ flexGrow: 1 }}>
                    <Typography variant="h6" component="h2" gutterBottom>
                      {item.title}
                    </Typography>
                    <Typography
                      variant="body2"
                      color="textSecondary"
                      sx={{
                        mb: 2,
                        overflow: 'hidden',
                        textOverflow: 'ellipsis',
                        display: '-webkit-box',
                        WebkitLineClamp: 3,
                        WebkitBoxOrient: 'vertical',
                      }}
                    >
                      {item.content}
                    </Typography>
                    <Box mb={2}>
                      {formatTags(item.tags).map((tag, index) => (
                        <Chip
                          key={index}
                          label={tag}
                          size="small"
                          sx={{ mr: 0.5, mb: 0.5 }}
                        />
                      ))}
                    </Box>
                    <Typography variant="caption" color="textSecondary">
                      创建时间: {formatDate(item.created_at)}
                    </Typography>
                    {item.updated_at !== item.created_at && (
                      <Typography variant="caption" color="textSecondary" display="block">
                        更新时间: {formatDate(item.updated_at)}
                      </Typography>
                    )}
                  </CardContent>
                  <Box p={1} display="flex" justifyContent="flex-end">
                    <IconButton
                      size="small"
                      onClick={() => navigate(`/content/${item.id}`)}
                      title="查看详情"
                    >
                      <ViewIcon />
                    </IconButton>
                    <IconButton
                      size="small"
                      onClick={() => navigate(`/edit/${item.id}`)}
                      title="编辑"
                    >
                      <EditIcon />
                    </IconButton>
                    <IconButton
                      size="small"
                      onClick={() => handleDeleteClick(item)}
                      title="删除"
                      color="error"
                    >
                      <DeleteIcon />
                    </IconButton>
                  </Box>
                </Card>
              </Grid>
            ))}
          </Grid>

          {(contentList?.total_pages || 0) > 1 && (
            <Box display="flex" justifyContent="center" mt={4}>
              <Pagination
                count={contentList?.total_pages || 0}
                page={page}
                onChange={handlePageChange}
                color="primary"
                size="large"
              />
            </Box>
          )}
        </>
      )}

      <Dialog open={deleteDialogOpen} onClose={() => setDeleteDialogOpen(false)}>
        <DialogTitle>确认删除</DialogTitle>
        <DialogContent>
          <Typography>
            确定要删除内容 "{itemToDelete?.title}" 吗？此操作无法撤销。
          </Typography>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setDeleteDialogOpen(false)}>取消</Button>
          <Button onClick={handleDeleteConfirm} color="error" variant="contained">
            删除
          </Button>
        </DialogActions>
      </Dialog>

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

export default ContentList;