const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const morgan = require('morgan');
const compression = require('compression');
const rateLimit = require('express-rate-limit');
require('dotenv').config();

const logger = require('./utils/logger');
const errorHandler = require('./middleware/errorHandler');
const authMiddleware = require('./middleware/auth');

// Import routes
const authRoutes = require('./routes/auth');
const profileRoutes = require('./routes/profiles');
const postRoutes = require('./routes/posts');
const nftRoutes = require('./routes/nfts');
const aiRoutes = require('./routes/ai');
const daoRoutes = require('./routes/dao');
const analyticsRoutes = require('./routes/analytics');

// Import services
const blockchainService = require('./services/blockchain');
const ipfsService = require('./services/ipfs');
const aiService = require('./services/ai');

const app = express();
const PORT = process.env.PORT || 3001;

// Security middleware
app.use(helmet({
  contentSecurityPolicy: {
    directives: {
      defaultSrc: ["'self'"],
      styleSrc: ["'self'", "'unsafe-inline'"],
      scriptSrc: ["'self'"],
      imgSrc: ["'self'", "data:", "https:"],
    },
  },
}));

// CORS configuration
app.use(cors({
  origin: process.env.FRONTEND_URL || 'http://localhost:3000',
  credentials: true,
  methods: ['GET', 'POST', 'PUT', 'DELETE', 'OPTIONS'],
  allowedHeaders: ['Content-Type', 'Authorization', 'X-Requested-With'],
}));

// Rate limiting
const limiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 100, // limit each IP to 100 requests per windowMs
  message: 'Too many requests from this IP, please try again later.',
  standardHeaders: true,
  legacyHeaders: false,
});
app.use('/api/', limiter);

// Body parsing middleware
app.use(express.json({ limit: '10mb' }));
app.use(express.urlencoded({ extended: true, limit: '10mb' }));

// Compression middleware
app.use(compression());

// Logging middleware
app.use(morgan('combined', {
  stream: {
    write: (message) => logger.info(message.trim()),
  },
}));

// Health check endpoint
app.get('/health', (req, res) => {
  res.status(200).json({
    status: 'OK',
    timestamp: new Date().toISOString(),
    uptime: process.uptime(),
    environment: process.env.NODE_ENV || 'development',
    version: process.env.npm_package_version || '1.0.0',
  });
});

// API documentation
app.get('/api', (req, res) => {
  res.json({
    name: 'Parrot API',
    version: '1.0.0',
    description: 'AI-driven SocialFi platform API for Nilotic blockchain',
    endpoints: {
      auth: '/api/auth',
      profiles: '/api/profiles',
      posts: '/api/posts',
      nfts: '/api/nfts',
      ai: '/api/ai',
      dao: '/api/dao',
      analytics: '/api/analytics',
    },
  });
});

// API routes
app.use('/api/auth', authRoutes);
app.use('/api/profiles', authMiddleware, profileRoutes);
app.use('/api/posts', authMiddleware, postRoutes);
app.use('/api/nfts', authMiddleware, nftRoutes);
app.use('/api/ai', authMiddleware, aiRoutes);
app.use('/api/dao', authMiddleware, daoRoutes);
app.use('/api/analytics', authMiddleware, analyticsRoutes);

// 404 handler
app.use('*', (req, res) => {
  res.status(404).json({
    error: 'Endpoint not found',
    message: `The requested endpoint ${req.originalUrl} does not exist`,
    availableEndpoints: [
      '/api/auth',
      '/api/profiles',
      '/api/posts',
      '/api/nfts',
      '/api/ai',
      '/api/dao',
      '/api/analytics',
    ],
  });
});

// Error handling middleware
app.use(errorHandler);

// Initialize services
async function initializeServices() {
  try {
    // Initialize blockchain service
    await blockchainService.initialize();
    logger.info('Blockchain service initialized');

    // Initialize IPFS service
    await ipfsService.initialize();
    logger.info('IPFS service initialized');

    // Initialize AI service
    await aiService.initialize();
    logger.info('AI service initialized');

    logger.info('All services initialized successfully');
  } catch (error) {
    logger.error('Failed to initialize services:', error);
    process.exit(1);
  }
}

// Start server
async function startServer() {
  try {
    await initializeServices();
    
    app.listen(PORT, () => {
      logger.info(`Parrot API server running on port ${PORT}`);
      logger.info(`Environment: ${process.env.NODE_ENV || 'development'}`);
      logger.info(`Health check: http://localhost:${PORT}/health`);
      logger.info(`API documentation: http://localhost:${PORT}/api`);
    });
  } catch (error) {
    logger.error('Failed to start server:', error);
    process.exit(1);
  }
}

// Graceful shutdown
process.on('SIGTERM', () => {
  logger.info('SIGTERM received, shutting down gracefully');
  process.exit(0);
});

process.on('SIGINT', () => {
  logger.info('SIGINT received, shutting down gracefully');
  process.exit(0);
});

// Handle uncaught exceptions
process.on('uncaughtException', (error) => {
  logger.error('Uncaught Exception:', error);
  process.exit(1);
});

process.on('unhandledRejection', (reason, promise) => {
  logger.error('Unhandled Rejection at:', promise, 'reason:', reason);
  process.exit(1);
});

// Start the server
startServer();

module.exports = app; 