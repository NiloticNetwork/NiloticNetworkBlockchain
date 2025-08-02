const tf = require('@tensorflow/tfjs-node');
const natural = require('natural');
const Sentiment = require('sentiment');
const logger = require('../utils/logger');

class AIService {
  constructor() {
    this.model = null;
    this.tokenizer = new natural.WordTokenizer();
    this.sentiment = new Sentiment();
    this.userPreferences = new Map();
    this.contentEmbeddings = new Map();
    this.isInitialized = false;
  }

  async initialize() {
    try {
      // Initialize TensorFlow
      await tf.ready();
      logger.info('TensorFlow initialized');

      // Load or create recommendation model
      await this.loadRecommendationModel();
      
      // Initialize content processing
      this.initializeContentProcessing();
      
      this.isInitialized = true;
      logger.info('AI service initialized successfully');
    } catch (error) {
      logger.error('Failed to initialize AI service:', error);
      throw error;
    }
  }

  async loadRecommendationModel() {
    try {
      // In a real implementation, this would load a pre-trained model
      // For now, we'll create a simple collaborative filtering model
      this.model = this.createRecommendationModel();
      logger.info('Recommendation model loaded');
    } catch (error) {
      logger.error('Error loading recommendation model:', error);
      throw error;
    }
  }

  createRecommendationModel() {
    // Simple collaborative filtering model
    const model = tf.sequential({
      layers: [
        tf.layers.dense({ units: 64, activation: 'relu', inputShape: [50] }),
        tf.layers.dropout({ rate: 0.2 }),
        tf.layers.dense({ units: 32, activation: 'relu' }),
        tf.layers.dropout({ rate: 0.2 }),
        tf.layers.dense({ units: 16, activation: 'relu' }),
        tf.layers.dense({ units: 1, activation: 'sigmoid' })
      ]
    });

    model.compile({
      optimizer: tf.train.adam(0.001),
      loss: 'binaryCrossentropy',
      metrics: ['accuracy']
    });

    return model;
  }

  initializeContentProcessing() {
    // Initialize content processing utilities
    this.contentProcessor = {
      extractKeywords: (text) => {
        const tokens = this.tokenizer.tokenize(text.toLowerCase());
        const stopWords = new Set(['the', 'a', 'an', 'and', 'or', 'but', 'in', 'on', 'at', 'to', 'for', 'of', 'with', 'by']);
        return tokens.filter(token => !stopWords.has(token) && token.length > 2);
      },
      
      extractFeatures: (text) => {
        const keywords = this.contentProcessor.extractKeywords(text);
        const sentiment = this.sentiment.analyze(text);
        const length = text.length;
        const wordCount = keywords.length;
        
        return {
          keywords,
          sentiment: sentiment.score,
          length,
          wordCount,
          complexity: length / wordCount
        };
      },
      
      createEmbedding: (text) => {
        const features = this.contentProcessor.extractFeatures(text);
        // Create a simple embedding based on features
        const embedding = new Array(50).fill(0);
        
        // Use keywords to create embedding
        features.keywords.forEach((keyword, index) => {
          if (index < 30) {
            const hash = this.hashString(keyword);
            embedding[index] = (hash % 100) / 100;
          }
        });
        
        // Add sentiment and other features
        embedding[30] = (features.sentiment + 5) / 10; // Normalize sentiment
        embedding[31] = features.length / 1000; // Normalize length
        embedding[32] = features.wordCount / 100; // Normalize word count
        embedding[33] = features.complexity / 10; // Normalize complexity
        
        return embedding;
      }
    };
  }

  hashString(str) {
    let hash = 0;
    for (let i = 0; i < str.length; i++) {
      const char = str.charCodeAt(i);
      hash = ((hash << 5) - hash) + char;
      hash = hash & hash; // Convert to 32-bit integer
    }
    return Math.abs(hash);
  }

  // Content recommendation methods
  async getRecommendations(userAddress, limit = 10) {
    try {
      // Get user preferences
      const userPrefs = this.userPreferences.get(userAddress) || this.getDefaultPreferences();
      
      // Get content embeddings
      const contentIds = Array.from(this.contentEmbeddings.keys());
      const recommendations = [];
      
      for (const contentId of contentIds) {
        const embedding = this.contentEmbeddings.get(contentId);
        const score = this.calculateRecommendationScore(userPrefs, embedding);
        
        recommendations.push({
          contentId,
          score,
          embedding
        });
      }
      
      // Sort by score and return top recommendations
      recommendations.sort((a, b) => b.score - a.score);
      return recommendations.slice(0, limit);
    } catch (error) {
      logger.error('Error getting recommendations:', error);
      throw error;
    }
  }

