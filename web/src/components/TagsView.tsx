import React, { useState, useEffect } from 'react';
import { Link } from 'react-router-dom';
import {
  Paper,
  Typography,
  Box,
  Chip,
  Card,
  CardContent,
  Alert,
  Grid,
  IconButton,
  Collapse,
} from '@mui/material';
import {
  Visibility as ViewIcon,
  ExpandMore as ExpandMoreIcon,
  ExpandLess as ExpandLessIcon,
} from '@mui/icons-material';

import { ContentAPI, ContentItem } from '../services/api';

const TagsView: React.FC = () => {
  const [tags, setTags] = useState<string[]>([]);
  const [selectedTag, setSelectedTag] = useState<string | null>(null);
  const [tagContent, setTagContent] = useState<ContentItem[]>([]);
  const [loading, setLoading] = useState(true);
  const [contentLoading, setContentLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [expandedCards, setExpandedCards] = useState<Set<number>>(new Set());

  useEffect(() => {
    fetchTags();
  }, []);

  const fetchTags = async () => {
    try {
      setLoading(true);
      const data = await ContentAPI.getTags();
      // 确保 data 是数组
      if (Array.isArray(data)) {
        setTags(data);
      } else {
        setTags([]);
        console.warn('Tags data is not an array:', data);
      }
      setError(null);
    } catch (err) {
      setError('获取标签失败');
      setTags([]); // 设置为空数组以防止 map 错误
      console.error('Error fetching tags:', err);
    } finally {
      setLoading(false);
    }
  };

  const fetchContentByTag = async (tag: string) => {
    try {
      setContentLoading(true);
      const data = await ContentAPI.getContentByTag(tag);
      setTagContent(data);
      setSelectedTag(tag);
      setError(null);
    } catch (err) {
      setError(`获取标签 "${tag}" 的内容失败`);
      console.error('Error fetching content by tag:', err);
    } finally {
      setContentLoading(false);
    }
  };

  const handleTagClick = (tag: string) => {
    if (selectedTag === tag) {
      setSelectedTag(null);
      setTagContent([]);
    } else {
      fetchContentByTag(tag);
    }
  };

  const toggleCardExpansion = (id: number) => {
    const newExpanded = new Set(expandedCards);
    if (newExpanded.has(id)) {
      newExpanded.delete(id);
    } else {
      newExpanded.add(id);
    }
    setExpandedCards(newExpanded);
  };

  const formatDate = (dateString: string) => {
    return new Date(dateString).toLocaleString('zh-CN');
  };

  const formatTags = (tags: string) => {
    return tags.split(',').filter(tag => tag.trim());
  };

  const truncateText = (text: string, maxLength: number = 200) => {
    if (text.length <= maxLength) return text;
    return text.substring(0, maxLength) + '...';
  };

  if (loading) {
    return (
      <Box display="flex" justifyContent="center" mt={4}>
        <Typography>加载中...</Typography>
      </Box>
    );
  }

  if (error && !selectedTag) {
    return (
      <Box display="flex" justifyContent="center" mt={4}>
        <Alert severity="error">{error}</Alert>
      </Box>
    );
  }

  return (
    <Box>
      <Typography variant="h4" component="h1" gutterBottom>
        标签管理
      </Typography>

      <Paper elevation={3} sx={{ p: 3, mb: 3 }}>
        <Typography variant="h6" gutterBottom>
          所有标签 ({tags.length} 个)
        </Typography>
        <Typography variant="body2" color="textSecondary" gutterBottom>
          点击标签查看相关内容
        </Typography>
        <Box mt={2}>
          {tags.length === 0 ? (
            <Typography color="textSecondary">
              暂无标签
            </Typography>
          ) : (
            tags.map((tag, index) => (
              <Chip
                key={index}
                label={tag}
                onClick={() => handleTagClick(tag)}
                color={selectedTag === tag ? 'primary' : 'default'}
                variant={selectedTag === tag ? 'filled' : 'outlined'}
                sx={{
                  mr: 1,
                  mb: 1,
                  cursor: 'pointer',
                  '&:hover': {
                    backgroundColor: selectedTag === tag ? undefined : 'action.hover',
                  },
                }}
              />
            ))
          )}
        </Box>
      </Paper>

      {selectedTag && (
        <Box>
          <Typography variant="h6" gutterBottom>
            标签 "{selectedTag}" 的内容
          </Typography>

          {contentLoading ? (
            <Box display="flex" justifyContent="center" mt={4}>
              <Typography>加载中...</Typography>
            </Box>
          ) : error ? (
            <Alert severity="error" sx={{ mb: 3 }}>
              {error}
            </Alert>
          ) : tagContent.length === 0 ? (
            <Alert severity="info">
              该标签下暂无内容
            </Alert>
          ) : (
            <Grid container spacing={2}>
              {tagContent.map((item) => {
                const isExpanded = expandedCards.has(item.id);
                return (
                  <Grid item xs={12} md={6} key={item.id}>
                    <Card sx={{ height: '100%', display: 'flex', flexDirection: 'column' }}>
                      <CardContent sx={{ flexGrow: 1 }}>
                        <Box display="flex" justifyContent="space-between" alignItems="flex-start">
                          <Typography variant="h6" component="h3" gutterBottom sx={{ flex: 1 }}>
                            {item.title}
                          </Typography>
                          <IconButton
                            component={Link}
                            to={`/content/${item.id}`}
                            size="small"
                            color="primary"
                            title="查看详情"
                          >
                            <ViewIcon />
                          </IconButton>
                        </Box>

                        <Typography
                          variant="body2"
                          color="textSecondary"
                          sx={{ mb: 2 }}
                        >
                          {isExpanded ? item.content : truncateText(item.content)}
                        </Typography>

                        {item.content.length > 200 && (
                          <Box display="flex" justifyContent="center" mb={2}>
                            <IconButton
                              size="small"
                              onClick={() => toggleCardExpansion(item.id)}
                            >
                              {isExpanded ? <ExpandLessIcon /> : <ExpandMoreIcon />}
                            </IconButton>
                          </Box>
                        )}

                        <Collapse in={isExpanded}>
                          <Box mb={2}>
                            {formatTags(item.tags).map((tag, index) => (
                              <Chip
                                key={index}
                                label={tag}
                                size="small"
                                color={tag === selectedTag ? 'primary' : 'default'}
                                sx={{ mr: 0.5, mb: 0.5 }}
                              />
                            ))}
                          </Box>
                        </Collapse>

                        <Typography variant="caption" color="textSecondary">
                          创建时间: {formatDate(item.created_at)}
                        </Typography>
                        {item.updated_at !== item.created_at && (
                          <Typography variant="caption" color="textSecondary" display="block">
                            更新时间: {formatDate(item.updated_at)}
                          </Typography>
                        )}
                      </CardContent>
                    </Card>
                  </Grid>
                );
              })}
            </Grid>
          )}
        </Box>
      )}
    </Box>
  );
};

export default TagsView;