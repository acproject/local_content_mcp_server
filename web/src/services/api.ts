import axios from 'axios';

// 动态配置管理
class ConfigService {
  private static instance: ConfigService;
  private apiBaseUrl: string = process.env.NODE_ENV === 'development' ? '/api' : 'http://localhost:8086/api'; // 开发环境使用代理
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
      const possiblePorts = [8086, 5555, 3001, 8000];
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
        console.warn('⚠️ Could not auto-detect server, using default URL: http://localhost:8086/api');
        this.apiBaseUrl = 'http://localhost:8086/api';
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

// 后端实际返回的格式
export interface BackendContentListResponse {
  items: ContentItem[];
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
  private static parseFilenameFromContentDisposition(headerValue?: string): string | null {
    if (!headerValue) return null;

    const utf8Match = headerValue.match(/filename\*\s*=\s*UTF-8''([^;]+)/i);
    if (utf8Match?.[1]) {
      try {
        return decodeURIComponent(utf8Match[1].trim());
      } catch {
        return utf8Match[1].trim();
      }
    }

    const filenameMatch = headerValue.match(/filename\s*=\s*"([^"]+)"/i) || headerValue.match(/filename\s*=\s*([^;]+)/i);
    if (filenameMatch?.[1]) {
      return filenameMatch[1].trim();
    }

    return null;
  }

  static getExportUrl(id: number, format?: string): string {
    const baseUrl = configService.getApiBaseUrl();
    const query = format ? `?format=${encodeURIComponent(format)}` : '';
    return `${baseUrl}/content/${id}/export${query}`;
  }

  static getExportAllUrl(format: string = 'json'): string {
    const baseUrl = configService.getApiBaseUrl();
    const query = format ? `?format=${encodeURIComponent(format)}` : '';
    return `${baseUrl}/content/export${query}`;
  }

  static async exportContentFile(id: number, format?: string): Promise<{ blob: Blob; filename: string }> {
    const query = format ? `?format=${encodeURIComponent(format)}` : '';
    const response = await api.get(`/content/${id}/export${query}`, { responseType: 'blob' });

    const headerValue = (response.headers?.['content-disposition'] || response.headers?.['Content-Disposition']) as
      | string
      | undefined;
    const filenameFromHeader = ContentAPI.parseFilenameFromContentDisposition(headerValue);
    const filename = filenameFromHeader || `content_${id}`;

    return { blob: response.data as Blob, filename };
  }

  static async getContent(id: number): Promise<ContentItem> {
    const response = await api.get(`/content/${id}`);
    // 处理后端返回的包装格式 - 后端返回 {data: {...}, success: true}
    if (response.data && response.data.success && response.data.data) {
      return response.data.data;
    }
    return response.data;
  }

  static async listContent(page: number = 1, pageSize: number = 10): Promise<ContentListResponse> {
    const response = await api.get(`/content?page=${page}&page_size=${pageSize}`);
    
    let rawData: any;
    
    // 处理后端返回的包装格式 - 后端返回 {data: {...}, success: true}
    if (response.data && response.data.success && response.data.data) {
      rawData = response.data.data;
    } else {
      // 如果没有包装格式，直接使用数据
      rawData = response.data;
    }
    
    // 转换后端的 items 字段为前端期望的 content 字段
    if (rawData && rawData.items) {
      return {
        content: rawData.items,
        page: rawData.page || page,
        page_size: rawData.page_size || pageSize,
        total_count: rawData.total_count || 0,
        total_pages: rawData.total_pages || 0
      };
    }
    
    // 如果已经是正确格式，直接返回
    return rawData;
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
    const response = await api.get(`/content/search?q=${encodeURIComponent(query)}`);
    
    let rawData: any;
    
    // 处理后端返回的包装格式
    if (response.data && response.data.success && response.data.data) {
      rawData = response.data.data;
    } else {
      rawData = response.data;
    }
    
    // 处理后端返回的items字段或content字段
    if (rawData && rawData.items) {
      return rawData.items;
    }
    
    if (rawData && rawData.content) {
      return rawData.content;
    }
    
    // 如果直接是数组，返回数组
    if (Array.isArray(rawData)) {
      return rawData;
    }
    
    return [];
  }

