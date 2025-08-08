import React from 'react';
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
} from '@mui/material';
import {
  Home as HomeIcon,
  Add as AddIcon,
  Search as SearchIcon,
  Label as TagIcon,
  Settings as SettingsIcon,
} from '@mui/icons-material';

import ContentList from './components/ContentList';
import ContentForm from './components/ContentForm';
import ContentSearch from './components/ContentSearch';
import TagsView from './components/TagsView';
import ContentDetail from './components/ContentDetail';
import ConfigView from './components/ConfigView';

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
              <Button color="inherit" component={Link} to="/" startIcon={<HomeIcon />}>
                首页
              </Button>
              <Button color="inherit" component={Link} to="/create" startIcon={<AddIcon />}>
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
            <Routes>
                <Route path="/" element={<ContentList />} />
              <Route path="/create" element={<ContentForm />} />
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