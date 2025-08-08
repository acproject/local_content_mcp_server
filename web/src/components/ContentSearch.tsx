import React, { useState } from 'react';
import { Link } from 'react-router-dom';
import {
  Paper,
  TextField,
  Button,
  Typography,
  Box,
  Card,
  CardContent,
  Chip,
  Alert,
  InputAdornment,
  IconButton,
} from '@mui/material';
import {
  Search as SearchIcon,
  Clear as ClearIcon,
  Visibility as ViewIcon,
} from '@mui/icons-material';

import { ContentAPI, ContentItem } from '../services/api';

const ContentSearch: React.FC = () => {
  const [query, setQuery] = useState('');
  const [results, setResults] = useState<ContentItem[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [hasSearched, setHasSearched] = useState(false);

  const handleSearch = async (event?: React.FormEvent) => {
    if (event) {
      event.preventDefault();
    }
    
    if (!query.trim()) {
      setError('请输入搜索关键词');
      return;
    }

    try {
      setLoading(true);
      setError(null);
      setHasSearched(true);
      const data = await ContentAPI.searchContent(query.trim());
      setResults(data);
    } catch (err) {
      setError('搜索失败');
      console.error('Error searching content:', err);
    } finally {
      setLoading(false);
    }
  };

  const handleClear = () => {
    setQuery('');
    setResults([]);
    setError(null);
    setHasSearched(false);
  };

  const formatDate = (dateString: string) => {
    return new Date(dateString).toLocaleString('zh-CN');
  };

  const formatTags = (tags: string) => {
    return tags.split(',').filter(tag => tag.trim());
  };

  const highlightText = (text: string, searchQuery: string) => {
    if (!searchQuery.trim()) return text;
    
    const regex = new RegExp(`(${searchQuery.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')})`, 'gi');
    const parts = text.split(regex);
    
    return parts.map((part, index) => 
      regex.test(part) ? (
        <Typography component="span" key={index} style={{ backgroundColor: '#ffeb3b', padding: '0 2px' }}>
          {part}
        </Typography>
      ) : (
        <Typography component="span" key={index}>{part}</Typography>
      )
    );
  };

  return (
    <Box>
      <Typography variant="h4" component="h1" gutterBottom>
        搜索内容
      </Typography>

      <Paper elevation={3} sx={{ p: 3, mb: 3 }}>
        <Box component="form" onSubmit={handleSearch}>
          <Box display="flex" gap={2}>
            <TextField
              fullWidth
              label="搜索关键词"
              value={query}
              onChange={(e: React.ChangeEvent<HTMLInputElement>) => setQuery(e.target.value)}
              placeholder="输入标题、内容或标签关键词..."
              InputProps={{
                endAdornment: (
                  <InputAdornment position="end">
                    {query && (
                      <IconButton onClick={handleClear} size="small">
                        <ClearIcon />
                      </IconButton>
                    )}
                  </InputAdornment>
                ),
              }}
            />
            <Button
              type="submit"
              variant="contained"
              color="primary"
              startIcon={<SearchIcon />}
              disabled={loading}
              sx={{ minWidth: 120 }}
            >
              {loading ? '搜索中...' : '搜索'}
            </Button>
          </Box>
        </Box>
      </Paper>

      {error && (
        <Alert severity="error" sx={{ mb: 3 }}>
          {error}
        </Alert>
      )}

      {hasSearched && (
        <Box>
          <Typography variant="h6" gutterBottom>
            搜索结果 ({results.length} 条)
          </Typography>

          {results.length === 0 ? (
            <Alert severity="info">
              没有找到匹配的内容，请尝试其他关键词。
            </Alert>
          ) : (
            <Box>
              {results.map((item) => (
                <Card key={item.id} sx={{ mb: 2 }}>
                  <CardContent>
                    <Box display="flex" justifyContent="space-between" alignItems="flex-start">
                      <Box flex={1}>
                        <Typography variant="h6" component="h3" gutterBottom>
                          {highlightText(item.title, query)}
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
                          {highlightText(item.content, query)}
                        </Typography>
                        <Box mb={2}>
                          {formatTags(item.tags).map((tag, index) => (
                            <Chip
                              key={index}
                              label={highlightText(tag, query)}
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
                      </Box>
                      <Box ml={2}>
                        <IconButton
                          component={Link}
                          to={`/content/${item.id}`}
                          color="primary"
                          title="查看详情"
                        >
                          <ViewIcon />
                        </IconButton>
                      </Box>
                    </Box>
                  </CardContent>
                </Card>
              ))}
            </Box>
          )}
        </Box>
      )}
    </Box>
  );
};

export default ContentSearch;