  calculateRecommendationScore(userPrefs, contentEmbedding) {
    // Simple cosine similarity
    let dotProduct = 0;
    let userNorm = 0;
    let contentNorm = 0;
    
    for (let i = 0; i < userPrefs.length; i++) {
      dotProduct += userPrefs[i] * contentEmbedding[i];
      userNorm += userPrefs[i] * userPrefs[i];
      contentNorm += contentEmbedding[i] * contentEmbedding[i];
    }
    
    const similarity = dotProduct / (Math.sqrt(userNorm) * Math.sqrt(contentNorm));
    return similarity || 0;
  }

  getDefaultPreferences() {
    // Default user preferences (balanced)
    return new Array(50).fill(0.5);
  }

  // User preference learning
  async updateUserPreferences(userAddress, contentId, interaction) {
    try {
      const contentEmbedding = this.contentEmbeddings.get(contentId);
      if (!contentEmbedding) return;
      
      let userPrefs = this.userPreferences.get(userAddress) || this.getDefaultPreferences();
      
      // Update preferences based on interaction
      const learningRate = 0.1;
      const weight = interaction === 'like' ? 1 : interaction === 'dislike' ? -1 : 0.5;
      
      for (let i = 0; i < userPrefs.length; i++) {
        userPrefs[i] += learningRate * weight * contentEmbedding[i];
        userPrefs[i] = Math.max(0, Math.min(1, userPrefs[i])); // Clamp between 0 and 1
      }
      
      this.userPreferences.set(userAddress, userPrefs);
      logger.info(`Updated preferences for user ${userAddress}`);
    } catch (error) {
      logger.error('Error updating user preferences:', error);
      throw error;
    }
  }

  // Content analysis
  async analyzeContent(content) {
    try {
      const features = this.contentProcessor.extractFeatures(content);
      const embedding = this.contentProcessor.createEmbedding(content);
      
      return {
        features,
        embedding,
        sentiment: features.sentiment,
        keywords: features.keywords,
        complexity: features.complexity,
        engagement: this.predictEngagement(features)
      };
    } catch (error) {
      logger.error('Error analyzing content:', error);
      throw error;
    }
  }

  predictEngagement(features) {
    // Simple engagement prediction based on features
    let score = 0;
    
    // Sentiment impact
    score += Math.abs(features.sentiment) * 0.3;
    
    // Length impact (optimal length around 100-200 words)
    const optimalLength = 150;
    const lengthScore = 1 - Math.abs(features.length - optimalLength) / optimalLength;
    score += lengthScore * 0.2;
    
    // Keyword diversity
    const keywordDiversity = features.keywords.length / 10;
    score += Math.min(keywordDiversity, 1) * 0.3;
    
    // Complexity (moderate complexity is better)
    const complexityScore = 1 - Math.abs(features.complexity - 5) / 5;
    score += complexityScore * 0.2;
    
    return Math.max(0, Math.min(1, score));
  }

  // Content categorization
  categorizeContent(content) {
    const features = this.contentProcessor.extractFeatures(content);
    const categories = {
      cultural: ['culture', 'tradition', 'heritage', 'african', 'nilotic', 'kenya'],
      art: ['art', 'creative', 'design', 'visual', 'aesthetic'],
      music: ['music', 'song', 'melody', 'rhythm', 'beat'],
      story: ['story', 'narrative', 'tale', 'legend', 'folklore'],
      star: ['star', 'sulwe', 'cosmic', 'celestial', 'astral'],
      technology: ['tech', 'innovation', 'digital', 'blockchain', 'crypto'],
      community: ['community', 'social', 'people', 'together', 'unity']
    };
    
    const scores = {};
    for (const [category, keywords] of Object.entries(categories)) {
      scores[category] = 0;
      for (const keyword of features.keywords) {
        if (keywords.some(k => keyword.includes(k))) {
          scores[category] += 1;
        }
      }
    }
    
    // Return top category
    const topCategory = Object.entries(scores).reduce((a, b) => 
      scores[a[0]] > scores[b[0]] ? a : b
    );
    
    return {
      primaryCategory: topCategory[0],
      confidence: topCategory[1] / features.keywords.length,
      allScores: scores
    };
  }

