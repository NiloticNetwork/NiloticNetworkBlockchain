# Sulwestake Deployment Guide

This guide covers deploying the Sulwestake DApp to various platforms and environments.

## 🚀 Quick Deployment

### Prerequisites
- Node.js 18+ installed
- Git repository access
- Web3Modal project ID
- Deployed staking smart contract

### Environment Setup

1. **Create environment file**
   ```bash
   cp .env.example .env.local
   ```

2. **Configure environment variables**
   ```env
   NEXT_PUBLIC_WEB3MODAL_PROJECT_ID=your_project_id_here
   NEXT_PUBLIC_NILOTIC_CHAIN_ID=1337
   NEXT_PUBLIC_STAKING_CONTRACT_ADDRESS=0x...
   NEXT_PUBLIC_RPC_URL=http://localhost:8545
   ```

## 🌐 Platform Deployments

### Vercel (Recommended)

1. **Connect to Vercel**
   - Install Vercel CLI: `npm i -g vercel`
   - Login: `vercel login`

2. **Deploy**
   ```bash
   vercel --prod
   ```

3. **Configure environment variables in Vercel dashboard**
   - Go to your project settings
   - Add all environment variables from `.env.local`

### Netlify

1. **Build the project**
   ```bash
   npm run build
   ```

2. **Deploy to Netlify**
   - Drag and drop the `out` folder to Netlify
   - Or use Netlify CLI: `netlify deploy`

3. **Configure environment variables**
   - Go to Site settings > Environment variables
   - Add all required variables

### IPFS (Decentralized)

1. **Build for static export**
   ```bash
   npm run build
   npm run export
   ```

2. **Upload to IPFS**
   ```bash
   # Using IPFS CLI
   ipfs add -r out/
   
   # Or using Pinata
   # Upload the out/ folder to Pinata
   ```

3. **Update DNS**
   - Point your domain to IPFS gateway
   - Example: `ipfs.io/ipfs/QmYourHash`

### Docker Deployment

1. **Create Dockerfile**
   ```dockerfile
   FROM node:18-alpine AS base
   
   # Install dependencies only when needed
   FROM base AS deps
   RUN apk add --no-cache libc6-compat
   WORKDIR /app
   
   # Install dependencies based on the preferred package manager
   COPY package.json package-lock.json* ./
   RUN npm ci --only=production
   
   # Rebuild the source code only when needed
   FROM base AS builder
   WORKDIR /app
   COPY --from=deps /app/node_modules ./node_modules
   COPY . .
   
   # Next.js collects completely anonymous telemetry data about general usage.
   # Learn more here: https://nextjs.org/telemetry
   # Uncomment the following line in case you want to disable telemetry during the build.
   ENV NEXT_TELEMETRY_DISABLED 1
   
   RUN npm run build
   
   # Production image, copy all the files and run next
   FROM base AS runner
   WORKDIR /app
   
   ENV NODE_ENV production
   ENV NEXT_TELEMETRY_DISABLED 1
   
   RUN addgroup --system --gid 1001 nodejs
   RUN adduser --system --uid 1001 nextjs
   
   COPY --from=builder /app/public ./public
   
   # Set the correct permission for prerender cache
   RUN mkdir .next
   RUN chown nextjs:nodejs .next
   
   # Automatically leverage output traces to reduce image size
   # https://nextjs.org/docs/advanced-features/output-file-tracing
   COPY --from=builder --chown=nextjs:nodejs /app/.next/standalone ./
   COPY --from=builder --chown=nextjs:nodejs /app/.next/static ./.next/static
   
   USER nextjs
   
   EXPOSE 3000
   
   ENV PORT 3000
   ENV HOSTNAME "0.0.0.0"
   
   CMD ["node", "server.js"]
   ```

2. **Build and run**
   ```bash
   docker build -t sulwestake .
   docker run -p 3000:3000 sulwestake
   ```

## 🔧 Configuration

### Smart Contract Configuration

1. **Update contract address**
   - Replace `STAKING_CONTRACT_ADDRESS` in `hooks/useStaking.ts`
   - Update ABI if contract functions differ

2. **Configure chains**
   - Update chain configuration in `providers/WagmiProvider.tsx`
   - Add Nilotic blockchain RPC endpoints

### Web3Modal Setup