  static async getTags(): Promise<string[]> {
    const response = await api.get('/tags');
    // 处理后端返回的包装格式
    if (response.data && response.data.data) {
      return response.data.data;
    }
    return response.data;
  }

  static async getContentByTag(tag: string): Promise<ContentItem[]> {
    const response = await api.get(`/content?tags=${encodeURIComponent(tag)}`);
    
    let rawData: any;
    
    // 处理后端返回的包装格式
    if (response.data && response.data.success && response.data.data) {
      rawData = response.data.data;
    } else {
      rawData = response.data;
    }
    
    // 处理后端返回的items字段或content字段
    if (rawData && rawData.items) {
      return rawData.items;
    }
    
    if (rawData && rawData.content) {
      return rawData.content;
    }
    
    // 如果直接是数组，返回数组
    if (Array.isArray(rawData)) {
      return rawData;
    }
    
    return [];
  }

  static async getStats(): Promise<any> {
    const response = await api.get('/stats');
    // 处理后端返回的包装格式
    if (response.data && response.data.data) {
      return response.data.data;
    }
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

  static async parseDocument(filePath: string, aiService?: 'ollama'): Promise<{ title: string; content: string; content_type: string; tags: string }> {
    try {
      console.log('📄 Parsing document:', filePath, 'with AI service:', aiService);
      
      const requestBody: any = {
        file_path: filePath
      };
      
      if (aiService) {
        requestBody.ai_service = aiService;
      }
      
      const response = await api.post('/files/parse', requestBody);
      
      console.log('✅ Document parsed successfully:', response.data);
      return response.data;
    } catch (error: any) {
      console.error('❌ Failed to parse document:', error);
      
      if (error.response?.status === 404) {
        throw new Error('文件未找到');
      } else if (error.response?.status === 400) {
        throw new Error(error.response.data?.error || '文件格式不支持');
      } else if (error.response?.status === 413) {
        throw new Error('文件过大');
      } else if (error.response?.status === 500) {
        throw new Error('服务器内部错误');
      } else if (error.code === 'ECONNABORTED') {
        throw new Error('请求超时，请稍后重试');
      } else if (error.code === 'ERR_NETWORK') {
        throw new Error('网络连接失败，请检查服务器状态');
      } else {
        throw new Error(error.response?.data?.error || error.message || '解析文档失败');
      }
    }
  }
}

// Ollama API 类
export class OllamaAPI {
  static async getModels(): Promise<string[]> {
    try {
      console.log('🤖 Getting Ollama models...');
      const response = await api.get('/ollama/models');
      console.log('✅ Ollama models retrieved:', response.data);
      return response.data.models || [];
    } catch (error: any) {
      console.error('❌ Failed to get Ollama models:', error);
      if (error.response?.status === 503) {
        throw new Error('Ollama 服务未启用或无法连接');
      }
      throw new Error(error.response?.data?.error || error.message || '获取 Ollama 模型列表失败');
    }
  }

  static async generate(prompt: string, model?: string, options?: { temperature?: number; max_tokens?: number }): Promise<any> {
    try {
      console.log('🤖 Generating with Ollama:', { prompt, model, options });
      const requestBody: any = {
        prompt
      };
      
      if (model) {
        requestBody.model = model;
      }
      
      if (options?.temperature !== undefined) {
        requestBody.temperature = options.temperature;
      }
      
      if (options?.max_tokens !== undefined) {
        requestBody.max_tokens = options.max_tokens;
      }
      
      const response = await api.post('/ollama/generate', requestBody);
      console.log('✅ Ollama generation completed:', response.data);
      return response.data;
    } catch (error: any) {
      console.error('❌ Failed to generate with Ollama:', error);
      if (error.response?.status === 503) {
        throw new Error('Ollama 服务未启用或无法连接');
      }
      throw new Error(error.response?.data?.error || error.message || 'Ollama 生成失败');
    }
  }

  static async getStatus(): Promise<any> {
    try {
      console.log('🤖 Getting Ollama status...');
      const response = await api.get('/ollama/status');
      console.log('✅ Ollama status retrieved:', response.data);
      return response.data;
    } catch (error: any) {
      console.error('❌ Failed to get Ollama status:', error);
      throw new Error(error.response?.data?.error || error.message || '获取 Ollama 状态失败');
    }
  }
}

export default api;