  // Analytics for creators
  async getCreatorAnalytics(creatorAddress, timeRange = '30d') {
    try {
      // This would integrate with the blockchain service to get creator data
      const analytics = {
        totalPosts: 0,
        totalLikes: 0,
        totalComments: 0,
        totalRewards: 0,
        averageEngagement: 0,
        topPerformingContent: [],
        audienceGrowth: [],
        contentCategories: {},
        sentimentTrends: [],
        recommendations: []
      };
      
      // Generate recommendations for improvement
      analytics.recommendations = this.generateCreatorRecommendations(analytics);
      
      return analytics;
    } catch (error) {
      logger.error('Error getting creator analytics:', error);
      throw error;
    }
  }

  generateCreatorRecommendations(analytics) {
    const recommendations = [];
    
    if (analytics.averageEngagement < 0.5) {
      recommendations.push({
        type: 'engagement',
        priority: 'high',
        message: 'Consider posting more engaging content with questions or calls to action',
        action: 'Add interactive elements to your posts'
      });
    }
    
    if (analytics.totalPosts < 10) {
      recommendations.push({
        type: 'frequency',
        priority: 'medium',
        message: 'Increase posting frequency to build audience',
        action: 'Aim for 2-3 posts per week'
      });
    }
    
    return recommendations;
  }

  // Content moderation
  async moderateContent(content) {
    try {
      const features = this.contentProcessor.extractFeatures(content);
      const sentiment = features.sentiment;
      
      // Simple moderation rules
      const moderation = {
        isApproved: true,
        confidence: 0.9,
        flags: [],
        riskScore: 0
      };
      
      // Check for negative sentiment
      if (sentiment < -3) {
        moderation.flags.push('negative_sentiment');
        moderation.riskScore += 0.3;
      }
      
      // Check for inappropriate keywords
      const inappropriateKeywords = ['spam', 'scam', 'fake', 'hate'];
      const foundKeywords = features.keywords.filter(keyword => 
        inappropriateKeywords.some(inappropriate => 
          keyword.includes(inappropriate)
        )
      );
      
      if (foundKeywords.length > 0) {
        moderation.flags.push('inappropriate_keywords');
        moderation.riskScore += 0.5;
      }
      
      // Determine approval
      if (moderation.riskScore > 0.7) {
        moderation.isApproved = false;
        moderation.confidence = 0.8;
      }
      
      return moderation;
    } catch (error) {
      logger.error('Error moderating content:', error);
      throw error;
    }
  }

  // Model training
  async trainModel(trainingData) {
    try {
      if (!this.model) {
        this.model = this.createRecommendationModel();
      }
      
      // Prepare training data
      const { xs, ys } = this.prepareTrainingData(trainingData);
      
      // Train the model
      await this.model.fit(xs, ys, {
        epochs: 10,
        batchSize: 32,
        validationSplit: 0.2,
        callbacks: {
          onEpochEnd: (epoch, logs) => {
            logger.info(`Epoch ${epoch + 1}: loss = ${logs.loss.toFixed(4)}, accuracy = ${logs.accuracy.toFixed(4)}`);
          }
        }
      });
      
      logger.info('Model training completed');
    } catch (error) {
      logger.error('Error training model:', error);
      throw error;
    }
  }

  prepareTrainingData(data) {
    // Convert training data to tensors
    const xs = [];
    const ys = [];
    
    for (const item of data) {
      xs.push(item.features);
      ys.push(item.label);
    }
    
    return {
      xs: tf.tensor2d(xs),
      ys: tf.tensor2d(ys)
    };
  }

  // Utility methods
  isInitialized() {
    return this.isInitialized;
  }

  getModel() {
    return this.model;
  }

  getUserPreferences(userAddress) {
    return this.userPreferences.get(userAddress) || this.getDefaultPreferences();
  }

  setContentEmbedding(contentId, embedding) {
    this.contentEmbeddings.set(contentId, embedding);
  }

  getContentEmbedding(contentId) {
    return this.contentEmbeddings.get(contentId);
  }
}

module.exports = new AIService(); 