1. **Get project ID**
   - Visit [Web3Modal Cloud](https://cloud.walletconnect.com/)
   - Create new project
   - Copy project ID

2. **Configure in code**
   - Update `projectId` in `layout.tsx`
   - Update `projectId` in `providers/WagmiProvider.tsx`

### Performance Optimization

1. **Enable compression**
   ```bash
   npm install compression
   ```

2. **Configure caching**
   ```nginx
   # Nginx configuration
   location /_next/static {
     expires 1y;
     add_header Cache-Control "public, immutable";
   }
   ```

3. **Enable CDN**
   - Configure Cloudflare or similar CDN
   - Enable edge caching for static assets

## 🔒 Security Configuration

### HTTPS Setup

1. **SSL Certificate**
   ```bash
   # Using Let's Encrypt
   certbot --nginx -d yourdomain.com
   ```

2. **Security Headers**
   ```javascript
   // next.config.js
   const securityHeaders = [
     {
       key: 'X-DNS-Prefetch-Control',
       value: 'on'
     },
     {
       key: 'Strict-Transport-Security',
       value: 'max-age=63072000; includeSubDomains; preload'
     },
     {
       key: 'X-XSS-Protection',
       value: '1; mode=block'
     },
     {
       key: 'X-Frame-Options',
       value: 'SAMEORIGIN'
     },
     {
       key: 'X-Content-Type-Options',
       value: 'nosniff'
     }
   ];
   ```

### Environment Security

1. **Secrets management**
   ```bash
   # Use environment variables for secrets
   export NEXT_PUBLIC_WEB3MODAL_PROJECT_ID="your_secret"
   ```

2. **Access control**
   - Implement rate limiting
   - Add API key authentication if needed
   - Monitor for suspicious activity

## 📊 Monitoring & Analytics

### Error Tracking

1. **Sentry Setup**
   ```bash
   npm install @sentry/nextjs
   ```

2. **Configure Sentry**
   ```javascript
   // sentry.client.config.js
   import * as Sentry from "@sentry/nextjs";
   
   Sentry.init({
     dsn: process.env.NEXT_PUBLIC_SENTRY_DSN,
     tracesSampleRate: 1.0,
   });
   ```

### Analytics

1. **Google Analytics**
   ```javascript
   // Add to _app.js or layout.tsx
   import { GoogleAnalytics } from 'nextjs-google-analytics';
   
   export default function Layout({ children }) {
     return (
       <>
         <GoogleAnalytics trackPageViews />
         {children}
       </>
     );
   }
   ```

2. **Custom Analytics**
   ```javascript
   // Track staking events
   const trackStakingEvent = (amount, action) => {
     gtag('event', action, {
       event_category: 'staking',
       event_label: amount,
       value: amount
     });
   };
   ```

## 🧪 Testing Deployment

### Pre-deployment Checklist

- [ ] All environment variables configured
- [ ] Smart contract deployed and verified
- [ ] Web3Modal project ID set
- [ ] RPC endpoints tested
- [ ] Mobile responsiveness verified
- [ ] Cross-browser compatibility tested

### Post-deployment Testing

1. **Functionality tests**
   ```bash
   # Run E2E tests
   npm run test:e2e
   ```

2. **Performance tests**
   ```bash
   # Lighthouse CI
   npm install -g lighthouse
   lighthouse https://yourdomain.com
   ```

3. **Security tests**
   ```bash
   # OWASP ZAP
   zap-baseline.py -t https://yourdomain.com
   ```

## 🔄 CI/CD Pipeline

### GitHub Actions

```yaml
# .github/workflows/deploy.yml
name: Deploy to Production

on:
  push:
    branches: [main]

jobs:
  deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Setup Node.js
        uses: actions/setup-node@v3
        with:
          node-version: '18'
          
      - name: Install dependencies
        run: npm ci
        
      - name: Run tests
        run: npm test
        
      - name: Build application
        run: npm run build
        env:
          NEXT_PUBLIC_WEB3MODAL_PROJECT_ID: ${{ secrets.WEB3MODAL_PROJECT_ID }}
          NEXT_PUBLIC_STAKING_CONTRACT_ADDRESS: ${{ secrets.STAKING_CONTRACT_ADDRESS }}
          
      - name: Deploy to Vercel
        uses: amondnet/vercel-action@v25
        with:
          vercel-token: ${{ secrets.VERCEL_TOKEN }}
          vercel-org-id: ${{ secrets.ORG_ID }}
          vercel-project-id: ${{ secrets.PROJECT_ID }}
          working-directory: ./
```

## 🚨 Troubleshooting

### Common Issues

1. **Build failures**
   ```bash
   # Clear cache
   rm -rf .next
   npm run build
   ```

2. **Environment variables not loading**
   ```bash
   # Check variable names
   # Ensure NEXT_PUBLIC_ prefix for client-side variables
   ```

3. **Wallet connection issues**
   - Verify Web3Modal project ID
   - Check chain configuration
   - Test with different wallets

4. **Performance issues**
   ```bash
   # Analyze bundle size
   npm install -g @next/bundle-analyzer
   ANALYZE=true npm run build
   ```

### Support

For deployment issues:
- Check [Vercel documentation](https://vercel.com/docs)
- Review [Next.js deployment guide](https://nextjs.org/docs/deployment)
- Contact the development team

---

**Happy deploying! 🚀** 