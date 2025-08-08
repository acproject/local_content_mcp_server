import axios from 'axios';

// 动态配置管理
class ConfigService {
  private static instance: ConfigService;
  private apiBaseUrl: string = process.env.NODE_ENV === 'development' ? '/api' : 'http://localhost:8080/api'; // 开发环境使用代理
  private serverConfig: ServerConfig | null = null;

  private constructor() {}

  static getInstance(): ConfigService {
    if (!ConfigService.instance) {
      ConfigService.instance = new ConfigService();
    }
    return ConfigService.instance;
  }

  async initialize(): Promise<void> {
    try {
      console.log('Initializing config service...');
      
      // 在开发环境中，直接使用代理，不需要自动检测
      if (process.env.NODE_ENV === 'development') {
        console.log('Development mode: using proxy /api');
        this.apiBaseUrl = '/api';
        try {
          const response = await axios.get('/api/config', { timeout: 3000 });
          if (response.status === 200) {
            this.serverConfig = response.data;
            console.log('✅ Successfully connected via proxy');
            console.log('Server config:', response.data);
            return;
          }
        } catch (error) {
          console.log('❌ Proxy connection failed, falling back to auto-detection');
        }
      }
      
      // 尝试从多个可能的端口获取服务器信息
      const possiblePorts = [8080, 5555, 3001, 8000];
      const possibleHosts = ['localhost', '127.0.0.1'];
      
      for (const host of possibleHosts) {
        for (const port of possiblePorts) {
          const testUrl = `http://${host}:${port}`;
          try {
            console.log(`Trying to connect to ${testUrl}/api/config`);
            const response = await axios.get(`${testUrl}/api/config`, {
              timeout: 3000
            });
            
            if (response.status === 200) {
              this.apiBaseUrl = `${testUrl}/api`;
              this.serverConfig = response.data;
              console.log(`✅ Successfully connected to server at ${testUrl}`);
              console.log('Server config:', response.data);
              return;
            }
          } catch (error) {
            console.log('❌ Failed to connect to', testUrl);
            // 继续尝试下一个端口
            continue;
          }
        }
      }
      
      // 如果都失败了，使用环境变量或默认值
      const envUrl = process.env.REACT_APP_API_URL;
      if (envUrl) {
        this.apiBaseUrl = envUrl;
        console.log(`Using API URL from environment: ${envUrl}`);
      } else {
        console.warn('⚠️ Could not auto-detect server, using default URL: http://localhost:8080/api');
        this.apiBaseUrl = 'http://localhost:8080/api';
      }
    } catch (error) {
      console.error('❌ Failed to initialize config service:', error);
      throw error;
    }
  }

  getApiBaseUrl(): string {
    console.log('📍 Current API base URL:', this.apiBaseUrl);
    return this.apiBaseUrl;
  }

  getServerConfig(): ServerConfig | null {
    return this.serverConfig;
  }

  setServerConfig(config: ServerConfig): void {
    this.serverConfig = config;
  }
}

// 创建配置服务实例
const configService = ConfigService.getInstance();

// 创建 axios 实例，使用动态配置
const createApiInstance = () => {
  return axios.create({
    baseURL: configService.getApiBaseUrl(),
    headers: {
      'Content-Type': 'application/json',
    },
  });
};

// 初始 API 实例
let api = createApiInstance();

// 导出配置服务和更新 API 实例的函数
export { configService };
export const updateApiInstance = () => {
  api = createApiInstance();
};

export interface ContentItem {
  id: number;
  title: string;
  content: string;
  content_type: string;
  tags: string;
  created_at: string;
  updated_at: string;
}

export interface CreateContentRequest {
  title: string;
  content: string;
  content_type: string;
  tags: string;
}

export interface UpdateContentRequest {
  title: string;
  content: string;
  content_type: string;
  tags: string;
}

export interface ContentListResponse {
  content: ContentItem[];
  page: number;
  page_size: number;
  total_count: number;
  total_pages: number;
}

export interface ServerConfig {
  host: string;
  port: number;
  database_path: string;
  log_level: string;
  log_file: string;
  max_content_size: number;
  default_page_size: number;
  max_page_size: number;
  enable_cors: boolean;
  cors_origin: string;
  static_files_path: string;
  enable_static_files: boolean;
  upload_path: string;
  max_file_size: number;
  allowed_file_types: string[];
  enable_file_upload: boolean;
  llama_model_path: string;
  llama_executable_path: string;
  llama_context_size: number;
  llama_threads: number;
  llama_temperature: number;
  llama_max_tokens: number;
  enable_llama: boolean;
  ollama_host: string;
  ollama_port: number;
  ollama_model: string;
  ollama_temperature: number;
  ollama_max_tokens: number;
  ollama_timeout: number;
  enable_ollama: boolean;
}

export class ConfigAPI {
  static async getConfig(): Promise<ServerConfig> {
    const response = await api.get('/config');
    const config = response.data;
    // 更新配置服务中的服务器配置
    configService.setServerConfig(config);
    return config;
  }

