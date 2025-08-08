import React, { useEffect, useState } from 'react';
import { BrowserRouter as Router, Routes, Route, Link } from 'react-router-dom';
import {
  AppBar,
  Toolbar,
  Typography,
  Container,
  Box,
  Button,
  CssBaseline,
  ThemeProvider,
  createTheme,
  CircularProgress,
  Alert,
  Chip,
} from '@mui/material';
import {
  Home as HomeIcon,
  Add as AddIcon,
  Search as SearchIcon,
  Label as TagIcon,
  Settings as SettingsIcon,
  CloudDone as ConnectedIcon,
  CloudOff as DisconnectedIcon,
} from '@mui/icons-material';

import ContentList from './components/ContentList';
import ContentForm from './components/ContentForm';
import ContentSearch from './components/ContentSearch';
import TagsView from './components/TagsView';
import ContentDetail from './components/ContentDetail';
import ConfigView from './components/ConfigView';
import { ConfigAPI } from './services/api';

const theme = createTheme({
  palette: {
    primary: {
      main: '#1976d2',
    },
    secondary: {
      main: '#dc004e',
    },
  },
});

function App() {
  const [isInitializing, setIsInitializing] = useState(true);
  const [connectionStatus, setConnectionStatus] = useState<'connected' | 'disconnected' | 'connecting'>('connecting');
  const [serverInfo, setServerInfo] = useState<{ url: string; config: any } | null>(null);
  const [initError, setInitError] = useState<string | null>(null);

  useEffect(() => {
    const initializeApp = async () => {
      try {
        setConnectionStatus('connecting');
        await ConfigAPI.initializeService();
        const info = ConfigAPI.getServerInfo();
        setServerInfo(info);
        setConnectionStatus('connected');
        setInitError(null);
      } catch (error) {
        console.error('Failed to initialize app:', error);
        setConnectionStatus('disconnected');
        setInitError(error instanceof Error ? error.message : '连接服务器失败');
      } finally {
        setIsInitializing(false);
      }
    };

    initializeApp();
  }, []);

  if (isInitializing) {
    return (
      <ThemeProvider theme={theme}>
        <CssBaseline />
        <Box
          display="flex"
          flexDirection="column"
          justifyContent="center"
          alignItems="center"
          minHeight="100vh"
          gap={2}
        >
          <CircularProgress size={60} />
          <Typography variant="h6">正在连接服务器...</Typography>
          <Typography variant="body2" color="textSecondary">
            正在自动检测服务器配置
          </Typography>
        </Box>
      </ThemeProvider>
    );
  }

  return (
    <ThemeProvider theme={theme}>
      <CssBaseline />
      <Router>
        <Box sx={{ flexGrow: 1 }}>
          <AppBar position="static">
            <Toolbar>
              <Typography variant="h6" component="div" sx={{ flexGrow: 1 }}>
                内容管理系统
              </Typography>
              
              {/* 连接状态指示器 */}
              <Box sx={{ mr: 2 }}>
                <Chip
                  icon={connectionStatus === 'connected' ? <ConnectedIcon /> : <DisconnectedIcon />}
                  label={connectionStatus === 'connected' ? '已连接' : '未连接'}
                  color={connectionStatus === 'connected' ? 'success' : 'error'}
                  size="small"
                  variant="outlined"
                  sx={{ color: 'white', borderColor: 'white' }}
                />
              </Box>
              
              <Button color="inherit" component={Link} to="/" startIcon={<HomeIcon />}>
                首页
              </Button>
              <Button color="inherit" component={Link} to="/add" startIcon={<AddIcon />}>
                创建
              </Button>
              <Button color="inherit" component={Link} to="/search" startIcon={<SearchIcon />}>
                搜索
              </Button>
              <Button color="inherit" component={Link} to="/tags" startIcon={<TagIcon />}>
                标签
              </Button>
              <Button color="inherit" component={Link} to="/config" startIcon={<SettingsIcon />}>
                配置
              </Button>
            </Toolbar>
          </AppBar>

          <Container maxWidth="lg" sx={{ mt: 4, mb: 4 }}>
            {/* 显示连接错误 */}
            {initError && (
              <Alert severity="warning" sx={{ mb: 2 }}>
                服务器连接异常: {initError}
                {serverInfo && (
                  <Typography variant="body2" sx={{ mt: 1 }}>
                    当前使用: {serverInfo.url}
                  </Typography>
                )}
              </Alert>
            )}
            
            {/* 显示服务器信息 */}
            {serverInfo && connectionStatus === 'connected' && (
              <Alert severity="info" sx={{ mb: 2 }}>
                已连接到服务器: {serverInfo.url}
              </Alert>
            )}
            
            <Routes>
              <Route path="/" element={<ContentList />} />
              <Route path="/add" element={<ContentForm />} />
              <Route path="/edit/:id" element={<ContentForm />} />
              <Route path="/content/:id" element={<ContentDetail />} />
              <Route path="/search" element={<ContentSearch />} />
              <Route path="/tags" element={<TagsView />} />
              <Route path="/config" element={<ConfigView />} />
            </Routes>
          </Container>
        </Box>
      </Router>
    </ThemeProvider>
  );
}

export default App;