import axios from 'axios';

const API_BASE_URL = process.env.REACT_APP_API_URL || 'http://localhost:8080/api';

const api = axios.create({
  baseURL: API_BASE_URL,
  headers: {
    'Content-Type': 'application/json',
  },
});

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
}

export default api;