  static async updateConfig(config: ServerConfig): Promise<ServerConfig> {
    console.log('🔄 Updating config with API base URL:', configService.getApiBaseUrl());
    console.log('🔄 Config data to send:', JSON.stringify(config, null, 2));
    
    const response = await api.put('/config', config);
    console.log('✅ Config update response:', response.status, response.data);
    
    const updatedConfig = response.data.config || response.data;
    // 更新配置服务中的服务器配置
    configService.setServerConfig(updatedConfig);
    return updatedConfig;
  }

  static async initializeService(): Promise<void> {
    await configService.initialize();
    updateApiInstance();
  }

  static getServerInfo(): { url: string; config: ServerConfig | null } {
    return {
      url: configService.getApiBaseUrl(),
      config: configService.getServerConfig()
    };
  }
}

export class ContentAPI {
  static async getContent(id: number): Promise<ContentItem> {
    const response = await api.get(`/content/${id}`);
    return response.data;
  }

  static async listContent(page: number = 1, pageSize: number = 10): Promise<ContentListResponse> {
    const response = await api.get(`/content?page=${page}&page_size=${pageSize}`);
    return response.data;
  }

  static async createContent(content: CreateContentRequest): Promise<ContentItem> {
    const response = await api.post('/content', content);
    return response.data;
  }

  static async updateContent(id: number, content: UpdateContentRequest): Promise<ContentItem> {
    const response = await api.put(`/content/${id}`, content);
    return response.data;
  }

  static async deleteContent(id: number): Promise<void> {
    await api.delete(`/content/${id}`);
  }

  static async searchContent(query: string): Promise<ContentItem[]> {
    const response = await api.get(`/content/search?query=${encodeURIComponent(query)}`);
    return response.data;
  }

  static async getTags(): Promise<string[]> {
    const response = await api.get('/tags');
    return response.data;
  }

  static async getContentByTag(tag: string): Promise<ContentItem[]> {
    const response = await api.get(`/content/tag/${encodeURIComponent(tag)}`);
    return response.data;
  }

  static async getStats(): Promise<any> {
    const response = await api.get('/stats');
    return response.data;
  }

  static async uploadFile(file: File): Promise<{ file_path: string; file_name: string }> {
    const formData = new FormData();
    formData.append('file', file);
    
    const response = await api.post('/files/upload', formData, {
      headers: {
        'Content-Type': 'multipart/form-data',
      },
    });
    
    // 适配后端返回的格式
    const data = response.data;
    return {
      file_path: data.file_info?.file_path || data.file_info?.id || '',
      file_name: data.file_info?.filename || data.file_info?.original_name || file.name
    };
  }

  static async parseDocument(filePath: string): Promise<{ title: string; content: string; content_type: string; tags: string }> {
    try {
      // 首先尝试使用 LLaMA 进行智能解析
      const response = await api.post('/llama/generate', {
        prompt: `Please analyze this file: ${filePath} and extract the following information in JSON format:
{
  "title": "extracted title",
  "content": "main content summary",
  "content_type": "document type",
  "tags": "comma-separated tags"
}`,
        max_tokens: 500
      });
      
      // 尝试解析 LLaMA 返回的 JSON
      const generatedText = response.data.generated_text || response.data.text || '';
      const jsonMatch = generatedText.match(/\{[^}]+\}/);
      
      if (jsonMatch) {
        const parsed = JSON.parse(jsonMatch[0]);
        return {
          title: parsed.title || 'Untitled Document',
          content: parsed.content || 'No content extracted',
          content_type: parsed.content_type || 'document',
          tags: parsed.tags || ''
        };
      }
    } catch (error) {
      console.warn('LLaMA service unavailable, using basic file analysis:', error);
    }
    
    // 如果 LLaMA 不可用，提供基本的文件分析
    const fileName = filePath.split('/').pop() || 'document';
    const nameWithoutExt = fileName.replace(/\.[^/.]+$/, '');
    const fileExt = fileName.split('.').pop()?.toLowerCase() || '';
    
    // 根据文件扩展名确定内容类型
    let contentType = 'document';
    let tags = 'uploaded';
    
    if (['txt', 'md', 'markdown'].includes(fileExt)) {
      contentType = 'text';
      tags += ', text, document';
    } else if (['pdf'].includes(fileExt)) {
      contentType = 'pdf';
      tags += ', pdf, document';
    } else if (['doc', 'docx'].includes(fileExt)) {
      contentType = 'word';
      tags += ', word, document';
    } else if (['jpg', 'jpeg', 'png', 'gif'].includes(fileExt)) {
      contentType = 'image';
      tags += ', image, media';
    }
    
    return {
      title: nameWithoutExt.replace(/[_-]/g, ' ').replace(/\b\w/g, l => l.toUpperCase()),
      content: `Document uploaded: ${fileName}\n\nThis file has been uploaded and is ready for editing. You can modify the title, content, and tags as needed.`,
      content_type: contentType,
      tags: tags
    };
  }
}

export default api;